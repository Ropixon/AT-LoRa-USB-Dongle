/**
 * @file hw_board_v0_0.h
 * @brief Hardware pin definitions for AT-USB LoRa Dongle v0.0 (black PCB prototype)
 * @date 2026-01-13
 *
 * DEPRECATED / OBSOLETE: v0.0 was the original black-PCB prototype run.
 * It is no longer manufactured or used - kept only for reference and so
 * old prototype units can still be rebuilt if ever needed.
 *
 * IMPORTANT: This file contains COMPLETE pin definitions for v0.0
 * When CubeMX regenerates main.h for later revisions, v0.0 builds still work
 * because they use these frozen pin definitions.
 */

#ifndef HW_BOARD_V0_0_H
#define HW_BOARD_V0_0_H

/* Board version */
#define HW_BOARD_VERSION_MAJOR  0
#define HW_BOARD_VERSION_MINOR  0
#define HW_BOARD_VERSION_STRING "v0.0-proto"

/* ============================================================================
 * Complete pin definitions for v0.0 hardware (black PCB prototype, obsolete)
 * ============================================================================ */

/* SX1262 Radio pins */
#undef SX1262_NSS_Pin
#undef SX1262_NSS_GPIO_Port
#define SX1262_NSS_Pin GPIO_PIN_4
#define SX1262_NSS_GPIO_Port GPIOA

#undef SX1262_RESET_Pin
#undef SX1262_RESET_GPIO_Port
#define SX1262_RESET_Pin GPIO_PIN_1
#define SX1262_RESET_GPIO_Port GPIOB

#undef SX1262_DIO1_Pin
#undef SX1262_DIO1_GPIO_Port
#undef SX1262_DIO1_EXTI_IRQn
#define SX1262_DIO1_Pin GPIO_PIN_2
#define SX1262_DIO1_GPIO_Port GPIOB
#define SX1262_DIO1_EXTI_IRQn EXTI2_3_IRQn

#undef SX1262_BUSY_Pin
#undef SX1262_BUSY_GPIO_Port
#define SX1262_BUSY_Pin GPIO_PIN_10
#define SX1262_BUSY_GPIO_Port GPIOB

#undef SX1262_RF_SW_Pin
#undef SX1262_RF_SW_GPIO_Port
#define SX1262_RF_SW_Pin GPIO_PIN_8
#define SX1262_RF_SW_GPIO_Port GPIOA

/* LED pins */
#undef LED_GREEN_Pin
#undef LED_GREEN_GPIO_Port
#define LED_GREEN_Pin GPIO_PIN_15
#define LED_GREEN_GPIO_Port GPIOB

#undef LED_BLUE_Pin
#undef LED_BLUE_GPIO_Port
#define LED_BLUE_Pin GPIO_PIN_5
#define LED_BLUE_GPIO_Port GPIOB

/* Logical LED mapping for AT RX indication - v0.0 uses BLUE */
#define LED_AT_RX_Pin       LED_BLUE_Pin
#define LED_AT_RX_GPIO_Port LED_BLUE_GPIO_Port
#define HW_HAS_LED_AT_RX    1

/* LED control macros for AT RX indication - v0.0 uses BLUE */
#define HW_LED_AT_RX_ON()   HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET)
#define HW_LED_AT_RX_OFF()  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET)

/* Logical LED mapping for RF RX Event - v0.0 does NOT have this LED */
#define HW_LED_RF_EVENT_ON()   do { /* No LED in v0.0 */ } while(0)
#define HW_LED_RF_EVENT_OFF()  do { /* No LED in v0.0 */ } while(0)

/* LED initialization macro - turn off all LEDs at startup */
#define HW_LED_INIT() do { \
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET); \
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET); \
} while(0)

/* Board-specific GPIO post-initialization */
/* v0.0: Initialize RF_SW (PA8) - ensures correct init regardless of CubeMX config */
#define HW_GPIO_PostInit() do { \
    GPIO_InitTypeDef GPIO_InitStruct = {0}; \
    /* Initialize RF_SW (PA8) */ \
    GPIO_InitStruct.Pin = SX1262_RF_SW_Pin; \
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; \
    GPIO_InitStruct.Pull = GPIO_NOPULL; \
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; \
    HAL_GPIO_Init(SX1262_RF_SW_GPIO_Port, &GPIO_InitStruct); \
    HAL_GPIO_WritePin(SX1262_RF_SW_GPIO_Port, SX1262_RF_SW_Pin, GPIO_PIN_RESET); \
} while(0)

/* Auxiliary pins */
#undef aux1_Pin
#undef aux1_GPIO_Port
#define aux1_Pin GPIO_PIN_9
#define aux1_GPIO_Port GPIOB

#undef aux2_Pin
#undef aux2_GPIO_Port
#define aux2_Pin GPIO_PIN_4
#define aux2_GPIO_Port GPIOB

#undef aux3_Pin
#undef aux3_GPIO_Port
#define aux3_Pin GPIO_PIN_3
#define aux3_GPIO_Port GPIOB

#undef aux4_Pin
#undef aux4_GPIO_Port
#define aux4_Pin GPIO_PIN_12
#define aux4_GPIO_Port GPIOA

#undef aux5_Pin
#undef aux5_GPIO_Port
#define aux5_Pin GPIO_PIN_7
#define aux5_GPIO_Port GPIOB

#undef aux6_Pin
#undef aux6_GPIO_Port
#define aux6_Pin GPIO_PIN_6
#define aux6_GPIO_Port GPIOB

#undef aux7_Pin
#undef aux7_GPIO_Port
#define aux7_Pin GPIO_PIN_3
#define aux7_GPIO_Port GPIOA

#undef aux8_Pin
#undef aux8_GPIO_Port
#define aux8_Pin GPIO_PIN_2
#define aux8_GPIO_Port GPIOA

#endif /* HW_BOARD_V0_0_H */
