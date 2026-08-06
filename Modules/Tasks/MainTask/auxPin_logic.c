

#include "auxPin_logic.h"
#include "FreeRTOS.h"
#include "main.h"

#define LOG_LEVEL	LOG_LEVEL_INFO
#include "Log.h"

// aux1-6: FreeRTOS software timers (period reprogrammed on every edge).
// aux7 (PA3/TIM21_CH2) and aux8 (PA2/TIM2_CH3): true hardware PWM peripherals,
// each on its own timer instance so their frequencies are fully independent.

AUX_PinControl_t auxPins[AUX_PINS_COUNT] = {
    {aux1_GPIO_Port, aux1_Pin}, // index 0
    {aux2_GPIO_Port, aux2_Pin}, // index 1
    {aux3_GPIO_Port, aux3_Pin}, // index 2
    {aux4_GPIO_Port, aux4_Pin}, // index 3
    {aux5_GPIO_Port, aux5_Pin}, // index 4
    {aux6_GPIO_Port, aux6_Pin}, // index 5
    {aux7_GPIO_Port, aux7_Pin}, // index 6 - HW PWM (TIM21_CH2)
    {aux8_GPIO_Port, aux8_Pin}  // index 7 - HW PWM (TIM2_CH3)
};

bool AUX_IsHwPwmPin(uint8_t index)
{
    return index == 6 || index == 7;
}

// ---------------------------------------------------------------------------
// SW engine (aux1-6): FreeRTOS software timer per pin
// ---------------------------------------------------------------------------

// Everything here is expressed in raw RTOS ticks (not ms) so the achievable
// range always matches this build's actual configTICK_RATE_HZ, whatever it is.
#define AUX_MIN_PHASE_TICKS 1  // shortest phase a software timer can resolve: 1 tick

typedef struct
{
    uint32_t      highTicks;
    uint32_t      lowTicks;
    TimerHandle_t timer;
    bool          isHigh;
} AUX_SwPwmState_t;

static AUX_SwPwmState_t auxSwState[AUX_PINS_COUNT];

// Round-to-nearest-tick split of a period into HIGH/LOW phases for the requested duty,
// clamped so neither phase drops below the 1-tick floor. Single source of truth used by
// both AUX_SwPwm_RoundDuty (reporting) and AUX_SwPwm_Start (execution) so they never disagree.
static void AUX_ComputeTicks(uint32_t period_ticks, uint8_t duty, uint32_t *outHigh, uint32_t *outLow)
{
    uint32_t highTicks = (period_ticks * duty + 50) / 100; // round to nearest tick

    if (highTicks < AUX_MIN_PHASE_TICKS) highTicks = AUX_MIN_PHASE_TICKS;
    if (highTicks > period_ticks - AUX_MIN_PHASE_TICKS) highTicks = period_ticks - AUX_MIN_PHASE_TICKS;

    *outHigh = highTicks;
    *outLow = period_ticks - highTicks;
}

uint8_t AUX_SwPwm_RoundDuty(uint32_t freq_hz, uint8_t duty)
{
    if (freq_hz == 0 || duty == 0 || duty == 100)
        return duty;

    uint32_t period_ticks = configTICK_RATE_HZ / freq_hz;
    if (period_ticks < 2)
        return duty >= 50 ? 100 : 0; // period too short to represent any in-between duty at all

    uint32_t high, low;
    AUX_ComputeTicks(period_ticks, duty, &high, &low);

    return (uint8_t)((high * 100 + period_ticks / 2) / period_ticks);
}

static void AUX_TimerCallback(TimerHandle_t xTimer)
{
    uint8_t index = (uint8_t)(uint32_t)pvTimerGetTimerID(xTimer);
    AUX_PinControl_t *pin = &auxPins[index];
    AUX_SwPwmState_t *ctx = &auxSwState[index];

    if (ctx->isHigh)
    {
        HAL_GPIO_WritePin(pin->port, pin->pinMask, GPIO_PIN_RESET);
        ctx->isHigh = false;
        xTimerChangePeriod(ctx->timer, ctx->lowTicks, 0);
    }
    else
    {
        HAL_GPIO_WritePin(pin->port, pin->pinMask, GPIO_PIN_SET);
        ctx->isHigh = true;
        xTimerChangePeriod(ctx->timer, ctx->highTicks, 0);
    }
}

static void AUX_SwPwm_Start(uint8_t index, uint32_t period_ticks, uint8_t duty)
{
    AUX_PinControl_t *pin = &auxPins[index];
    AUX_SwPwmState_t *ctx = &auxSwState[index];

    ctx->isHigh = false;

    if (ctx->timer == NULL)
        return;

    xTimerStop(ctx->timer, 0);

    if (duty == 0)
    {
        HAL_GPIO_WritePin(pin->port, pin->pinMask, GPIO_PIN_RESET);
        return;
    }

    if (duty == 100)
    {
        HAL_GPIO_WritePin(pin->port, pin->pinMask, GPIO_PIN_SET);
        return;
    }

    AUX_ComputeTicks(period_ticks, duty, &ctx->highTicks, &ctx->lowTicks);

    // Nastartuj timer rovnou do HIGH fáze
    xTimerChangePeriod(ctx->timer, ctx->highTicks, 0);
    HAL_GPIO_WritePin(pin->port, pin->pinMask, GPIO_PIN_SET);
    ctx->isHigh = true;
    xTimerStart(ctx->timer, 0);
}

static void AUX_SwPwm_Stop(uint8_t index)
{
    AUX_PinControl_t *pin = &auxPins[index];
    AUX_SwPwmState_t *ctx = &auxSwState[index];

    if (ctx->timer != NULL)
    {
        xTimerStop(ctx->timer, 0);
        HAL_GPIO_WritePin(pin->port, pin->pinMask, GPIO_PIN_RESET);
        ctx->isHigh = false;
    }
}

// ---------------------------------------------------------------------------
// HW engine (aux7/aux8): true hardware PWM via HAL, one timer instance per pin
// ---------------------------------------------------------------------------

#define AUX_HW_TIMCLK_HZ 32000000UL // APB clock feeding TIM2/TIM21 (SystemClock_Config)

static TIM_HandleTypeDef auxHtimAux8; // TIM2,  aux8 / PA2, channel 3
static TIM_HandleTypeDef auxHtimAux7; // TIM21, aux7 / PA3, channel 2

static void AUX_HwPwm_InitOne(TIM_HandleTypeDef *htim, TIM_TypeDef *instance, uint32_t channel)
{
    htim->Instance = instance;
    htim->Init.Prescaler = 0;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = AUX_HW_TIMCLK_HZ / 1000 - 1; // arbitrary initial 1kHz, reset on first AUX_StartPWM
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(htim);

    TIM_OC_InitTypeDef ocInit = {0};
    ocInit.OCMode = TIM_OCMODE_PWM1;
    ocInit.Pulse = 0;
    ocInit.OCPolarity = TIM_OCPOLARITY_HIGH;
    ocInit.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(htim, &ocInit, channel);

    HAL_TIM_PWM_Start(htim, channel);
}

static void AUX_HwPwm_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM21_CLK_ENABLE();

    GPIO_InitTypeDef gpioInit = {0};
    gpioInit.Mode = GPIO_MODE_AF_PP;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;

    gpioInit.Pin = aux8_Pin;
    gpioInit.Alternate = GPIO_AF2_TIM2;
    HAL_GPIO_Init(aux8_GPIO_Port, &gpioInit);

    gpioInit.Pin = aux7_Pin;
    gpioInit.Alternate = GPIO_AF0_TIM21;
    HAL_GPIO_Init(aux7_GPIO_Port, &gpioInit);

    AUX_HwPwm_InitOne(&auxHtimAux8, TIM2, TIM_CHANNEL_3);
    AUX_HwPwm_InitOne(&auxHtimAux7, TIM21, TIM_CHANNEL_2);
}

static void AUX_HwPwm_GetHandle(uint8_t index, TIM_HandleTypeDef **htim, uint32_t *channel)
{
    if (index == 7)
    {
        *htim = &auxHtimAux8;
        *channel = TIM_CHANNEL_3;
    }
    else
    {
        *htim = &auxHtimAux7;
        *channel = TIM_CHANNEL_2;
    }
}

static void AUX_HwPwm_Start(uint8_t index, uint32_t freq_hz, uint8_t duty)
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    AUX_HwPwm_GetHandle(index, &htim, &channel);

    if (freq_hz == 0)
        return;

    // Maximize duty resolution: pick the smallest prescaler that still fits ARR in 16 bits.
    uint32_t total = AUX_HW_TIMCLK_HZ / freq_hz;
    if (total < 1) total = 1;
    uint32_t psc = (total + 65535UL) / 65536UL;
    if (psc < 1) psc = 1;
    uint32_t arr = total / psc;
    if (arr < 1) arr = 1;

    __HAL_TIM_SET_PRESCALER(htim, (uint32_t)(psc - 1));
    __HAL_TIM_SET_AUTORELOAD(htim, arr - 1);
    HAL_TIM_GenerateEvent(htim, TIM_EVENTSOURCE_UPDATE); // latch new PSC/ARR immediately

    uint32_t ccr = (duty == 0) ? 0 : (duty >= 100 ? arr : (arr * duty) / 100);
    __HAL_TIM_SET_COMPARE(htim, channel, ccr);

    //LOG PSC, ARR, FREQ
    LOG_INFO("PSC=%lu, ARR=%lu, FREQ=%luHz, CCR=%lu", (unsigned long)psc, (unsigned long)arr, (unsigned long)freq_hz, (unsigned long)ccr);
}

static void AUX_HwPwm_Stop(uint8_t index)
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    AUX_HwPwm_GetHandle(index, &htim, &channel);

    __HAL_TIM_SET_COMPARE(htim, channel, 0);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AUX_InitTimers(void)
{
    for (uint8_t i = 0; i < AUX_PINS_COUNT; i++)
    {
        if (AUX_IsHwPwmPin(i))
            continue;

        if (auxSwState[i].timer == NULL)
        {
            auxSwState[i].timer = xTimerCreate(
                "AUX_PWM",
                pdMS_TO_TICKS(1),     // Dummy hodnota, změní se později
                pdFALSE,
                (void *)(uint32_t)i,  // TimerID = AUX pin index
                AUX_TimerCallback
            );
        }
    }

    AUX_HwPwm_Init();
}

void AUX_StartPWM(uint8_t index, uint32_t freq_hz, uint8_t duty)
{
    if (AUX_IsHwPwmPin(index))
    {
        AUX_HwPwm_Start(index, freq_hz, duty);
        return;
    }

    uint32_t period_ticks = (freq_hz == 0) ? 0 : (configTICK_RATE_HZ / freq_hz);
    if (period_ticks < 1) period_ticks = 1;

    AUX_SwPwm_Start(index, period_ticks, duty);
}

void AUX_StopPWM(uint8_t index)
{
    if (AUX_IsHwPwmPin(index))
    {
        AUX_HwPwm_Stop(index);
        return;
    }

    AUX_SwPwm_Stop(index);
}