/**
 * @file general_sys_cmd.c
 * @brief General system command processing
 * @version 0.1
 * @date 2024-11-11
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "main.h"
#include "Main_task.h"
#include "general_sys_cmd.h"
#include "NVMA.h"
#include <string.h>
#include <strings.h>  // For strcasecmp
#include <stdlib.h>
#include <stdio.h>
#include "AT_cmd.h"
#include "radio_user.h"
#include <errno.h>
#include "auxPin_logic.h"

#define RESPONSE_BUFF_SIZE  32

const AT_CommandLimit_t AT_CommandLimits[] = {
    {SYS_CMD_TX_FREQ, 150000000, 960000000, 9},    // TX frequency in Hz (100 MHz to 960 MHz, max 9 znaků)
    {SYS_CMD_RX_FREQ, 150000000, 960000000, 9},    // RX frequency in Hz (100 MHz to 960 MHz, max 9 znaků)
    {SYS_CMD_TX_POWER, 0, 22, 2},                 // TX power in dBm (0 to 22, max 2 znaky)
    {SYS_CMD_TX_SF, 5, 12, 2},                     // TX spreading factor (5 to 12, max 2 znaky)
    {SYS_CMD_RX_SF, 5, 12, 2},                     // RX spreading factor (5 to 12, max 2 znaky)
    {SYS_CMD_TX_BW, 0, 9, 1},                      // TX bandwidth (0-9, max 1 znak)
    {SYS_CMD_RX_BW, 0, 9, 1},                      // RX bandwidth (0-9, max 1 znak)
    {SYS_CMD_TX_IQ, 0, 1, 1},                      // TX IQ inversion (0 = FALSE, 1 = TRUE, max 1 znak)
    {SYS_CMD_RX_IQ, 0, 1, 1},                      // RX IQ inversion (0 = FALSE, 1 = TRUE, max 1 znak)
    {SYS_CMD_TX_CR, 45, 48, 2},                    // TX coding rate (45 to 48, max 2 znaky)
    {SYS_CMD_RX_CR, 45, 48, 2},                    // RX coding rate (45 to 48, max 2 znaky)
    {SYS_CMD_HEADERMODE_TX, 0, 1, 1},              // TX header mode (0 = FALSE, 1 = TRUE, max 1 znak)
    {SYS_CMD_HEADERMODE_RX, 0, 1, 1},              // RX header mode (0 = FALSE, 1 = TRUE, max 1 znak)
    {SYS_CMD_CRC_TX, 0, 1, 1},                     // TX CRC check (0 = FALSE, 1 = TRUE, max 1 znak)
    {SYS_CMD_CRC_RX, 0, 1, 1},                     // RX CRC check (0 = FALSE, 1 = TRUE, max 1 znak)
    {SYS_CMD_PREAM_SIZE_TX, 1, 65535, 5},          // TX preamble size (1 to 65535, max 5 znaků)
    {SYS_CMD_PREAM_SIZE_RX, 1, 65535, 5},          // RX preamble size (1 to 65535, max 5 znaků)
    {SYS_CMD_RF_TX_HEX, 0, 255, 512},              // RF TX HEX (max 512 znaků pro HEX data)
    {SYS_CMD_RF_TX_TXT, 0, 255, 512},              // RF TX TXT (max 512 znaků pro textová data)
    {SYS_CMD_RF_TX_NVM_PERIOD, 1, 65535, 5},       // RF NVM packet periodic TX period (1 to 65535 ms, max 5 znaků)
    {SYS_CMD_RF_TX_PERIODIC_NVM, 0, 0, 3},         // RF periodic NVM packet TX control (ON/OFF, max 3 znaky)
    {SYS_CMD_RF_SAVE_PCKT_NVM, 0, 255, 512},       // Save RF packet to NVM (max 512 znaků pro HEX data)
    {SYS_CMD_RF_TX_NVM_ONCE, 1, 1, 1},             // Transmit saved NVM packet once (1, max 1 znak)
    {SYS_CMD_RF_PERIOD_STATUS, 0, 1, 1}            // Get periodic TX status (0 = FALSE, 1 = TRUE, max 1 znak)
};



extern osMessageQueueId_t queueRadioHandle;

// Timer pro debouncing RX rekonfigurace
static TimerHandle_t rxReconfigTimer = NULL;
#define RX_RECONFIG_DELAY_MS 50  // Prodleva před odesláním rekonfigurace

// Timer handle pro periodické RF vysílání
static TimerHandle_t periodicTxTimer = NULL;


const uint32_t AllowedBandwidths[] = {7810, 10420, 15630, 20830, 31250, 41670, 62500, 125000, 250000, 500000};
const size_t AllowedBandwidthCount = sizeof(AllowedBandwidths) / sizeof(AllowedBandwidths[0]);

static void _GSC_Handle_TX(uint8_t *data, uint8_t size);
static bool GetCommandLimits(eATCommands cmd, int32_t *minValue, int32_t *maxValue, size_t *maxLength);
static uint8_t HexStringToByteArray(const char *hexStr, uint8_t *byteArray, size_t byteArraySize);
static void ByteArrayToHexString(const uint8_t *byteArray, size_t arraySize, char *hexStr, size_t hexStrSize);
static bool _GSC_Handle_RX_TO_UART(uint8_t *data, uint8_t size);
static bool _GSC_Handle_AUX_PIN_PWM(uint8_t *data, uint8_t size);
static bool _GSC_Handle_AUX_PIN_SET(uint8_t *data, uint8_t size);
static bool _GSC_Handle_AUX_STOP(uint8_t *data, uint8_t size);
static void RxReconfigTimerCallback(TimerHandle_t xTimer);
// static void TriggerRxReconfig(void);  // Currently unused
void PeriodicTxTimerCallback(TimerHandle_t xTimer);  // Non-static - used in Main_task.c
static void StartPeriodicTx(void);
static void StopPeriodicTx(void);
static bool ParseBoolValue(const char *data, uint8_t *value);

/**
 * @brief Parse boolean value accepting both "1"/"0" and "ON"/"OFF" formats
 *
 * @param data Input string
 * @param value Output: 0 or 1
 * @return true if successfully parsed, false otherwise
 */
static bool ParseBoolValue(const char *data, uint8_t *value)
{
    if (strcmp(data, "1") == 0 || strcasecmp(data, "ON") == 0)
    {
        *value = 1;
        return true;
    }
    else if (strcmp(data, "0") == 0 || strcasecmp(data, "OFF") == 0)
    {
        *value = 0;
        return true;
    }
    return false;
}

/**
 * @brief Parse data to uint32_t
 * 
 * @param data 
 * @return uint32_t 
 */
size_t AT_ParseUint32(const uint8_t *data, uint32_t *value, size_t maxLength)
{
    uint32_t result = 0;
    const uint8_t *ptr = data;

    if (*ptr == '\0')
    {
        return 0; // Prázdný řetězec
    }

    size_t len = 0;

    while (*ptr != '\0')
    {
        if (*ptr < '0' || *ptr > '9')
        {
            return 0; // Nečíselný znak
        }

        uint8_t digit = *ptr - '0';

        if (result > (UINT32_MAX - digit) / 10)
        {
            return 0; // Přetečení
        }

        result = result * 10 + digit;
        ptr++;
        len++;
    }

    if (len > maxLength)
    {
        return 0; // Délka přesáhla limit
    }

    *value = result;
    return len;
}


size_t AT_ParseUint16(const uint8_t *data, uint16_t *value, size_t maxLength)
{
    uint16_t result = 0;
    const uint8_t *ptr = data;

    if (*ptr == '\0')
    {
        return 0; // Prázdný řetězec
    }

    size_t len = 0;

    while (*ptr != '\0')
    {
        if (*ptr < '0' || *ptr > '9')
        {
            return 0; // Nečíselný znak
        }

        uint8_t digit = *ptr - '0';

        if (result > (UINT16_MAX - digit) / 10)
        {
            return 0; // Přetečení
        }

        result = result * 10 + digit;
        ptr++;
        len++;
    }

    if (len > maxLength)
    {
        return 0; // Délka přesáhla limit
    }

    *value = result;
    return len;
}


/**
 * @brief Parse data to uint8_t
 * 
 * @param data 
 * @return uint8_t 
 */
size_t AT_ParseUint8(const uint8_t *data, uint8_t *value, size_t maxLength)
{
    uint32_t result = 0;
    const uint8_t *ptr = data;

    if (*ptr == '\0')
    {
        return 0; // Prázdný řetězec
    }

    size_t len = 0;

    while (*ptr != '\0')
    {
        if (*ptr < '0' || *ptr > '9')
        {
            return 0; // Nečíselný znak
        }

        uint8_t digit = *ptr - '0';

        if (result > ((uint32_t)UINT8_MAX - digit) / 10)
        {
            return 0; // Přetečení
        }

        result = result * 10 + digit;
        ptr++;
        len++;
    }

    if (len > maxLength)
    {
        return 0; // Délka přesáhla limit
    }

    *value = (uint8_t)result;
    return len;
}


/**
 * @brief Timer callback pro odeslání RX rekonfigurace
 * 
 * @param xTimer 
 */
static void RxReconfigTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    dataQueue_t txm;
    txm.ptr = NULL;
    txm.cmd = CMD_RF_RADIO_RECONFIG_RX;
    xQueueSend(queueRadioHandle, &txm, portMAX_DELAY);
}


/**
 * @brief Callback funkce pro periodický TX timer
 * 
 * @param xTimer 
 */
void PeriodicTxTimerCallback(TimerHandle_t xTimer)
{
    // Kontrola zda nebyl timer zastaven (callback mohl být ve frontě)
    // Timer ID se mění okamžitě (ne přes frontu), takže vidíme aktuální stav
    if (pvTimerGetTimerID(xTimer) == NULL)
    {
        return;
    }
    
    uint8_t packet[256];
    uint16_t packetSize;
    
    // Načtení uloženého paketu z NVM
    NVMA_Get_LR_Saved_Pckt_Size(&packetSize);
    NVMA_Get_LR_TX_RF_PCKT(packet, packetSize);
    
    // Využití existující funkce pro odeslání
    _GSC_Handle_TX(packet, packetSize);
}

/**
 * @brief Nastaví handle timeru pro periodické vysílání
 *
 * Tato funkce je volána z Main_task.c po vytvoření timeru,
 * aby byl timer handle dostupný pro Start/StopPeriodicTx() funkce.
 *
 * @param timer Handle timeru z FreeRTOS
 */
void GSC_SetPeriodicTxTimer(TimerHandle_t timer)
{
    periodicTxTimer = timer;
}

/**
 * @brief Spustí periodické RF vysílání
 *
 * Načte periodu z NVM a spustí timer. Pokud timer již běží,
 * restartuje ho s novou periodou z NVM.
 */
static void StartPeriodicTx(void)
{
    if (periodicTxTimer == NULL)
    {
        AT_SendStringResponse("ERROR: Periodic TX timer not initialized\r\n");
        return;
    }

    // Načtení periody z NVM
    uint32_t period;
    NVMA_Get_LR_TX_Period_TX(&period);

    // Nastavení timer ID na non-NULL (active flag)
    // Pokud již běží, restart s novou periodou z NVM
    vTimerSetTimerID(periodicTxTimer, (void*)1);

    // Změna periody a start/restart timeru
    xTimerChangePeriod(periodicTxTimer, pdMS_TO_TICKS(period), 0);
    xTimerStart(periodicTxTimer, 0);

    AT_SendStringResponse("OK: Periodic TX started\r\n");
}

/**
 * @brief Zastaví periodické RF vysílání
 *
 * Nastaví timer ID na NULL (callback se tím pádem přeskočí)
 * a zastaví timer.
 */
static void StopPeriodicTx(void)
{
    if (periodicTxTimer == NULL)
    {
        AT_SendStringResponse("ERROR: Periodic TX timer not initialized\r\n");
        return;
    }

    // Nastavení timer ID na NULL (stopped flag) - callback toto zkontroluje
    vTimerSetTimerID(periodicTxTimer, NULL);

    // Zastavení timeru
    xTimerStop(periodicTxTimer, 0);

   // AT_SendStringResponse("OK: Periodic TX stopped\r\n");
}


/**
 * @brief 
 * 
 * @param tx 
 * @param params 
 */
void ProcessRFMultiSetCommand(bool tx, char *params)
{      
   bool isQuery = (params[0] == '?' && params[1] == '\0');

    // Seznam příkazů k iteraci
    eATCommands commands[] = {
        tx ? SYS_CMD_TX_SF : SYS_CMD_RX_SF,
        tx ? SYS_CMD_TX_BW : SYS_CMD_RX_BW,
        tx ? SYS_CMD_TX_CR : SYS_CMD_RX_CR,
        tx ? SYS_CMD_TX_FREQ : SYS_CMD_RX_FREQ,
        tx ? SYS_CMD_TX_IQ : SYS_CMD_RX_IQ,
        tx ? SYS_CMD_HEADERMODE_TX : SYS_CMD_HEADERMODE_RX,
        tx ? SYS_CMD_CRC_TX : SYS_CMD_CRC_RX,
        tx ? SYS_CMD_PREAM_SIZE_TX : SYS_CMD_PREAM_SIZE_RX,
        tx ? SYS_CMD_TX_LDRO : SYS_CMD_RX_LDRO,
        tx ? SYS_CMD_TX_POWER : SYS_CMD_TX_POWER
    };

    const char *keys[] = {
        "SF", "BW", "CR", "FREQ", "IQINV", "HEADERMODE", "CRC", "PREAMBLE", "LDRO", "POWER"
    };

    //Pokud jde o dotaz
    if (isQuery)
    {
        // Iteruj přes všechny příkazy a volej `GSC_ProcessCommand`
        for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        {   
            AT_SendStringResponse((char *)keys[i]);
            AT_SendStringResponse(":");

             // Získej hodnotu pomocí `GSC_ProcessCommand`
            if (GSC_ProcessCommand(commands[i], (uint8_t *)"?", 1))
            {
                // Hodnota už je vypsána přímo v `GSC_ProcessCommand`
            }
            else
            {
                AT_SendStringResponse("ERROR");
            }

            AT_SendStringResponse("\r\n"); // Přidání nového řádku pro čitelnost
        }
        return;
    }

    char *token = strtok(params, ",");

    char errorMessages[256] = ""; // Pole pro chybové zprávy
    size_t errorMessagesLen = 0;

    while (token != NULL)
    {
        char *key = token;
        char *value_str = strchr(token, ':');
        if (value_str != NULL)
        {
            *value_str = '\0'; // Ukončíme řetězec klíče
            value_str++; // Posuneme ukazatel na hodnotu
        }
        else
        {
            break;
        }

        eATCommands cmd = SYS_CMD_NONE;

        if (strcasecmp(key, "SF") == 0)
        {
            if(tx == true)    cmd = SYS_CMD_TX_SF;
            else              cmd = SYS_CMD_RX_SF;
            // Případně ověření hodnoty
        }
        else if (strcasecmp(key, "BW") == 0)
        {
            if(tx == true) cmd = SYS_CMD_TX_BW;
            else           cmd = SYS_CMD_RX_BW;
            // Případně ověření hodnoty
        }
        else if (strcasecmp(key, "CR") == 0)
        {
            if(tx == true) cmd = SYS_CMD_TX_CR;
            else           cmd = SYS_CMD_RX_CR;
            // Případně ověření hodnoty
        }
        else if (strcasecmp(key, "POWER") == 0)
        {
            if(tx == true) cmd = SYS_CMD_TX_POWER;
            // Případně ověření hodnoty
        }
        else if (strcasecmp(key, "FREQ") == 0)
        {
            if(tx == true) cmd = SYS_CMD_TX_FREQ;
            else           cmd = SYS_CMD_RX_FREQ;
            // Případně ověření hodnoty
        }
        else if (strcasecmp(key, "IQINV") == 0)
        {
            if(tx == true) cmd = SYS_CMD_TX_IQ;
            else           cmd = SYS_CMD_RX_IQ;
        
        }
        else if (strcasecmp(key, "HEADERMODE") == 0)
        {
            if(tx == true) cmd = SYS_CMD_HEADERMODE_TX;
            else           cmd = SYS_CMD_HEADERMODE_RX;

        }
        else if (strcasecmp(key, "CRC") == 0)
        {
            if(tx == true) cmd = SYS_CMD_CRC_TX;
            else           cmd = SYS_CMD_CRC_RX;
           
        }
        else if (strcasecmp(key, "PREAMBLE") == 0)
        {
            if(tx == true) cmd = SYS_CMD_PREAM_SIZE_TX;
            else           cmd = SYS_CMD_PREAM_SIZE_RX;
        }
        else if (strcasecmp(key, "LDRO") == 0)
        {
            if(tx == true) cmd = SYS_CMD_TX_LDRO;
            else           cmd = SYS_CMD_RX_LDRO;
        }
        else
        {
            AT_SendStringResponse("ERROR: Unknown parameter\r\n");
            break;
        }

        // Předáme value_str přímo do GSC_ProcessCommand
        if (!GSC_ProcessCommand(cmd, (uint8_t *)value_str, strlen(value_str)))
        {   
            if (errorMessagesLen < sizeof(errorMessages) - 1)
            {
                strncat(errorMessages, "Failed to set parameter: ", sizeof(errorMessages) - errorMessagesLen - 1);
                strncat(errorMessages, key, sizeof(errorMessages) - errorMessagesLen - 1);
                strncat(errorMessages, "; ", sizeof(errorMessages) - errorMessagesLen - 1);
                errorMessagesLen = strlen(errorMessages);
            }
        }

        // Další token
        token = strtok(NULL, ",");
    }

}




/**
 * @brief Return uint32_t value as a string
 * 
 * @param value 
 * @param response 
 * @param response_size 
 */
static void AT_FormatUint32Response(uint32_t value, uint8_t *response, uint16_t *response_size)
{
    *response_size = snprintf((char *)response, RESPONSE_BUFF_SIZE, "%lu", (unsigned long) value);
}

static void AT_FormatUint16Response(uint16_t value, uint8_t *response, uint16_t *response_size)
{
    *response_size = snprintf((char *)response, RESPONSE_BUFF_SIZE, "%u", (unsigned int) value);
}


/**
 * @brief Return uint8_t value as a string
 * 
 * @param value 
 * @param response 
 * @param response_size 
 */
static void AT_FormatUint8Response(uint8_t value, uint8_t *response, uint16_t *response_size)
{   
    *response_size = snprintf((char *)response, RESPONSE_BUFF_SIZE, "%u", value);
    //*response_size = sprintf((char *)response, "%u", value);
}

/**
 * @brief Get command limits
 * 
 * @param cmd 
 * @param minValue 
 * @param maxValue 
 * @return true 
 * @return false 
 */
static bool GetCommandLimits(eATCommands cmd, int32_t *minValue, int32_t *maxValue, size_t *maxLength)
{
    for (uint16_t i = 0; i < sizeof(AT_CommandLimits) / sizeof(AT_CommandLimits[0]); i++)
    {
        if (AT_CommandLimits[i].cmd == cmd)
        {
            if (minValue) *minValue = AT_CommandLimits[i].minValue;
            if (maxValue) *maxValue = AT_CommandLimits[i].maxValue;
            if (maxLength) *maxLength = AT_CommandLimits[i].maxLength;
            return true;
        }
    }
    return false; // Pokud nebyl příkaz nalezen
}


/**
 * @brief Convert hex string to uint8_t array
 * 
 * @param hexStr 
 * @param byteArray 
 * @param byteArraySize 
 * @return true 
 * @return false 
 */
static uint8_t HexStringToByteArray(const char *hexStr, uint8_t *byteArray, size_t byteArraySize)
{
    size_t hexStrLen = strlen(hexStr);
    if (hexStrLen % 2 != 0 || hexStrLen / 2 > byteArraySize)
    {
        return 0;
    }

    for (size_t i = 0; i < hexStrLen; i += 2)
    {
        char byteStr[3] = {hexStr[i], hexStr[i + 1], '\0'};
        byteArray[i / 2] = (uint8_t)strtoul(byteStr, NULL, 16);
    }

    return (uint8_t)(hexStrLen / 2);
}

/**
 * @brief Convert uint8_t array to hex string
 * 
 * @param byteArray 
 * @param arraySize 
 * @param hexStr 
 * @param hexStrSize 
 */
static void ByteArrayToHexString(const uint8_t *byteArray, size_t arraySize, char *hexStr, size_t hexStrSize)
{
    if (hexStrSize < (arraySize * 2 + 1))
    {
        return; // Nedostatečná velikost bufferu
    }

    for (size_t i = 0; i < arraySize; i++)
    {
        snprintf(&hexStr[i * 2], 3, "%02X", byteArray[i]);
    }
    hexStr[arraySize * 2] = '\0';
}

/**
 * @brief Process system command
 * 
 * @param cmd 
 * @param data 
 * @param size 
 * @return true 
 * @return false 
 */
bool GSC_ProcessCommand(eATCommands cmd, uint8_t *data, uint16_t size)
{
    UNUSED(size);
    char response[RESPONSE_BUFF_SIZE];
    uint16_t response_size = 0;
    bool isQuery = (data[0] == '?');
    bool hasResponse = false;
    bool commandHandled = true;
    bool constrained = false;
    size_t maxLength;
    bool reconfigure_rx = false;

    int32_t minValue, maxValue;

    switch (cmd)
    {   
        case SYS_CMD_TX_FREQ:
        {   
            uint32_t freq;
            if (isQuery)
            {
                NVMA_Get_LR_Freq_TX(&freq);
                AT_FormatUint32Response(freq, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (AT_ParseUint32(data, &freq,maxLength) == 0)
                {
                    AT_SendStringResponse("ERROR: Invalid TX_FREQ value\r\n");
                    commandHandled = false;
                    break;
                }

               
                freq = Constrain_u32(freq, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: TX_FREQ value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_Freq_TX(freq);
            }
            break;
        }

        case SYS_CMD_RX_FREQ:
        {
            uint32_t freq;
            if (isQuery)
            {
                NVMA_Get_LR_Freq_RX(&freq);
                AT_FormatUint32Response(freq, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint32(data, &freq,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid RX_FREQ value\r\n");
                    commandHandled = false;
                    break;
                }
              
                freq = Constrain_u32(freq, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: RX_FREQ value out of limit\r\n");
                    commandHandled = false;
                    break;
                }

                NVMA_Set_LR_Freq_RX(freq);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_TX_POWER:
        {
            uint8_t power;
            if (isQuery)
            {
                NVMA_Get_LR_TX_Power(&power);
                AT_FormatUint8Response(power, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &power,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid TX_POWER value\r\n");
                    commandHandled = false;
                    break;
                }
                
                power = Constrain_u8(power, minValue, maxValue, &constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: TX_POWER value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_TX_Power(power);
            }
            break;
        }

        case SYS_CMD_TX_SF:
        {   
            uint8_t sf;
            if (isQuery)
            {
                NVMA_Get_LR_TX_SF(&sf);
                AT_FormatUint8Response(sf, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &sf,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid TX_SF value\r\n");
                    commandHandled = false;
                    break;
                }
              
                sf = Constrain_u8(sf, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: TX_SF value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_TX_SF(sf);
            }
            break;
        }

        case SYS_CMD_RX_SF:
        {   
            uint8_t sf;
            if (isQuery)
            {
                NVMA_Get_LR_RX_SF(&sf);
                AT_FormatUint8Response(sf, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &sf,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid RX_SF value\r\n");
                    commandHandled = false;
                    break;
                }
                
                sf = Constrain_u8(sf, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: RX_SF value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_RX_SF(sf);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_TX_BW:
        {   
            uint8_t bw;
            if (isQuery)
            {
                NVMA_Get_LR_TX_BW(&bw);
                AT_FormatUint32Response(bw, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &bw,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid TX_BW value\r\n");
                    commandHandled = false;
                    break;
                }
               
                bw = Constrain_u8(bw, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: TX_BW value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_TX_BW(bw);
            }
            break;
        }

        case SYS_CMD_RX_BW:
        {   
            uint8_t bw;
            if (isQuery)
            {
                NVMA_Get_LR_RX_BW(&bw);
                AT_FormatUint32Response(bw, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &bw,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid RX_BW value\r\n");
                    commandHandled = false;
                    break;
                }
                
                bw = Constrain_u8(bw, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: RX_BW value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_RX_BW(bw);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_TX_IQ:
        {   
            uint8_t iq;
            if (isQuery)
            {
                NVMA_Get_LR_TX_IQ(&iq);
                AT_FormatUint8Response(iq, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &iq,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid TX_IQ value\r\n");
                    commandHandled = false;
                    break;
                }
               
                iq = Constrain_u8(iq, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: TX_IQ value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_TX_IQ(iq);
            }
            break;
        }

        case SYS_CMD_RX_IQ:
        {   
            uint8_t iq;
            if (isQuery)
            {
                NVMA_Get_LR_RX_IQ(&iq);
                AT_FormatUint8Response(iq, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &iq,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid RX_IQ value\r\n");
                    commandHandled = false;
                    break;
                }
                
                iq = Constrain_u8(iq, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: RX_IQ value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_RX_IQ(iq);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_TX_CR:
        {   
            uint8_t cr;
            if (isQuery)
            {
                NVMA_Get_LR_TX_CR(&cr);
                AT_FormatUint8Response(cr, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &cr,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid TX_CR value\r\n");
                    commandHandled = false;
                    break;
                }
               
                cr = Constrain_u8(cr, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: TX_CR value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_TX_CR(cr);
            }
            break;
        }

        case SYS_CMD_RX_CR:
        {   
            uint8_t cr;
            if (isQuery)
            {
                NVMA_Get_LR_RX_CR(&cr);
                AT_FormatUint8Response(cr, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &cr,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid RX_CR value\r\n");
                    commandHandled = false;
                    break;
                }
                    
                cr = Constrain_u8(cr, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: RX_CR value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_RX_CR(cr);
                reconfigure_rx = true;
            }
            break;
        }

         case SYS_CMD_TX_SYNCWORD:
        {
            uint8_t sync;
            if (isQuery)
            {
                NVMA_Get_LR_SyncWord_TX(&sync);
                snprintf(response, sizeof(response), "%02X\r\n", sync);
                hasResponse = true;
            }
            else
            {
                // Use HexStringToByteArray for consistent hex parsing
                uint8_t syncByte;
                if (HexStringToByteArray((char*)data, &syncByte, 1) != 1)
                {
                    AT_SendStringResponse("ERROR: TX_SYNCWORD must be 2 hex digits (e.g. 12 or 34)\r\n");
                    commandHandled = false;
                    break;
                }
                NVMA_Set_LR_SyncWord_TX(syncByte);
            }
            break;
        }

        case SYS_CMD_RX_SYNCWORD:
        {
            uint8_t sync;
            if (isQuery)
            {
                NVMA_Get_LR_SyncWord_RX(&sync);
                snprintf(response, sizeof(response), "%02X\r\n", sync);
                hasResponse = true;
            }
            else
            {
                // Use HexStringToByteArray for consistent hex parsing
                uint8_t syncByte;
                if (HexStringToByteArray((char*)data, &syncByte, 1) != 1)
                {
                    AT_SendStringResponse("ERROR: RX_SYNCWORD must be 2 hex digits (e.g. 12 or 34)\r\n");
                    commandHandled = false;
                    break;
                }
                NVMA_Set_LR_SyncWord_RX(syncByte);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_HEADERMODE_TX:
        {   
            uint8_t mode;
            if (isQuery)
            {
                NVMA_Get_LR_HeaderMode_TX(&mode);
                AT_FormatUint8Response(mode, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &mode,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid HEADERMODE_TX value\r\n");
                    commandHandled = false;
                    break;
                }
               
                mode = Constrain_u8(mode, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: HEADERMODE_TX value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_HeaderMode_TX(mode);
            }
            break;
        }

        case SYS_CMD_HEADERMODE_RX:
        {   
            uint8_t mode;
            if (isQuery)
            {
                NVMA_Get_LR_HeaderMode_RX(&mode);
                AT_FormatUint8Response(mode, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &mode,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid HEADERMODE_RX value\r\n");
                    commandHandled = false;
                    break;
                }
                
                mode = Constrain_u8(mode, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: HEADERMODE_RX value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_HeaderMode_RX(mode);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_CRC_TX:
        {   
            uint8_t crc;
            if (isQuery)
            {
                NVMA_Get_LR_CRC_TX(&crc);
                AT_FormatUint8Response(crc, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &crc,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid CRC_TX value\r\n");
                    commandHandled = false;
                    break;
                }
               
                crc = Constrain_u8(crc, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: CRC_TX value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_CRC_TX(crc);
            }
            break;
        }

        case SYS_CMD_CRC_RX:
        {   
            uint8_t crc;
            if (isQuery)
            {
                NVMA_Get_LR_CRC_RX(&crc);
                AT_FormatUint8Response(crc, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint8(data, &crc,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid CRC_RX value\r\n");
                    commandHandled = false;
                    break;
                }
               
                crc = Constrain_u8(crc, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: CRC_RX value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_CRC_RX(crc);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_PREAM_SIZE_TX:
        {   
            uint16_t size;
            if (isQuery)
            {
                NVMA_Get_LR_PreamSize_TX((uint16_t*)&size);
                AT_FormatUint16Response(size, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint16(data,(uint16_t*) &size,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid PREAM_SIZE_TX value\r\n");
                    commandHandled = false;
                    break;
                }
               
                size = Constrain_u16(size, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: PREAM_SIZE_TX value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_PreamSize_TX(size);
            }
            break;
        }

        case SYS_CMD_PREAM_SIZE_RX:
        {   
            uint16_t size = 0;
            if (isQuery)
            {
                NVMA_Get_LR_PreamSize_RX((uint16_t*)&size);
                AT_FormatUint16Response(size, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint16(data,(uint16_t*) &size,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid PREAM_SIZE_RX value\r\n");
                    commandHandled = false;
                    break;
                }
               
                size = Constrain_u16(size, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: PREAM_SIZE_RX value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_PreamSize_RX(size);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_RF_TX_HEX:
        {   
            uint8_t packet[256];
            uint8_t packetSize;
            packetSize = HexStringToByteArray((char *)data, packet, sizeof(packet));
            if (packetSize == 0)
            {
                AT_SendStringResponse("ERROR: Invalid HEX data\r\n");
                commandHandled = false;
                break;
            }
            StopPeriodicTx(); // Stop periodic TX if running
            _GSC_Handle_TX(packet, packetSize);
            break;
        }

        case SYS_CMD_RF_TX_TXT:
        {
            StopPeriodicTx(); // Stop periodic TX if running
            uint8_t packet[256];
            size_t packetSize = strlen((char *)data);
            
            if (packetSize == 0 || packetSize >= sizeof(packet))
            {
                AT_SendStringResponse("ERROR: Invalid text data\r\n");
                commandHandled = false;
                break;
            }

            StopPeriodicTx(); // Stop periodic TX if running
            memcpy(packet, data, packetSize);
            _GSC_Handle_TX(packet, packetSize);
            break;
        }

        case SYS_CMD_RF_TX_NVM_ONCE:
        {   
            StopPeriodicTx(); // Stop periodic TX if running
            uint8_t tx;

            if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
            {
                AT_SendStringResponse("ERROR: Command not found\r\n");
                commandHandled = false;
                break;
            }
            StopPeriodicTx(); // Stop periodic TX if running

            // Transmit saved NVM packet once
            if (!AT_ParseUint8(data, &tx,maxLength))
            {
                AT_SendStringResponse("ERROR: Invalid TX_NVM_ONCE value\r\n");
                commandHandled = false;
                break;
            }
            else
            {   
                if(tx == 1)
                {
                    uint8_t packet[256];
                    uint16_t packetSize;
                    NVMA_Get_LR_Saved_Pckt_Size(&packetSize);
                    NVMA_Get_LR_TX_RF_PCKT(packet,packetSize);
                    _GSC_Handle_TX(packet, packetSize);
                }
                else 
                {
                    AT_SendStringResponse("ERROR: Invalid TX_NVM_ONCE value\r\n");
                    commandHandled = false;
                    break;
                }
                
            }

            break;
        }

        case SYS_CMD_RF_TX_NVM_PERIOD:
        {
            uint32_t period;
            if (isQuery)
            {
                NVMA_Get_LR_TX_Period_TX(&period);
                AT_FormatUint32Response(period, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {   
                 if (!GetCommandLimits(cmd, &minValue, &maxValue, &maxLength))
                {
                     AT_SendStringResponse("ERROR: Command not found\r\n");
                    commandHandled = false;
                    break;
                }

                if (!AT_ParseUint32(data, &period,maxLength))
                {
                    AT_SendStringResponse("ERROR: Invalid TX_NVM_PERIOD value\r\n");
                    commandHandled = false;
                    break;
                }
                
                period = Constrain_u32(period, minValue, maxValue,&constrained);
                if(constrained)
                {
                    AT_SendStringResponse("ERROR: TX_NVM_PERIOD value out of limit\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_TX_Period_TX(period);
            }
            break;
        }

        case SYS_CMD_RF_TX_PERIODIC_NVM:
        {
            // Start/Stop periodic NVM packet TX
            if (isQuery)
            {
                if(pvTimerGetTimerID(periodicTxTimer))
                {
                    AT_SendStringResponse("1\r\n");
                }
                else
                {
                    AT_SendStringResponse("0\r\n");
                }
                commandHandled = false;
            }
            else
            {
                uint8_t value;
                if (ParseBoolValue((char*)data, &value))
                {
                    if (value == 1)
                    {
                        StartPeriodicTx();
                    }
                    else
                    {
                        StopPeriodicTx();
                    }
                }
                else
                {
                    AT_SendStringResponse("ERROR: Invalid value (use 1/ON or 0/OFF)\r\n");
                    commandHandled = false;
                    break;
                }
            }
            break;
        }

        case SYS_CMD_RF_SAVE_PCKT_NVM:
        {   
            if (isQuery)
            {      
                uint16_t pcktSize;
                uint8_t packet[256];
                char hexString[513]; // 256 bytes * 2 + 1 null terminator
                char outputBuffer[560]; // hexString + size info + TOA
                
                NVMA_Get_LR_Saved_Pckt_Size(&pcktSize);
                
                if (pcktSize > 0 && pcktSize <= 256)
                {
                    uint32_t toa_ms = ru_calculate_toa_ms((uint8_t)pcktSize);
                    NVMA_Get_LR_TX_RF_PCKT(packet, pcktSize);
                    ByteArrayToHexString(packet, pcktSize, hexString, sizeof(hexString));
                    snprintf(outputBuffer, sizeof(outputBuffer), "packet:%s, size:%u B, TOA:%lu ms, ", hexString, pcktSize, toa_ms);
                    AT_SendStringResponse(outputBuffer);
                    hasResponse = false; // Už bylo odesláno
                }
                else
                {
                    AT_SendStringResponse("ERROR: No packet saved or invalid size\r\n");
                    commandHandled = false;
                }
            }
            else
            {
                // Save packet to NVM
                uint8_t packet[256];
                uint8_t packetSize;
                packetSize = HexStringToByteArray((char *)data, packet, sizeof(packet));
                if (packetSize == 0)
                {
                    AT_SendStringResponse("ERROR: Invalid HEX data\r\n");
                    commandHandled = false;
                    break;
                }
                NVMA_Set_LR_Pckt_Size(packetSize);
                NVMA_Set_LR_TX_RF_PCKT(packet,packetSize);

            }
            break;
        }

        case SYS_CMD_TX_CW:
        {
            // Start/Stop TX CW mode
            if (isQuery)
            {
                AT_SendStringResponse("Use =1 (ON) or =0 (OFF)\r\n");
                commandHandled = false;
            }
            else
            {
                uint8_t value;
                if (ParseBoolValue((char*)data, &value))
                {
                    dataQueue_t queueData;
                    queueData.cmd = CMD_RF_TX_CW;
                    queueData.data = value;
                    queueData.ptr = NULL;

                    if (xQueueSend(queueRadioHandle, &queueData, pdMS_TO_TICKS(100)) != pdPASS)
                    {
                        AT_SendStringResponse("ERROR: Failed to send CW command to RF task\r\n");
                        commandHandled = false;
                    }
                }
                else
                {
                    AT_SendStringResponse("ERROR: Invalid value (use 1/ON or 0/OFF)\r\n");
                    commandHandled = false;
                    break;
                }
            }
            break;
        }

        case SYS_CMD_RF_PERIOD_STATUS:
        {
            AT_SendStringResponse("TODO\r\n");
            commandHandled = false;
            break;
            // Get periodic TX status
            // if (isQuery)
            // {
            //     NVMA_Set_LR_TX_Period_TX(&status);
            //     AT_FormatUint8Response(status, (uint8_t *)response, &response_size);
            //     hasResponse = true;
            // }
        }

        case SYS_CMD_TX_COMPLETE_SET:
        {
            // Set TX complete callback
            ProcessRFMultiSetCommand(true, (char *)data);
            break;
        }

        case SYS_CMD_RX_COMPLETE_SET:
        {
            // Set RX complete callback
            ProcessRFMultiSetCommand(false, (char *)data);
            break;
        }

        case SYS_CMD_RF_RX_TO_UART:
        {
            if (isQuery)
            {
                uint8_t rxToUart;
                NVMA_Get_RX_TO_UART(&rxToUart);
                AT_FormatUint8Response(rxToUart, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else if(_GSC_Handle_RX_TO_UART(data, size) == false)
            {
                AT_SendStringResponse("ERROR: Invalid RX_TO_UART value\r\n");
                commandHandled = false;
            }
            break;
        }

        case SYS_CMD_RF_GET_TOA:
        {
            // Parse packet size from data
            uint16_t packetSize = 0;
            if (AT_ParseUint16(data, &packetSize, 3) && packetSize > 0 && packetSize <= 255)
            {
                uint32_t toa_ms = ru_calculate_toa_ms((uint8_t)packetSize);
                snprintf(response, sizeof(response), "TOA: %lu ms\r\n", toa_ms);
                hasResponse = true;
            }
            else
            {
                AT_SendStringResponse("ERROR: Invalid packet size (1-255)\r\n");
                commandHandled = false;
            }
            break;
        }

        case SYS_CMD_RF_GET_TSYM:
        {
            uint32_t tsym_us = ru_calculate_symbol_time_us();
            snprintf(response, sizeof(response), "TSYM: %lu us\r\n", tsym_us);
            hasResponse = true;
            break;
        }

        case SYS_CMD_TX_LDRO:
        {   
            uint8_t ldro;
            if (isQuery)
            {
                NVMA_Get_LR_TX_LDRO(&ldro);
                AT_FormatUint8Response(ldro, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {
                if (!AT_ParseUint8(data, &ldro, 1))
                {
                    AT_SendStringResponse("ERROR: Invalid TX_LDRO value\r\n");
                    commandHandled = false;
                    break;
                }
                
                if (ldro > 2)
                {
                    AT_SendStringResponse("ERROR: TX_LDRO value must be 0, 1, or 2\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_TX_LDRO(ldro);
            }
            break;
        }

        case SYS_CMD_RX_LDRO:
        {   
            uint8_t ldro;
            if (isQuery)
            {
                NVMA_Get_LR_RX_LDRO(&ldro);
                AT_FormatUint8Response(ldro, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {
                if (!AT_ParseUint8(data, &ldro, 1))
                {
                    AT_SendStringResponse("ERROR: Invalid RX_LDRO value\r\n");
                    commandHandled = false;
                    break;
                }
                
                if (ldro > 2)
                {
                    AT_SendStringResponse("ERROR: RX_LDRO value must be 0, 1, or 2\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_RX_LDRO(ldro);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_RX_PLDLEN:
        {   
            uint8_t pldlen;
            if (isQuery)
            {
                NVMA_Get_LR_RX_PldLen(&pldlen);
                AT_FormatUint8Response(pldlen, (uint8_t *)response, &response_size);
                hasResponse = true;
            }
            else
            {
                if (!AT_ParseUint8(data, &pldlen, 3))  // max 255 = 3 digits
                {
                    AT_SendStringResponse("ERROR: Invalid RX_PLDLEN value\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_LR_RX_PldLen(pldlen);
                reconfigure_rx = true;
            }
            break;
        }

        case SYS_CMD_AUX_SET:
            if(_GSC_Handle_AUX_PIN_SET(data, size) == false)
            {
                AT_SendStringResponse("ERROR: Invalid AUX_SET value\r\n");
                commandHandled = false;
            }
            break;

        case SYS_CMD_AUX_PULSE:
            if(_GSC_Handle_AUX_PIN_PWM(data, size) == false)
            {
                AT_SendStringResponse("ERROR: Invalid AUX_PULSE value\r\n");
                commandHandled = false;
            }
            break;

        case SYS_CMD_AUX_STOP:
            if(_GSC_Handle_AUX_STOP(data, size) == false)
            {
                AT_SendStringResponse("ERROR: Invalid AUX_STOP value\r\n");
                commandHandled = false;
            }
        
            break;

        case SYS_CMD_UART_BAUD:
        {
            uint32_t baud;
            if (isQuery)
            {
                NVMA_Get_UART_Baud(&baud);
                snprintf(response, sizeof(response), "%lu\r\n", baud);
                hasResponse = true;
            }
            else
            {
                if (!AT_ParseUint32(data, &baud, 6))  // max 921600 = 6 digits
                {
                    AT_SendStringResponse("ERROR: Invalid UART_BAUD value\r\n");
                    commandHandled = false;
                    break;
                }
                
                if (!NVMA_Is_Valid_Baud(baud))
                {
                    AT_SendStringResponse("ERROR: Invalid baud rate. Use: 9600, 19200, 38400, 57600, 115200, 230400\r\n");
                    commandHandled = false;
                    break;
                }
                
                NVMA_Set_UART_Baud(baud);
                AT_SendStringResponse("OK\r\n");
                // Small delay to let response be sent before restart
                osDelay(100);
                NVIC_SystemReset();
            }
            break;
        }

        case SYS_CMD_RX_FORMAT:
        {
            uint8_t format;
            if (isQuery)
            {
                NVMA_Get_RX_Format(&format);
                snprintf(response, sizeof(response), "%s\r\n", (format == RX_FORMAT_ASCII) ? "ASCII" : "HEX");
                hasResponse = true;
            }
            else
            {
                if (strcasecmp((char*)data, "HEX") == 0)
                {
                    NVMA_Set_RX_Format(RX_FORMAT_HEX);
                }
                else if (strcasecmp((char*)data, "ASCII") == 0)
                {
                    NVMA_Set_RX_Format(RX_FORMAT_ASCII);
                }
                else
                {
                    AT_SendStringResponse("ERROR: Invalid format. Use HEX or ASCII\r\n");
                    commandHandled = false;
                }
            }
            break;
        }

        default:
        {
            commandHandled = false;
            AT_SendStringResponse("ERROR\r\n");
            break;
        }
    }

    if (hasResponse)
    {
        AT_SendStringResponse(response);
    }
    else
    {
        if (commandHandled == false)
        {
          //
        }
        else
        {
            AT_SendStringResponse("OK\r\n");
        }
    }

    if(reconfigure_rx == true)
    {
        RxReconfigTimerCallback(NULL);
        // Spustí/restartuje timer pro RX rekonfiguraci
        //TriggerRxReconfig();
    }
    return commandHandled;
}
  

/**
 * @brief Handle Blue LED command
 * 
 * @param data 
 * @return true 
 * @return false 
 */
static void _GSC_Handle_TX(uint8_t *data, uint8_t size)
{   
    dataQueue_t     txm;
    packet_info_t	*tx_pkt;
    uint8_t         *tx_raw_data;

    tx_raw_data =  pvPortMalloc(size);
    if (tx_raw_data == NULL)
    {
        _exit(313513);
    }

    memcpy(tx_raw_data, data, size);

    tx_pkt = pvPortMalloc(sizeof(packet_info_t));
    if (tx_pkt == NULL)
    {
        _exit(314687);
    }
    
    tx_pkt->packet = tx_raw_data;
    tx_pkt->size = size;
    
    txm.ptr = tx_pkt;
    txm.cmd = CMD_RF_SEND_DATA_NOW;

    xQueueSend(queueRadioHandle,&txm,portMAX_DELAY);
}

/**
 * @brief 
 * 
 * @param data 
 * @param size 
 * @return true 
 * @return false 
 */
static bool _GSC_Handle_RX_TO_UART(uint8_t *data, uint8_t size)
{
    (void)size;
    dataQueue_t     txm;
    txm.ptr = NULL;

    txm.cmd = CMD_RF_RADIO_RX_TO_UART;

    uint8_t value;
    if (ParseBoolValue((char*)data, &value))
    {
        txm.data = value;
        NVMA_Set_RX_To_UART(value);
    }
    else
    {
        return false;
    }

    xQueueSend(queueRadioHandle,&txm,portMAX_DELAY);
    return true;
}

static bool _GSC_Handle_AUX_PIN_SET(uint8_t *data, uint8_t size)
{
    (void)size;
    uint8_t pin, state;

    // Ověření: existují přesně dva parametry (pin,state)
    char *token = strtok((char*)data, ",");
    if (!token || !AT_ParseUint8((uint8_t*)token, &pin, 1)) {
        return false;
    }
    if (pin < 1 || pin > AUX_PINS_COUNT) {
        return false;
    }
    pin -= 1; // uživatel zadává 1-8, indexujeme 0-7

    token = strtok(NULL, ",");
    if (!token || !AT_ParseUint8((uint8_t*)token, &state, 1)) {
        return false;
    }
    if (state > 1) {
        return false;
    }

    // Pokud je další token, chyba
    if (strtok(NULL, ",") != NULL) {
        return false;
    }

    // Nastavení pinu - použijeme auxPins pole s daným indexem
    AUX_PinControl_t *ctx = &auxPins[pin];
    HAL_GPIO_WritePin(ctx->port, ctx->pinMask, state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    return true;
}

static bool _GSC_Handle_AUX_PIN_PWM(uint8_t *data, uint8_t size)
{
    (void)size;
    uint8_t pin, duty;
    uint32_t period;

    // Oddělení tokenů
    char *token = strtok((char*)data, ",");
    if (!token || !AT_ParseUint8((uint8_t*)token, &pin, 1)) {
       // AT_SendStringResponse("ERROR: Invalid AUX_PULSE pin value\r\n");
        return false;
    }
    if (pin < 1 || pin > AUX_PINS_COUNT) {
      //  AT_SendStringResponse("ERROR: AUX_PULSE pin out of range (1-8)\r\n");
        return false;
    }
    pin -= 1; // uživatel zadává 1-8, indexujeme 0-7

    uint32_t fmin = AUX_IsHwPwmPin(pin) ? AUX_PWM_HW_MIN_FREQ_HZ : AUX_PWM_SW_MIN_FREQ_HZ;
    uint32_t fmax = AUX_IsHwPwmPin(pin) ? AUX_PWM_HW_MAX_FREQ_HZ : AUX_PWM_SW_MAX_FREQ_HZ;

    token = strtok(NULL, ",");
    if (!token || !AT_ParseUint32((uint8_t*)token, &period, 6)
        || period < fmin || period > fmax)
        return false;

    token = strtok(NULL, ",");
    if (!token || !AT_ParseUint8((uint8_t*)token, &duty, 3) || duty > 100)
        return false;

    // Pokud je další token, chyba
    if (strtok(NULL, ",") != NULL)
        return false;

    // Na SW enginu (aux1-6) se duty zaokrouhlí na nejbližší hodnotu, kterou lze
    // reprezentovat celým počtem RTOS ticků (fáze nemůže být kratší než 1 tick).
    // Uživatel je o zaokrouhlení informován, příkaz ale i tak uspěje.
    if (!AUX_IsHwPwmPin(pin))
    {
        uint8_t roundedDuty = AUX_SwPwm_RoundDuty(period, duty);
        if (roundedDuty != duty)
        {
            char msg[64];
            snprintf(msg, sizeof(msg), "WARN: duty rounded to %u%%\r\n", roundedDuty);
            AT_SendStringResponse(msg);
        }
    }

    // Spuštění PWM na daném pinu
    AUX_StartPWM(pin, period, duty);

    return true;
}


static bool _GSC_Handle_AUX_STOP(uint8_t *data, uint8_t size)
{
    (void)size;
    uint8_t pin;

    // Ověření: existuje přesně jeden parametr (pin)
    if (!data || !AT_ParseUint8(data, &pin, 1)) {
        AT_SendStringResponse("ERROR: Invalid AUX_STOP pin value\r\n");
        return false;
    }
    if (pin < 1 || pin > AUX_PINS_COUNT) {
        AT_SendStringResponse("ERROR: AUX_STOP pin out of range (1-8)\r\n");
        return false;
    }
    pin -= 1;

    AUX_StopPWM(pin);
    return true;
}
