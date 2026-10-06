/*
 * main_task.c
 *
 *  Created on: Dec 13, 2023
 *      Author: Uzivatel
 */

#include "main.h"
#include "main_task.h"
#include "projdefs.h"
#include "radio_user.h"
#include "semphr.h"
#include "AT_cmd.h"
#include "general_sys_cmd.h"
#include "NVMA.h"
#include "auxPin_logic.h"
#include "iwdg.h"

#define LOG_LEVEL	LOG_LEVEL_NONE
#include "Log.h"
//#include "process_main_task.h"

// Struktura pro uložení statického semaforu
StaticSemaphore_t xSemaphoreBuffer_USART;
SemaphoreHandle_t xBinarySemaphore_USART;

extern UART_HandleTypeDef huart1;
extern osMessageQId queueRadioHandle;
extern osMessageQId queueMainHandle;

void main_task_off(main_ctx_t *ctx,dataQueue_t *rxd);
void main_task_on(main_ctx_t *ctx,dataQueue_t *rxd);

// Callback for periodic TX timer (defined in general_sys_cmd.c)
extern void PeriodicTxTimerCallback(TimerHandle_t xTimer);

volatile uint32_t GL_debug=0;

void (*main_task_states[2])(main_ctx_t *ctx, dataQueue_t *rxd) = {main_task_off, main_task_on};


static uint8_t rxBuffer_USART[MAX_UART_RX_BUFFER];
static uint8_t txBuffer_USART[MAX_UART_RX_BUFFER];

static uint8_t rxShadowBuffer_USART[MAX_UART_RX_BUFFER];

static SemaphoreHandle_t _Main_QueueSemaphore;
static StaticSemaphore_t _Main_QueueSemaphoreBuffer;


/**
 * @brief 
 * 
 * @param xTimer 
 */
static void _Main_Alive_Callback(TimerHandle_t xTimer)
{
    static uint8_t pattern_cnt = 0;

    // Definice patternu (čas v milisekundách) - odpovídá patternu X_X______
    const uint32_t pattern[] = {100,150,100,1000};
    const uint8_t pattern_length = sizeof(pattern) / sizeof(pattern[0]);

    xTimerChangePeriod(xTimer, pdMS_TO_TICKS(pattern[pattern_cnt]), portMAX_DELAY);

    HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);

    //Next step
    pattern_cnt = (pattern_cnt + 1) % pattern_length;
}

/**
 * @brief Construct a new main rx done callback object
 *
 * @param xTimer
 */
static void _Main_RX_AT_done_Callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    HW_LED_AT_RX_OFF();
}


static void GeneralSys_IWDG_Callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    dataQueue_t txm;
    txm.ptr = NULL;
    
    // Send heartbeat request to RF task
    txm.cmd = CMD_RF_HB_REQUEST;
    xQueueSend(queueRadioHandle, &txm, 0);  // Non-blocking
    
    // Notify main task to start collecting responses
    txm.cmd = CMD_MAIN_IWDG_REFRESH;
    xQueueSend(queueMainHandle, &txm, 0);   // Non-blocking
}

/**
 * @brief 
 * 
 * @param data 
 * @param atCmd 
 * @param size 
 * @return true 
 * @return false 
 */
bool AT_CustomCommandHandler(char *data,eATCommands atCmd, uint16_t size)
{
	dataQueue_t txm;
	txm.ptr = NULL;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(xSemaphoreTakeFromISR(xBinarySemaphore_USART, &xHigherPriorityTaskWoken) == pdTRUE)
	{
		memcpy(rxShadowBuffer_USART, data, size);
		txm.cmd = CMD_MAIN_AT_RX_PACKET;
		txm.tmp_8 = (uint8_t) atCmd;
		txm.tmp_16 = size;
		MT_SendDataToMainTask(&txm);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
	{
		return false;
	}

	return true;
}


void AT_SendRfPacketResponse(uint8_t *packet, int16_t rssi, uint16_t length)
{
    uint8_t response[600]={0}; 
    uint8_t rxFormat;

    uint16_t response_size = 0;
    int ret;
    
    // Get configured output format
    NVMA_Get_RX_Format(&rxFormat);

    // Přidání prefixu a délky
    ret = snprintf((char *)&response[response_size], sizeof(response) - response_size, "+RX:%u,", length);
    if (ret < 0 || ret >= (int)(sizeof(response) - response_size))
    {
        // Chyba při formátování nebo nedostatek místa v bufferu
        return;
    }
    response_size += ret;

    if (rxFormat == RX_FORMAT_ASCII)
    {
        // ASCII format - copy printable chars, replace non-printable with '.'
        for (uint16_t i = 0; i < length; i++)
        {
            if (response_size + 1 >= (uint16_t)sizeof(response))
            {
                return;
            }
            uint8_t byte = packet[i];
            // Print printable ASCII chars (32-126), replace others with '.'
            response[response_size++] = (byte >= 32 && byte <= 126) ? byte : '.';
        }
    }
    else
    {
        // HEX format (default)
        static const char hex_digits[] = "0123456789ABCDEF";
        for (uint16_t i = 0; i < length; i++)
        {
            if (response_size + 2 >= (uint16_t)sizeof(response))
            {
                return;
            }
            uint8_t byte = packet[i];
            response[response_size++] = hex_digits[(byte >> 4) & 0x0F];
            response[response_size++] = hex_digits[byte & 0x0F];
        }
    }

    // Přidání RSSI na konec
    ret = snprintf((char *)&response[response_size], sizeof(response) - response_size, ",RSSI:%d", rssi);
    if (ret < 0 || ret >= (int)(sizeof(response) - response_size))
    {
        // Chyba nebo nedostatek místa v bufferu
        return;
    }
    response_size += ret;

    // Přidání odřádkování
    if ((unsigned int)response_size + 2 >= sizeof(response))
    {
        // Nedostatek místa v bufferu
        return;
    }
    response[response_size++] = '\r';
    response[response_size++] = '\n';

    // Odeslání odpovědi
    AT_SendStringResponse((char *)response);
}




/**
 * @brief 
 * 
 * @param ctx 
 * @param rxd 
 */
void main_task_off(main_ctx_t *ctx,dataQueue_t *rxd)
{
    (void)ctx;
    (void)rxd;
}



/**
 * @brief 
 * 
 * @param ctx 
 * @param rxd 
 */
void main_task_on(main_ctx_t *ctx, dataQueue_t *rxd)
{
	packet_info_t	*rx_pkt;
	bool 	AtCmdProcessed;

	switch (rxd->cmd)
	{
		case CMD_MAIN_RF_RX_PACKET:
			rx_pkt = rxd->ptr;
			
			AT_SendRfPacketResponse(rx_pkt->packet, rx_pkt->rx_rssi,rx_pkt->size);
			vPortFree(rx_pkt->packet);
			rx_pkt->packet=NULL;

			break;

        case CMD_MAIN_RF_TX_DONE:
            // Asynchronni notifikace o dokonceni vysilani
            AT_SendStringResponse("TX:DONE\r\n");

            // Kontrola zda je aktivní periodické vysílání (timer ID != NULL)
            if (pvTimerGetTimerID(ctx->timers.Periodic_RF_TX.timer) != NULL)
            {
                // Periodické TX je aktivní, restart timeru pro další vysílání
                // Perioda se načítá z NVM při každém cyklu (umožňuje změnu periody za běhu)
                uint32_t period;
                NVMA_Get_LR_TX_Period_TX(&period);
                xTimerChangePeriod(ctx->timers.Periodic_RF_TX.timer,
                                  pdMS_TO_TICKS(period), 0);
                xTimerStart(ctx->timers.Periodic_RF_TX.timer, 0);
            }
            // Pokud je timer ID NULL, periodické TX bylo zastaveno - nedělat nic
            break;

		case CMD_MAIN_AT_RX_PACKET:
			AtCmdProcessed = GSC_ProcessCommand((eATCommands) rxd->tmp_8, rxShadowBuffer_USART, rxd->tmp_16);
            if(AtCmdProcessed)
            {
                HW_LED_AT_RX_ON();
                osTimerStart(ctx->timers.LED_AT_RX_done.timer, pdMS_TO_TICKS(100));
            }
            
			xSemaphoreGive(xBinarySemaphore_USART);

			break;

		case CMD_MAIN_IWDG_REFRESH:
			// Start heartbeat pending, clear previous responses
			ctx->heartbeat.hb_pending = true;
			ctx->heartbeat.rf_task_alive = false;
			break;

		case CMD_MAIN_HB_RESPONSE_RF:
			// RF task responded
			ctx->heartbeat.rf_task_alive = true;
			
			// Check if all tasks responded
			if (ctx->heartbeat.hb_pending && ctx->heartbeat.rf_task_alive)
			{
				// All tasks alive - refresh IWDG
				refresh_iwdg();
				
				// Clear flags and restart timer for next heartbeat cycle
				ctx->heartbeat.hb_pending = false;
				ctx->heartbeat.rf_task_alive = false;
				xTimerStart(ctx->timers.IWDG_timer.timer, 0);
			}
			break;

		default:
			break;
	}

}

/**
 * @brief 
 * 
 * @param data 
 * @return true 
 * @return false 
 */
bool MT_SendDataToMainTask(dataQueue_t *data)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result;

    // Zkontrolujeme, zda je voláno z ISR
    if (__get_IPSR())
    {
        // Pokud jsme v ISR, pokusíme se získat semafor bez blokování
        if (xSemaphoreTakeFromISR(_Main_QueueSemaphore, &xHigherPriorityTaskWoken) == pdTRUE)
        {
            // Semafor je volný, můžeme poslat data do fronty
            result = xQueueSendFromISR(queueMainHandle, data, &xHigherPriorityTaskWoken);

            // Uvolníme semafor pro další přístup
            xSemaphoreGiveFromISR(_Main_QueueSemaphore, &xHigherPriorityTaskWoken);

            // Přepneme kontext, pokud je to nutné
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else
        {
            // Semafor nebyl volný - fronta je momentálně obsazena
            return false;
        }
    }
    else
    {
        // Mimo ISR - čekáme na získání semaforu
        if (xSemaphoreTake(_Main_QueueSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Semafor je získán, posíláme data do fronty
            result = xQueueSend(queueMainHandle, data, portMAX_DELAY);

            // Uvolníme semafor
            xSemaphoreGive(_Main_QueueSemaphore);
        }
        else
        {
            // Semafor nebyl získán, vracíme chybu
            return false;
        }
    }

    return (result == pdTRUE);
}


/**
 * @brief 
 * 
 */
void main_task(void)
{
	dataQueue_t	rxd;
	BaseType_t ret;
	main_ctx_t ctx;
	ctx.task_state = MAIN_TASK_ON;
	ctx.heartbeat.hb_pending = false;
	ctx.heartbeat.rf_task_alive = false;

	// Initialize all LEDs (turn off)
	HW_LED_INIT();

	_Main_QueueSemaphore = xSemaphoreCreateMutexStatic(&_Main_QueueSemaphoreBuffer);

    ctx.timers.LED_alive.timer = xTimerCreateStatic("LED alive timer", pdMS_TO_TICKS(100), pdFALSE, NULL, 
                                                         _Main_Alive_Callback,  &ctx.timers.LED_alive.timerPlace);

    ctx.timers.LED_AT_RX_done.timer = xTimerCreateStatic("LED RX done", pdMS_TO_TICKS(100), pdFALSE, NULL, 
                                                        _Main_RX_AT_done_Callback,  &ctx.timers.LED_AT_RX_done.timerPlace);

	osTimerStart(ctx.timers.LED_alive.timer, pdMS_TO_TICKS(100));

	/* init and start recieve*/
	if (xBinarySemaphore_USART == NULL)
    {
        // Vytvoření semaforu
        xBinarySemaphore_USART = xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer_USART);

        if (xBinarySemaphore_USART != NULL)
        {
            xSemaphoreGive(xBinarySemaphore_USART);
        }
    }

	NVMA_Init();
	
	// Initialize EEPROM with defaults if not already done
	if (!NVMA_InitDefaults())
	{
	    LOG_ERROR("EEPROM initialization failed!");
	}

	AT_cmd_t at_ctx;
	at_ctx.sp_ctx.rxStorage.raw_data = rxBuffer_USART;
	at_ctx.sp_ctx.rxStorage.size = MAX_UART_RX_BUFFER;
	at_ctx.sp_ctx.txStorage.raw_data = txBuffer_USART;
	at_ctx.sp_ctx.txStorage.size = MAX_UART_TX_BUFFER;
	at_ctx.sp_ctx.phuart = &huart1;
	// Assign the custom command handler to be called when data is received from ISR
	at_ctx.onDataReceivedFromISR = AT_CustomCommandHandler;
    AT_Init(&at_ctx);

    AUX_InitTimers();
    
	LOG_DEBUG("Main task started");

    uint8_t rxUart;
    dataQueue_t txm;
    txm.ptr = NULL;
    NVMA_Get_RX_TO_UART(&rxUart);
    
    txm.cmd = CMD_RF_RADIO_RX_TO_UART;
    txm.data = rxUart;
    xQueueSend(queueRadioHandle,&txm,portMAX_DELAY);

    /* create timer for iwdg (one-shot, will be restarted after successful HB collection) */
    ctx.timers.IWDG_timer.timer = xTimerCreateStatic("IWDG timer", pdMS_TO_TICKS(1000), pdFALSE, NULL, 
                                                         GeneralSys_IWDG_Callback,  &ctx.timers.IWDG_timer.timerPlace);
    
    ctx.timers.Periodic_RF_TX.timer = xTimerCreateStatic("RF Periodic timer", pdMS_TO_TICKS(1000), pdFALSE, NULL,
     PeriodicTxTimerCallback, &ctx.timers.Periodic_RF_TX.timerPlace);

    // Nastavení timer handle pro Start/StopPeriodicTx() v general_sys_cmd.c
    GSC_SetPeriodicTxTimer(ctx.timers.Periodic_RF_TX.timer);

    xTimerStart(ctx.timers.IWDG_timer.timer, 0);

	for(;;)
	{
		ret = xQueueReceive(queueMainHandle, &rxd, portMAX_DELAY);
		if (ret == pdPASS)
		{
			main_task_states[ctx.task_state](&ctx, &rxd);

			/* Clear malloc */
			vPortFree(rxd.ptr);
			rxd.ptr=NULL;
		}

	}
}

