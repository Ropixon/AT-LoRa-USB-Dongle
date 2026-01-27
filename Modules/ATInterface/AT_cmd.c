/**
 * @file AT_cmd.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "main.h"
#include "AT_cmd.h"
#include <stdio.h>   // For sprintf
#include <ctype.h>
#include <strings.h>  // For strncasecmp
#include "Main_task.h"
#include "semphr.h"
#include "NVMA.h"


/**
 * @brief 
 * 
 * @param params 
 */
static void AT_HandleFactoryReset(char *params);
static void AT_HandleHelp(char *params);
static void AT_HandleRestartSys(char *params);
static void AT_HandleIdentify(char *params);

extern UART_HandleTypeDef huart1;
extern osMessageQueueId_t queueMainHandle;

AT_cmd_t at_ctx;
SemaphoreHandle_t xUART_TXSemaphore;

/**
 * @brief 
 * 
 */
typedef struct
{
    char *command;
   // void (*handler)(char *params, uint8_t cmdtoCore,uint16_t size);  // Handler s pevnym parametrem
    void (*simpleHandler)(char *params);            // Handler bez pevneho parametru
    eATCommands cmdtoCore;
  //  bool isFixedParamUsed;
    const char *usage;   
    const char *parameters;
} AT_Command_Struct;

// Společná reference table
#define BW_TABLE "BW index: 0=7.81, 1=10.42, 2=15.63, 3=20.83, 4=31.25, 5=41.67, 6=62.5, 7=125, 8=250, 9=500 kHz"
/* Table of AT commands */
const AT_Command_Struct AT_Commands[] = {
    {"AT",                       AT_HandleHelp,          0,                                 "AT - Basic test command",                         ""},
    {"AT+HELP",                  AT_HandleHelp,          0,                                 "AT+HELP - List all supported commands",           ""},
    {"AT+IDENTIFY",              AT_HandleIdentify,      0,                                 "AT+IDENTIFY - Identify the device (returns unique ID)",               ""},
    {"AT+FACTORY_RST",           AT_HandleFactoryReset,  0,                                 "AT+FACTORY_RST - Reset all settings to defaults",      ""},
    {"AT+SYS_RESTART",           AT_HandleRestartSys,    0,                                 "AT+SYS_RESTART - Restart the system",             ""},
    /* single LoRa params*/
    {"AT+LR_TX_FREQ",                NULL,               SYS_CMD_TX_FREQ,                     "AT+LR_TX_FREQ - Set TX frequency",                "=<frequency_in_Hz>, ?"},
    {"AT+LR_RX_FREQ",                NULL,               SYS_CMD_RX_FREQ,                     "AT+LR_RX_FREQ - Set RX frequency",                "=<frequency_in_Hz>, ?"},
    {"AT+LR_TX_POWER",               NULL,               SYS_CMD_TX_POWER,                    "AT+LR_TX_POWER - Set TX power",                   "=<power_in_dBm>, ?"},
    {"AT+LR_TX_SF",                  NULL,               SYS_CMD_TX_SF,                       "AT+LR_TX_SF - Set TX spreading factor",           "=5 to 12, ?"},
    {"AT+LR_RX_SF",                  NULL,               SYS_CMD_RX_SF,                       "AT+LR_RX_SF - Set RX spreading factor",           "=5 to 12, ?"},
    {"AT+LR_TX_BW",                 NULL,               SYS_CMD_TX_BW,                       "AT+LR_TX_BW - Set TX bandwidth",                                 BW_TABLE},
    {"AT+LR_RX_BW",                 NULL,               SYS_CMD_RX_BW,                       "AT+LR_RX_BW - Set RX bandwidth",                                 BW_TABLE},
    {"AT+LR_TX_IQ_INV",              NULL,               SYS_CMD_TX_IQ,                       "AT+LR_TX_IQ_INV - Set TX IQ inversion",           "=1, =0, ?"},
    {"AT+LR_RX_IQ_INV",              NULL,               SYS_CMD_RX_IQ,                       "AT+LR_RX_IQ_INV - Set RX IQ inversion",           "=1, =0, ?"},
    {"AT+LR_TX_CR",                  NULL,               SYS_CMD_TX_CR,                       "AT+LR_TX_CR - Set TX coding rate",                "=45, =46, =47, =48, ?"},
    {"AT+LR_RX_CR",                  NULL,               SYS_CMD_RX_CR,                       "AT+LR_RX_CR - Set RX coding rate",                "=45, =46, =47, =48, ?"},
    {"AT+LR_TX_HEADERMODE",         NULL,               SYS_CMD_HEADERMODE_TX,               "AT+LR_TX_HEADERMODE - Enable TX header mode, explicit = 0",    "=1, =0, ?"},
    {"AT+LR_RX_HEADERMODE",         NULL,               SYS_CMD_HEADERMODE_RX,               "AT+LR_RX_HEADERMODE - Enable RX header mode, explicit = 0",    "=1, =0, ?"},
    {"AT+LR_TX_CRC",                NULL,               SYS_CMD_CRC_TX,                      "AT+LR_TX_CRC - Set TX CRC check",                "=1, =0, ?"},
    {"AT+LR_RX_CRC",                NULL,               SYS_CMD_CRC_RX,                      "AT+LR_RX_CRC - Set RX CRC check",                "=1, =0, ?"},
    {"AT+LR_TX_PREAMBLE_SIZE",      NULL,               SYS_CMD_PREAM_SIZE_TX,               "AT+LR_TX_PREAMBLE_SIZE",                         "=<1 to 65535>, optimum >=8, ?"  },
    {"AT+LR_RX_PREAMBLE_SIZE",      NULL,               SYS_CMD_PREAM_SIZE_RX,               "AT+LR_RX_PREAMBLE_SIZE",                         "=<1 to 65535> should be >= TX side,?"},
    {"AT+LR_TX_LDRO",               NULL,               SYS_CMD_TX_LDRO,                     "AT+LR_TX_LDRO - Set TX Low Data Rate Optimization", "=0 (off), =1 (on), =2 (auto), ?"},
    {"AT+LR_RX_LDRO",               NULL,               SYS_CMD_RX_LDRO,                     "AT+LR_RX_LDRO - Set RX Low Data Rate Optimization", "=0 (off), =1 (on), =2 (auto), ?"},

    {"AT+LR_TX_SYNCWORD",           NULL,               SYS_CMD_TX_SYNCWORD,                  "AT+LR_TX_SYNCWORD - Set TX sync word (0x12 or 0x34)", "=<hex_byte>, ?"},
    {"AT+LR_RX_SYNCWORD",           NULL,               SYS_CMD_RX_SYNCWORD,                  "AT+LR_RX_SYNCWORD - Set RX sync word (0x12 or 0x34)", "=<hex_byte>, ?"},
    {"AT+LR_RX_PLDLEN",             NULL,               SYS_CMD_RX_PLDLEN,                   "AT+LR_RX_PLDLEN - RX payload length (required only for implicit mode)", "=<1-255>, ?"},
 
    /* RF immediate TX commands */
    {"AT+RF_TX_HEX",                NULL,               SYS_CMD_RF_TX_HEX,                   "AT+RF_TX_HEX - Transmit data via RF in HEX format",  "=<HEX data>"},
    {"AT+RF_TX_TXT",                NULL,               SYS_CMD_RF_TX_TXT,                   "AT+RF_TX_TXT - Transmit data via RF in text format", "=<Text data>"},
    
    /* RF saved packet commands */
    {"AT+RF_SAVE_PACKET",           NULL,               SYS_CMD_RF_SAVE_PCKT_NVM,            "AT+RF_SAVE_PACKET - Save packet to memory",       "=<HEX data>, ?"},
    {"AT+RF_TX_SAVED",              NULL,               SYS_CMD_RF_TX_NVM_ONCE,              "AT+RF_TX_SAVED - Send saved packet once",         ""},
    {"AT+RF_TX_SAVED_REPEAT",       NULL,               SYS_CMD_RF_TX_PERIODIC_NVM,          "AT+RF_TX_SAVED_REPEAT - Start/Stop periodic saved packet TX", "=1 (ON), =0 (OFF), ?"},
    {"AT+RF_TX_NVM_PERIOD",         NULL,               SYS_CMD_RF_TX_NVM_PERIOD,            "AT+RF_TX_NVM_PERIOD - Set period for saved packet TX",   "=<period_ms>, ?"},
    {"AT+RF_TX_PERIOD_STATUS",      NULL,               SYS_CMD_RF_PERIOD_STATUS,            "AT+RF_TX_PERIOD_STATUS - Get periodic TX status",     "?"},
    {"AT+RF_TX_CW",                 NULL,               SYS_CMD_TX_CW,                       "AT+RF_TX_CW - Start/Stop TX CW mode", "=1 (ON), =0 (OFF), ?"},

    /* RF RX commands */
    {"AT+RF_RX_TO_UART",            NULL,               SYS_CMD_RF_RX_TO_UART,               "AT+RF_RX_TO_UART - Auto-print received RF data to serial",            "=1 (ON), =0 (OFF), ?"},
    {"AT+RF_RX_FORMAT",             NULL,               SYS_CMD_RX_FORMAT,                   "AT+RF_RX_FORMAT - Set RX output format", "=HEX|ASCII, ?"},
    
    /* RF TOA command */
    {"AT+RF_GET_TOA",               NULL,               SYS_CMD_RF_GET_TOA,                  "AT+RF_GET_TOA - Get TOA (TX config)",             "=<packet_size_bytes>"},
    {"AT+RF_GET_TSYM",              NULL,               SYS_CMD_RF_GET_TSYM,                 "AT+RF_GET_TSYM - Get symbol time in us (TX config)", ""},
    
    /* AUX GPIO commands */
    {"AT+AUX",            NULL, SYS_CMD_AUX_SET,   "AT+AUX - Set AUX pin state", "=<pin:1-8>,<state:0|1>"},
    {"AT+AUX_PULSE",      NULL, SYS_CMD_AUX_PULSE, "AT+AUX_PULSE - Start PWM on AUX pin", "=<pin:1-8>,<period_ms>,<duty%:0-100>"},
    {"AT+AUX_PULSE_STOP", NULL, SYS_CMD_AUX_STOP,  "AT+AUX_PULSE_STOP - Stop PWM on AUX pin", "=<pin:1-8>"},
    /* System commands */
    {"AT+UART_BAUD",                NULL,               SYS_CMD_UART_BAUD,                   "AT+UART_BAUD - Set UART baud rate", "=9600|19200|38400|57600|115200|230400, ?"},
    
    /* multiple LoRa params - set all at once */
    {"AT+LR_TX_SET",                NULL,               SYS_CMD_TX_COMPLETE_SET,             "AT+LR_TX_SET - Set multiple TX params",      "=SF:<5-12>,BW:<0-9>,CR:<45-48>,Freq:<Hz>,IQInv:<0|1>,HeaderMode:<0|1>,CRC:<0|1>,Preamble:<1-65535>,LDRO:<0|1|2>,Power:<dBm>, ?"},
    {"AT+LR_RX_SET",                NULL,               SYS_CMD_RX_COMPLETE_SET,             "AT+LR_RX_SET - Set multiple RX params",      "=SF:<5-12>,BW:<0-9>,CR:<45-48>,Freq:<Hz>,IQInv:<0|1>,HeaderMode:<0|1>,CRC:<0|1>,Preamble:<1-65535>,LDRO:<0|1|2>, ?"}

};



/**
 * @brief 
 * 
 * @param p_at_Ctx 
 */
void AT_Init(AT_cmd_t *p_at_Ctx)
{   
    at_ctx.onDataReceivedFromISR = NULL;

    if(xUART_TXSemaphore == NULL)
    {
        xUART_TXSemaphore = xSemaphoreCreateBinary();
        if (xUART_TXSemaphore != NULL)
        {
            xSemaphoreGive(xUART_TXSemaphore);
        }else
        {
            Error_Handler();
        }
    }
    
    at_ctx.sp_ctx.phuart = p_at_Ctx->sp_ctx.phuart;
    at_ctx.sp_ctx.rxStorage.raw_data = p_at_Ctx->sp_ctx.rxStorage.raw_data;
    at_ctx.sp_ctx.rxStorage.size = p_at_Ctx->sp_ctx.rxStorage.size;
    at_ctx.sp_ctx.txStorage.raw_data = p_at_Ctx->sp_ctx.txStorage.raw_data;
    at_ctx.sp_ctx.txStorage.size = p_at_Ctx->sp_ctx.txStorage.size;

   
    at_ctx.onDataReceivedFromISR = p_at_Ctx->onDataReceivedFromISR;

    SP_PlatformInit(&at_ctx.sp_ctx);

}


/**
 * @brief 
 * 
 * @param data 
 */
static void AT_TrimEndings(char *data)
{
    size_t length = strlen(data);

    while (length > 0 && (data[length - 1] == '\n' || data[length - 1] == '\r'))
    {
        data[length - 1] = '\0';
        length--;
    }
}

/**
 * @brief 
 * 
 * @param data 
 */
void AT_HandleATCommand(uint16_t size)
{   
    //char *data=(char*) sp_ctx->rxStorage.raw_data;
    char *data = (char*) at_ctx.sp_ctx.rxStorage.raw_data;
    bool noParam = false;
    bool isCommand = false;

    AT_TrimEndings(data);

    for (uint16_t i = 0; i < sizeof(AT_Commands) / sizeof(AT_Command_Struct); i++)
    {   
        size_t commandLen = strlen(AT_Commands[i].command);

        if (strncasecmp(data, AT_Commands[i].command, commandLen) == 0 &&
            (data[commandLen] == '\0' ||
             data[commandLen] == '=' ||
             data[commandLen] == '?'))
        {
            char *params = data + commandLen;  // Nastavíme ukazatel za příkaz
            isCommand = true;
            // Pokud je tam `=`, přeskočíme znak `=`, abychom získali parametry
            if (*params == '=')
            {
                params++;
            }
            // Pokud je tam `?`, předáme `?` jako parametr (pro `GET` příkaz)
            else if (*params == '?')
            {
                params = "?";  // Nastavíme `params` přímo na `?`
            }
            else
            {
                // zadny parametr
                noParam= true;
            }

            if(AT_Commands[i].simpleHandler == NULL)
            {      
                if (noParam == true)
                {
                    AT_SendStringResponse("ERROR - Missing parameters\r\n");
                    break;
                }

                if(at_ctx.onDataReceivedFromISR == NULL)
                {
                    AT_SendStringResponse("ERROR - No handler for this command\r\n");
                    break;
                }
                
                if(at_ctx.onDataReceivedFromISR(params,(uint8_t) AT_Commands[i].cmdtoCore,size) == false)
                {
                    AT_SendStringResponse("ERROR - Previous data was not processed yet!\r\n");
                }
                
                //AT_Commands[i].handler(params,AT_Commands[i].cmdtoCore,size);
            }
            else
            {
                AT_Commands[i].simpleHandler(params);
            }
            
        }
    }
    if(isCommand == false)
    {
        AT_SendStringResponse("ERROR - Unknown command\r\n");
    }

    memset(data,0,size);
    SP_RxComplete(&at_ctx.sp_ctx, size);  
   
}

/**
 * @brief 
 * 
 */
void AT_HandleUartError(void)
{   
    SP_HandleUARTError(&at_ctx.sp_ctx);
    AT_SendStringResponse("ERROR - UART error\r\n");
}

/**
 * @brief 
 * 
 * @param params 
 */
static  void AT_HandleFactoryReset(char *params)
{   
    (void)params;  // No parameters needed
    
    // Reset EEPROM to factory defaults
    if (NVMA_FactoryReset())
    {
        AT_SendStringResponse("OK\r\n");
        AT_SendStringResponse("Factory reset complete. Rebooting...\r\n");
        osDelay(100);  // Let message be sent
        NVIC_SystemReset();
    }
    else
    {
        AT_SendStringResponse("ERROR: Factory reset failed\r\n");
    }
}

/**
 * @brief 
 * 
 * @param params 
 */
static void AT_HandleHelp(char *params)
{   
    UNUSED(params);

    AT_SendStringResponse("\r\nSupported AT commands:\r\n");

    // Základní sekce
    AT_SendStringResponse("\r\n--- Basic ---\r\n");
    for (uint16_t i = 0; i < sizeof(AT_Commands) / sizeof(AT_Command_Struct); i++) {
        if (
            strcmp(AT_Commands[i].command, "AT") == 0 ||
            strcmp(AT_Commands[i].command, "AT+HELP") == 0 ||
            strcmp(AT_Commands[i].command, "AT+IDENTIFY") == 0 ||
            strcmp(AT_Commands[i].command, "AT+FACTORY_RST") == 0 ||
            strcmp(AT_Commands[i].command, "AT+SYS_RESTART") == 0 ||
            strcmp(AT_Commands[i].command, "AT+UART_BAUD") == 0
        ) {
            AT_SendStringResponse((char*)AT_Commands[i].usage);
            if (strlen(AT_Commands[i].parameters) > 0) {
                AT_SendStringResponse(" (Par: ");
                AT_SendStringResponse((char*)AT_Commands[i].parameters);
                AT_SendStringResponse(")");
            }
            AT_SendStringResponse("\r\n");
        }
    }

    // LoRa sekce
    AT_SendStringResponse("\r\n--- LoRa Radio ---\r\n");
    for (uint16_t i = 0; i < sizeof(AT_Commands) / sizeof(AT_Command_Struct); i++) {
        if (strstr(AT_Commands[i].command, "LR_") != NULL) {
            AT_SendStringResponse((char*)AT_Commands[i].usage);
            if (strlen(AT_Commands[i].parameters) > 0) {
                AT_SendStringResponse(" (Par: ");
                AT_SendStringResponse((char*)AT_Commands[i].parameters);
                AT_SendStringResponse(")");
            }
            AT_SendStringResponse("\r\n");
        }
    }

    // RF sekce
    AT_SendStringResponse("\r\n--- RF ---\r\n");
    for (uint16_t i = 0; i < sizeof(AT_Commands) / sizeof(AT_Command_Struct); i++) {
        if (strstr(AT_Commands[i].command, "RF_") != NULL) {
            AT_SendStringResponse((char*)AT_Commands[i].usage);
            if (strlen(AT_Commands[i].parameters) > 0) {
                AT_SendStringResponse(" (Par: ");
                AT_SendStringResponse((char*)AT_Commands[i].parameters);
                AT_SendStringResponse(")");
            }
            AT_SendStringResponse("\r\n");
        }
    }

    // AUX sekce
    AT_SendStringResponse("\r\n--- AUX GPIO ---\r\n");
    for (uint16_t i = 0; i < sizeof(AT_Commands) / sizeof(AT_Command_Struct); i++) {
        if (strstr(AT_Commands[i].command, "AUX") != NULL) {
            AT_SendStringResponse((char*)AT_Commands[i].usage);
            if (strlen(AT_Commands[i].parameters) > 0) {
                AT_SendStringResponse(" (Par: ");
                AT_SendStringResponse((char*)AT_Commands[i].parameters);
                AT_SendStringResponse(")");
            }
            AT_SendStringResponse("\r\n");
        }
    }

    // Add examples for complex commands
    AT_SendStringResponse("\r\nExamples for complex commands:\r\n");
    AT_SendStringResponse("  AT+LR_TX_SET=SF:9,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,Power:22,LDRO:2\r\n");
    AT_SendStringResponse("  AT+LR_RX_SET=SF:9,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2\r\n");
}

/**
 * @brief 
 * 
 * @param params 
 */
static void AT_HandleIdentify(char *params)
{
    char uid_str[32];
    uint32_t uid0, uid1, uid2;
    
    UNUSED(params);
    
    // Získání unique device ID z STM32 (96 bitů = 3x 32 bitů)
    uid0 = HAL_GetUIDw0();
    uid1 = HAL_GetUIDw1();
    uid2 = HAL_GetUIDw2();
    
    // Formátování unique ID jako hexadecimální řetězec (96 bitů = 24 hex znaků)
    sprintf(uid_str, "%08lX%08lX%08lX", (unsigned long)uid2, (unsigned long)uid1, (unsigned long)uid0);
    
    // Odeslání odpovědi: Device Name, Version, Build Date a Unique ID (CSV formát pro snadné parsování)
    char info_str[128];
    snprintf(info_str, sizeof(info_str), "%s, v%d.%d.%d, Build: %s %s, UID: ",
             FW_DEVICE_NAME, FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH, __DATE__, __TIME__);
    AT_SendStringResponse(info_str);
    AT_SendStringResponse(uid_str);
    AT_SendStringResponse("\r\n");
}


/**
 * @brief 
 * 
 * @param params 
 */
static void AT_HandleRestartSys(char *params)
{
    UNUSED(params);
    NVIC_SystemReset();
}


/**
 * @brief 
 * 
 * @param response 
 */
void AT_SendStringResponse(char *response)
{   
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (__get_IPSR())  
    {
        // Použití semaforu z ISR
        if (xSemaphoreTakeFromISR(xUART_TXSemaphore, &xHigherPriorityTaskWoken) == pdTRUE)
        {
         //   HAL_UART_Transmit_DMA(&huart1, (uint8_t *)response, strlen(response));
            HAL_UART_Transmit(&huart1, (uint8_t *)response, strlen(response),HAL_MAX_DELAY);
            xSemaphoreGiveFromISR(xUART_TXSemaphore, &xHigherPriorityTaskWoken);
            // Uvolníme semafor při dokončení přenosu (v přerušení)
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        if (xSemaphoreTake(xUART_TXSemaphore, portMAX_DELAY) == pdTRUE)
        {
            HAL_UART_Transmit(&huart1, (uint8_t *)response, strlen(response),HAL_MAX_DELAY);
            xSemaphoreGive(xUART_TXSemaphore);
        }
    }

    //TODOJR DMA
    //HAL_UART_Transmit(&huart1, (uint8_t *)response, strlen(response),0xFFFF);
}
