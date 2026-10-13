/*
 * RF1_Task.c
 *
 *  Created on: Nov 14, 2023
 *      Author: Uzivatel
 */

#include "main.h"
#include "RF_Task.h"
#include "radio_user.h"
#include "Main_task.h"


extern osMessageQueueId_t queueRadioHandle;

void radio_task_off(radio_context_t *ctx,dataQueue_t *rxd);
void radio_task_on(radio_context_t *ctx,dataQueue_t *rxd);
void (*radio_states[2])(radio_context_t *ctx, dataQueue_t *rxd) = {radio_task_off, radio_task_on};


/**
 * @brief 
 * 
 * @param timer 
 */
static void _RF_HeartBeat_Callback(TimerHandle_t timer)
{	
	UNUSED(timer);
	dataQueue_t txm;
	txm.ptr = NULL;

	txm.cmd = CMD_RF_RADIO_HB;	//TODO nepouzvame
	RT_SendDataToRFTask(&txm, portMAX_DELAY);
}

static void _RF_EventLed_Callback(TimerHandle_t timer)
{
	UNUSED(timer);
	HW_LED_RF_EVENT_OFF();
}

/*
 *
 */
void radio_task_off(radio_context_t *ctx,dataQueue_t *rxd)
{
	dataQueue_t	sd;
	sd.ptr = NULL;

	switch (rxd->cmd)
	{
		case CMD_RF_TURN_ON:

			ru_radio_process_commands(RADIO_CMD_INIT,ctx,rxd);
			ru_radio_process_commands(RADIO_CMD_START_RX, ctx, rxd);

			sd.cmd = CMD_CORE_RF_IS_ON;
			MT_SendDataToMainTask(&sd, portMAX_DELAY);

			ctx->rfTaskState.previousState = ctx->rfTaskState.currentState;
			ctx->rfTaskState.currentState = RF_TASK_ON;

			break;

		case CMD_RF_TURN_OFF:
			ru_radio_process_commands(RADIO_CMD_SLEEP,ctx,rxd);

			sd.cmd = CMD_CORE_RF_IS_OFF;
			MT_SendDataToMainTask(&sd, portMAX_DELAY);

			ctx->rfTaskState.previousState = ctx->rfTaskState.currentState;
			ctx->rfTaskState.currentState = RF_TASK_OFF;
			break;

		default:
			break;
	}
}


/*
 *
 */
void radio_task_on(radio_context_t *ctx, dataQueue_t *rxd)
{
	dataQueue_t	sd;
	sd.ptr = NULL;

	switch (rxd->cmd)
	{
		case CMD_RF_TURN_ON:
			ru_radio_process_commands(RADIO_CMD_INIT,ctx,rxd);
			ru_radio_process_commands(RADIO_CMD_START_RX, ctx, rxd);

			sd.cmd = CMD_CORE_RF_IS_ON;
			MT_SendDataToMainTask(&sd, portMAX_DELAY);

			ctx->rfTaskState.previousState = ctx->rfTaskState.currentState;
			ctx->rfTaskState.currentState = RF_TASK_ON;
			break;

		case CMD_RF_TURN_OFF:
			ru_radio_process_commands(RADIO_CMD_SLEEP,ctx,rxd);

			sd.cmd = CMD_CORE_RF_IS_OFF;
			MT_SendDataToMainTask(&sd, portMAX_DELAY);

			ctx->rfTaskState.previousState = ctx->rfTaskState.currentState;
			ctx->rfTaskState.currentState = RF_TASK_OFF;
			break;

		case CMD_RF_IRQ_FIRED:
			ru_radio_process_IRQ(ctx);
			break;

		case CMD_RF_SEND_DATA_NOW:
			ru_radio_process_commands(RADIO_CMD_SEND_UNIVERSAL_PAYLOAD_NOW,ctx,rxd);
			break;

		case CMD_RF_RADIO_RX_TO_UART:
			if (rxd->data == 1)
			{
				ru_radio_process_commands(RADIO_CMD_START_RX,ctx,rxd);
				ctx->rx_to_uart = true;
			}
			else
			{
				/* code */
				ctx->rx_to_uart = false;
			}
			
			break;

		case CMD_RF_RADIO_RECONFIG_RX:
			ru_radio_process_commands(RADIO_CMD_START_RX,ctx,rxd);
			break;

		case CMD_RF_HB_REQUEST:
			// Respond to heartbeat request from main task
			sd.cmd = CMD_MAIN_HB_RESPONSE_RF;
			MT_SendDataToMainTask(&sd, 0);  // Non-blocking
			break;

		case CMD_RF_TX_CW:
			if (rxd->data == 1)
			{
				// Start TX CW mode
				ru_radio_process_commands(RADIO_CMD_TX_CW, ctx, rxd);
			}
			else
			{
				// Stop TX CW mode - go to standby
				ru_radio_process_commands(RADIO_CMD_STANDBY, ctx, rxd);
			}
			break;

		default:
			break;
	}

}


/**
 * @brief Posle zpravu do fronty RF tasku. Lze volat z tasku i z ISR.
 *
 * @param data    zprava (kopiruje se do fronty)
 * @param timeout max. doba cekani na misto ve fronte (v ISR se ignoruje, nikdy neblokuje)
 * @return true   zprava zarazena do fronty
 * @return false  fronta plna - pokud data->ptr nese alokovanou pamet, musi ji uvolnit volajici
 */
bool RT_SendDataToRFTask(dataQueue_t *data, TickType_t timeout)
{
	if (__get_IPSR())
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		BaseType_t result = xQueueSendFromISR(queueRadioHandle, data, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		return (result == pdTRUE);
	}

	return (xQueueSend(queueRadioHandle, data, timeout) == pdTRUE);
}


/**
 * @brief
 *
 */
void radio_task(void)
{	
	dataQueue_t rxd;
	BaseType_t ret;
	radio_context_t ctx;
	ctx.rfTaskState.currentState = RF_TASK_ON;
	ctx.rfTaskState.previousState = RF_TASK_ON;

	ctx.timers.rfHBTimer.timer = xTimerCreateStatic("RF_TimerHeartBeat", pdMS_TO_TICKS(RF_HEART_BEAT_TIMEOUT_MS),
   	    		pdFALSE, NULL, _RF_HeartBeat_Callback,  &ctx.timers.rfHBTimer.timerPlace);
	ctx.timers.rfEventLedTimer.timer = xTimerCreateStatic("RF_Event_LED", pdMS_TO_TICKS(RF_EVENT_LED_TIMEOUT_MS), 
							pdFALSE, NULL, _RF_EventLed_Callback, &ctx.timers.rfEventLedTimer.timerPlace);

	ru_sx1262_assign(&ctx);

	if(ru_radioInit(&ctx) == false)	_exit(48351);
	ru_radio_start_rx(&ctx);

	for(;;)
	{
		ret = xQueueReceive(queueRadioHandle, &rxd, portMAX_DELAY);
		if (ret == pdPASS)
		{
			radio_states[ctx.rfTaskState.currentState](&ctx, &rxd);
			/* Clear malloc */
			vPortFree(rxd.ptr);
			rxd.ptr=NULL;
		}
	}
}




