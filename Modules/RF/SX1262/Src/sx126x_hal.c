/*
 * sx126x_hal.c
 *
 *  Created on: Apr 3, 2023
 *      Author: jirik
 */

/************************************************************************/
/* include header												   		*/
/************************************************************************/
#include "sx126x_hal.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include <stddef.h>
#include "spi.h"
#include "ral_sx126x_bsp.h"

/************************************************************************/
/* Declaration importing objects                                        */
/************************************************************************/

/************************************************************************/
/* Definition global variables                                          */
/************************************************************************/

/************************************************************************/
/* Local #DEFINE														*/
/************************************************************************/
#define SX126X_BUSY_TIMEOUT_MS		100		// max. doba cekani na BUSY - kdyz radio nereaguje, nezasekneme se
#define SX126X_BUSY_SPIN_MS			1		// do teto doby cekame aktivne, pak osDelay(1)

/************************************************************************/
/* Local TYPEDEF												   		*/
/************************************************************************/
typedef enum radio_mode_e
{
    RADIO_SLEEP,
    RADIO_AWAKE
} radio_mode_t;

/************************************************************************/
/* Definition local variables										   	*/
/************************************************************************/
static volatile radio_mode_t radio_mode = RADIO_SLEEP;

/************************************************************************/
/* Declaration functions											   	*/
/************************************************************************/
void SX126xWaitOnBusy(GPIO_TypeDef *GPIOx, uint16_t pin);
sx126x_hal_status_t sx126xCheckDeviceReady(const void* context );

/************************************************************************/
/* Definition functions                                                 */
/************************************************************************/

/**
 * Radio data transfer - write
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context          Radio implementation parameters
 * @param [in] command          Pointer to the buffer to be transmitted
 * @param [in] command_length   Buffer size to be transmitted
 * @param [in] data             Pointer to the buffer to be transmitted
 * @param [in] data_length      Buffer size to be transmitted
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_write(const void *context, const uint8_t *command, const uint16_t command_length, const uint8_t *data,
		const uint16_t data_length)
{
	/* alokujeme statisky - tedy bereme to ze stacku - nikoliv z heapu */

	radio_hal_cfg_t* spiDev;
	spiDev = (radio_hal_cfg_t*) context;


	if(sx126xCheckDeviceReady(context) != SX126X_HAL_STATUS_OK)	return SX126X_HAL_STATUS_ERROR;
#if(FRAM_RF_SPI_WITH_DMA == 1)

	if(SPI_RFFRAMGetLine(SPI_RFFRAM_Select_RF)==false)	_exit(785186);
	if(SPI_RFFRAMTransmit_DMA_NoNSS(SPI_RFFRAM_Select_RF, (uint8_t*)command, command_length) != HAL_OK)
	{
		SPI_RFFRAMFreeLine(SPI_RFFRAM_Select_RF);
		return SX126X_HAL_STATUS_ERROR;
	}
	if(data_length > 0)
	{
		if(SPI_RFFRAMTransmit_DMA_NoNSS(SPI_RFFRAM_Select_RF, (uint8_t*)data, data_length) != HAL_OK)
		{
			SPI_RFFRAMFreeLine(SPI_RFFRAM_Select_RF);
			return SX126X_HAL_STATUS_ERROR;
		}
	}

	if(SPI_RFFRAMFreeLine(SPI_RFFRAM_Select_RF) == false)	_exit(785185);

#else
	vTaskSuspendAll();
	//Put NSS low to start spi transaction
	HAL_GPIO_WritePin(spiDev->pin_NSS.port, spiDev->pin_NSS.pin, GPIO_PIN_RESET);

	if(HAL_SPI_Transmit(spiDev->target,command,command_length,0xffff) != HAL_OK)					_exit(789);
	if(data_length > 0)	if(HAL_SPI_Transmit(spiDev->target,data,data_length,0xffff) != HAL_OK) 	_exit(789);
	
	HAL_GPIO_WritePin(spiDev->pin_NSS.port, spiDev->pin_NSS.pin, GPIO_PIN_SET);

	xTaskResumeAll();
#endif
	
	// 0x84 - SX126x_SET_SLEEP opcode. In sleep mode the radio dio is struck to 1 => do not test it
	if( command[0] != 0x84 )
	{
		if(sx126xCheckDeviceReady(context) != SX126X_HAL_STATUS_OK)	return SX126X_HAL_STATUS_ERROR;
	}
	else
	{
		radio_mode = RADIO_SLEEP;
	}

	return SX126X_HAL_STATUS_OK;
}

/**
 * Radio data transfer - read
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context          Radio implementation parameters
 * @param [in] command          Pointer to the buffer to be transmitted
 * @param [in] command_length   Buffer size to be transmitted
 * @param [in] data             Pointer to the buffer to be received
 * @param [in] data_length      Buffer size to be received
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_read(const void *context, const uint8_t *command, const uint16_t command_length, uint8_t *data,
		const uint16_t data_length)
{
	if(sx126xCheckDeviceReady(context) != SX126X_HAL_STATUS_OK)	return SX126X_HAL_STATUS_ERROR;

#if(FRAM_RF_SPI_WITH_DMA == 1)
	
	if(SPI_RFFRAMGetLine(SPI_RFFRAM_Select_RF)==false)	_exit(785186);
	if(SPI_RFFRAMTransmit_DMA_NoNSS(SPI_RFFRAM_Select_RF, (uint8_t*)command, command_length) != HAL_OK)
	{
		SPI_RFFRAMFreeLine(SPI_RFFRAM_Select_RF);
		return SX126X_HAL_STATUS_ERROR;
	}

	if(data_length > 0)
	{	
		if(SPI_RFFRAMReceive_DMA_NoNSS(SPI_RFFRAM_Select_RF,(uint8_t*) data, data_length) != HAL_OK) 	
		{
			SPI_RFFRAMFreeLine(SPI_RFFRAM_Select_RF);
			return SX126X_HAL_STATUS_ERROR;
		}
	}
	
	if(SPI_RFFRAMFreeLine(SPI_RFFRAM_Select_RF) == false)	_exit(785185);

#else

	vTaskSuspendAll();
	radio_hal_cfg_t* spiDev;
	spiDev = (radio_hal_cfg_t*) context;

	// Put NSS low to start spi transaction
	HAL_GPIO_WritePin(spiDev->pin_NSS.port, spiDev->pin_NSS.pin, GPIO_PIN_RESET);

	if(HAL_SPI_Transmit(spiDev->target,command,command_length,0xffff) != HAL_OK)				_exit(787);

	if(data_length > 0)	if(HAL_SPI_Receive(spiDev->target,data,data_length,0xffff) != HAL_OK) 	_exit(788);

    // Put NSS high as the spi transaction is finished
	HAL_GPIO_WritePin(spiDev->pin_NSS.port, spiDev->pin_NSS.pin, GPIO_PIN_SET);

	xTaskResumeAll();

#endif
    return SX126X_HAL_STATUS_OK;
}

/**
 * Reset the radio
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context Radio implementation parameters
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_reset(const void *context)
{
	radio_hal_cfg_t* spiDev;
	spiDev = (radio_hal_cfg_t*) context;

	HAL_GPIO_WritePin(spiDev->pin_NSS.port, spiDev->pin_NSS.pin, GPIO_PIN_SET);

	HAL_GPIO_WritePin(spiDev->pin_RESET.port, spiDev->pin_RESET.pin, GPIO_PIN_SET);
	osDelay(5);
	HAL_GPIO_WritePin(spiDev->pin_RESET.port, spiDev->pin_RESET.pin, GPIO_PIN_RESET);
	osDelay(5);
	HAL_GPIO_WritePin(spiDev->pin_RESET.port, spiDev->pin_RESET.pin, GPIO_PIN_SET);
	osDelay(5);

	if(sx126xCheckDeviceReady(context) != SX126X_HAL_STATUS_OK)	return SX126X_HAL_STATUS_ERROR;

	radio_mode = RADIO_AWAKE;
	return SX126X_HAL_STATUS_OK;
}

/**
 * Wake the radio up.
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context Radio implementation parameters
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_wakeup( const void* context )
{
//	const uint8_t buf[1] = {
//				0xC0,
//			};
//		uint8_t         status_local = 0;
//
//		sx126x_hal_read( context, buf, 1, &status_local, 1 );
//		return SX126X_HAL_STATUS_OK;

	return sx126xCheckDeviceReady(context);
}


/*
 *
 */
static sx126x_hal_status_t sx126x_hal_wait_on_busy(const void* context)
{
	radio_hal_cfg_t* spiDev;
	spiDev = (radio_hal_cfg_t*) context;
	uint32_t start = HAL_GetTick();

    while( HAL_GPIO_ReadPin(spiDev->pin_BUSY.port, spiDev->pin_BUSY.pin)==GPIO_PIN_SET )
    {
    	uint32_t elapsed = HAL_GetTick() - start;

    	if(elapsed > SX126X_BUSY_TIMEOUT_MS)	return SX126X_HAL_STATUS_ERROR;

    	// BUSY bezne spadne za par us - aktivne cekame. Trva-li dele (kalibrace, porucha),
    	// uvolnime CPU ostatnim taskum. Nesmi se volat uvnitr vTaskSuspendAll()!
    	if(elapsed >= SX126X_BUSY_SPIN_MS)	osDelay(1);
    }

    return SX126X_HAL_STATUS_OK;
}


/*
 *
 */
sx126x_hal_status_t sx126xCheckDeviceReady(const void* context )
{
	radio_hal_cfg_t* spiDev;
	spiDev = (radio_hal_cfg_t*) context;
	sx126x_hal_status_t status;

	if( radio_mode != RADIO_SLEEP )
	{
		status = sx126x_hal_wait_on_busy(context);
	}
	else
	{
		// Busy is HIGH in sleep mode, wake-up the device
		HAL_GPIO_WritePin(spiDev->pin_NSS.port, spiDev->pin_NSS.pin, GPIO_PIN_RESET);
		status = sx126x_hal_wait_on_busy(context);
		HAL_GPIO_WritePin(spiDev->pin_NSS.port, spiDev->pin_NSS.pin, GPIO_PIN_SET);
		if(status == SX126X_HAL_STATUS_OK)	radio_mode = RADIO_AWAKE;
	}

	return status;
}
