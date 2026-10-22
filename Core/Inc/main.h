/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "timers.h"
#include "stm32l0xx.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "core_cm0plus.h"
#include "Constrain.h"
#include <unistd.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

#define CONST_1_SEC		(1000)
#define CONST_1_MIN		(60*CONST_1_SEC)
#define CONST_1_HR		(60*CONST_1_MIN)

typedef struct
{
	uint32_t	cmd;        // Command identifier or code
	uint32_t	data;       // Main data value
	uint32_t	tmp_32;     // Temporary storage for 32-bit data
	uint16_t	tmp_16;     // Temporary storage for 16-bit data
	uint8_t		tmp_8;      // Temporary storage for 8-bit data
	bool		  tmp_bool;   // Temporary boolean for flags or conditions

	void 		*ptr;       // Pointer to additional data if needed

} dataQueue_t;


typedef struct
{
	TimerHandle_t	timer;
	StaticTimer_t	timerPlace;

}TimerResource_t;



/**
 * @brief Command identifiers for the RF task
 * 
 */
#define CMD_RF_TURN_ON			  254
#define CMD_RF_TURN_OFF			  253
#define CMD_RF_IRQ_FIRED		  252
#define CMD_RF_SEND_PACKET		251


/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define aux8_Pin GPIO_PIN_2
#define aux8_GPIO_Port GPIOA
#define aux7_Pin GPIO_PIN_3
#define aux7_GPIO_Port GPIOA
#define SX1262_NSS_Pin GPIO_PIN_4
#define SX1262_NSS_GPIO_Port GPIOA
#define SX1262_RESET_Pin GPIO_PIN_1
#define SX1262_RESET_GPIO_Port GPIOB
#define SX1262_DIO1_Pin GPIO_PIN_2
#define SX1262_DIO1_GPIO_Port GPIOB
#define SX1262_DIO1_EXTI_IRQn EXTI2_3_IRQn
#define SX1262_BUSY_Pin GPIO_PIN_10
#define SX1262_BUSY_GPIO_Port GPIOB
#define LED_GREEN_Pin GPIO_PIN_15
#define LED_GREEN_GPIO_Port GPIOB
#define SX1262_RF_SW_Pin GPIO_PIN_12
#define SX1262_RF_SW_GPIO_Port GPIOB
#define aux4_Pin GPIO_PIN_12
#define aux4_GPIO_Port GPIOA
#define aux3_Pin GPIO_PIN_3
#define aux3_GPIO_Port GPIOB
#define aux2_Pin GPIO_PIN_4
#define aux2_GPIO_Port GPIOB
#define LED_BLUE_Pin GPIO_PIN_5
#define LED_BLUE_GPIO_Port GPIOB
#define aux6_Pin GPIO_PIN_6
#define aux6_GPIO_Port GPIOB
#define aux5_Pin GPIO_PIN_7
#define aux5_GPIO_Port GPIOB
#define aux1_Pin GPIO_PIN_9
#define aux1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* Include hardware configuration - must be after pin definitions above */
#include "hw_config.h"

/**
 * @brief Command identifiers for the main task
 * 
 */
#define CMD_MAIN_RF_IS_ON				      254
#define CMD_MAIN_RF_IS_OFF			      253
#define CMD_MAIN_RF_RX_PACKET		      252
#define CMD_MAIN_AT_RX_PACKET		      251
#define CMD_MAIN_IWDG_REFRESH		      250   // Start heartbeat collection
#define CMD_MAIN_HB_RESPONSE_RF       249   // RF task heartbeat response
#define CMD_MAIN_RF_TX_DONE           248
#define CMD_MAIN_RF_RADIO_ERROR       247   // Radio neinicializovano / nereaguje

#define CMD_RF_TURN_ON			    254
#define CMD_RF_TURN_OFF			    253
#define CMD_RF_IRQ_FIRED		    252
#define CMD_RF_SEND_DATA_NOW		251
#define CMD_RF_SEND_DATA_LBT		250
#define CMD_RF_RADIO_HB         249
#define CMD_RF_RADIO_RX_TO_UART 248
#define CMD_RF_RADIO_RECONFIG_RX 247
#define CMD_RF_HB_REQUEST       246   // Heartbeat request from main task
#define CMD_RF_TX_CW            245   // Start/Stop TX CW mode



#define CMD_CORE_RF_IS_ON			254
#define CMD_CORE_RF_IS_OFF			253
#define CMD_CORE_SEND_TO_UART		252
#define CMD_CORE_CHECK_UART_RX		251
#define CMD_CORE_RF_RX_DONE			250




/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
