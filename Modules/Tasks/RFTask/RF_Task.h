/*
 * RF1_Task.h
 *
 *  Created on: Nov 15, 2023
 *      Author: Uzivatel
 */

#ifndef INC_RF1_TASK_H_
#define INC_RF1_TASK_H_

#include "main.h"
#include "ralf_defs.h"
#include "ralf.h"


#define RF_CNT			1

typedef enum
{
	RF_MODE_NONE	=	0,
	RF_MODE_RX		=	1,
	RF_MODE_TX		=	2,
	RF_MODE_CAD		=	3,
	RF_MODE_IDLE	=	4,

}radio_modes_e;





typedef struct{
	uint8_t	*payload;
	uint8_t	 size;

}payloadSend_t;

/*
 *
 */
typedef struct
{
	uint32_t 		pin;
	GPIO_TypeDef	*port;

}pin_struct_t;

typedef struct {

	pin_struct_t 			pin_NSS;				/* SPI Select pin */
	pin_struct_t			pin_BUSY;				/* BUSY pin */
	pin_struct_t			pin_RESET;				/* Reset pin */
	pin_struct_t			pin_DIO1;				/* DIO1 - IRQ 1 pin */
    pin_struct_t		    pin_RF_SWITCH_1;			/* RF switch */
	pin_struct_t		    pin_RF_SWITCH_2;			/* RF switch */
	SPI_HandleTypeDef	    *target;				/* SPI target */
#if (RF_USE_DMA==1)
	osSemaphoreId		RFBinarySPISemaphore;
	uint8_t				Gl_RF_RX_SPI_DMA_Buffer[100];
	uint8_t				Gl_RF_TX_SPI_DMA_Buffer[100];
	void (*RF_TX_DMATransferComplete(void));
	void (*RF_RX_DMATransferComplete(void));
#endif
	bool 				TCXO_is_used;
    bool                DIO2_AS_RF_SWITCH;
	void (*AtomicActionEnter)(void);				/* pointer na funkci - vstup do Atomicke oblasti  */
	void (*AtomicActionExit)(void);					/* pointer na funkci - vystup do Atomicke oblasti */
//	void (*RadioRFSwitch) (Enum_RF_switch state);

}radio_hal_cfg_t;

typedef struct
{
	radio_hal_cfg_t		radioHal;
    ralf_params_lora_t 	loraParam_rx;
	ralf_params_lora_t 	loraParam_tx;
	radio_modes_e		lastMode;
	ralf_t				ralf;

}radioConfig_t;

typedef struct
{
	bool			txDone;
	bool 			rxDone;
	uint8_t			packetsize;
	void 			*ptrPacket;

}rfTemp;



typedef enum
{
	RF_TASK_OFF = 0,
	RF_TASK_ON = 1,

}radio_task_state_e;


/*
 *
 */
typedef struct RF_StateAutomat
{
	radio_task_state_e currentState;
	radio_task_state_e previousState;
} radio_states_t;


typedef struct
{
	//TimerResource_t	rfTimer;
	TimerResource_t	rfHBTimer;	// pokud SX1262 nevykona delsi dobu zadnou akci,
								// tak ho znovu aktivujeme
	TimerResource_t rfEventLedTimer; // LED indikace udalosti (RX/TX)


}RFTimers_t;

typedef struct
{
	radioConfig_t		rfConfig;
	radio_states_t		rfTaskState;
	RFTimers_t			timers;
	bool				rx_to_uart;		//true, false

} radio_context_t;

void radio_task(void);
bool RT_SendDataToRFTask(dataQueue_t *data, TickType_t timeout);



#endif /* INC_RF1_TASK_H_ */
