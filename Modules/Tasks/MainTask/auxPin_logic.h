

#ifndef AUX_PIN_LOGIC_H
#define AUX_PIN_LOGIC_H

#include "main.h"
#include "FreeRTOS.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pinMask;
} AUX_PinControl_t;

#define AUX_PINS_COUNT 8

// Globální pole AUX pinů
extern AUX_PinControl_t auxPins[AUX_PINS_COUNT];

// aux7 (index 6, PA3/TIM21_CH2) and aux8 (index 7, PA2/TIM2_CH3) are driven by
// true hardware PWM peripherals; the rest run on FreeRTOS software timers.
bool AUX_IsHwPwmPin(uint8_t index);

// Valid range of AUX_StartPWM's frequency argument (Hz) - differs per engine.
// Single source of truth for both the AT+AUX_PULSE help text (AT_cmd.c) and its
// argument validation (general_sys_cmd.c).
#define AUX_PWM_SW_MIN_FREQ_HZ 1
// Hard ceiling: a software timer can't resolve a phase shorter than 1 RTOS tick, so any
// duty other than 0/100% needs a period of at least 2 ticks. Derived from this build's
// actual configTICK_RATE_HZ (FreeRTOSConfig.h) rather than a hardcoded assumption, so it
// tracks automatically if the tick rate ever changes.
#define AUX_PWM_SW_MAX_FREQ_HZ (configTICK_RATE_HZ / 2)
#define AUX_PWM_HW_MIN_FREQ_HZ 1
// Hard ceiling: below ARR=100 the duty resolution gets coarser than 1%, so this is
// where TIM2/TIM21 (32MHz APB clock, see SystemClock_Config) stop being able to hit
// arbitrary integer duty values precisely. TIMCLK / 100.
#define AUX_PWM_HW_MAX_FREQ_HZ 320000UL

// Nearest duty (0-100) actually achievable on the SW (FreeRTOS timer) engine at freq_hz,
// given that neither phase can be shorter than 1 RTOS tick. Equal to `duty` whenever the
// exact value is representable; the caller can compare to detect and report rounding.
// Not applicable to HW PWM pins, which have no such restriction (fine-grained HW compare).
uint8_t AUX_SwPwm_RoundDuty(uint32_t freq_hz, uint8_t duty);

void AUX_InitTimers(void);
void AUX_StartPWM(uint8_t index, uint32_t freq_hz, uint8_t duty);
void AUX_StopPWM(uint8_t index);
#endif
