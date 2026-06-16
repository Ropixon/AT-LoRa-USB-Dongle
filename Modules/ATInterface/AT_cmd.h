/**
 * @file AT_cmd.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-11-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef AT_CMD_H
#define AT_CMD_H

#include "portSTM32L071xx.h"

/* Firmware version */
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    1
#define FW_DEVICE_NAME      "Ropixon AT-USB LoRa Dongle"

/**
 * @brief 
 * 
 */
typedef enum
{   
    SYS_CMD_NONE                = 0,
    SYS_CMD_SYS_NAME            = 1,    
    SYS_CMD_FACTORY_RST         = 2,
    SYS_CMD_TX_CW               = 3,    
    SYS_CMD_RF_RX_TO_UART       = 4,
    SYS_CMD_RF_TX_HEX           = 5,      
    SYS_CMD_RF_TX_TXT           = 6,      
    SYS_CMD_RF_TX_NVM_ONCE      = 7, 
    SYS_CMD_RF_TX_NVM_PERIOD    = 8,  
    SYS_CMD_RF_TX_PERIODIC_NVM  = 9, 
    SYS_CMD_RF_SAVE_PCKT_NVM    = 10,
    SYS_CMD_RF_PERIOD_STATUS    = 11,

    /* LoRa SX1262-specific commands */
    SYS_CMD_TX_FREQ         = 20,
    SYS_CMD_RX_FREQ         = 21,
    SYS_CMD_TX_POWER        = 22,
    SYS_CMD_TX_SF           = 23,
    SYS_CMD_RX_SF           = 24,
    SYS_CMD_TX_BW           = 25,
    SYS_CMD_RX_BW           = 26,
    SYS_CMD_TX_IQ           = 27,
    SYS_CMD_RX_IQ           = 28,
    SYS_CMD_TX_CR           = 29,
    SYS_CMD_RX_CR           = 30,
    SYS_CMD_HEADERMODE_TX   = 31,
    SYS_CMD_HEADERMODE_RX   = 32,
    SYS_CMD_CRC_TX          = 33,
    SYS_CMD_CRC_RX          = 34,
    SYS_CMD_PREAM_SIZE_TX   = 35,
    SYS_CMD_PREAM_SIZE_RX   = 36,
    SYS_CMD_TX_SYNCWORD     = 50,
    SYS_CMD_RX_SYNCWORD     = 51,
    SYS_CMD_TX_COMPLETE_SET = 37,
    SYS_CMD_RX_COMPLETE_SET = 38,

    // AUX commands
    SYS_CMD_AUX_SET         = 39,
    SYS_CMD_AUX_PULSE       = 40,
    SYS_CMD_AUX_QUERY       = 41,
    SYS_CMD_AUX_STOP        = 42,

    // TOA command
    SYS_CMD_RF_GET_TOA      = 43,
    SYS_CMD_RF_GET_TSYM     = 44,
    SYS_CMD_TX_LDRO         = 45,
    SYS_CMD_RX_LDRO         = 46,
    SYS_CMD_RX_PLDLEN       = 47,
    SYS_CMD_UART_BAUD       = 48,
    SYS_CMD_RX_FORMAT       = 49,

} eATCommands;


/**
 * @brief 
 * 
 */
typedef struct AT_cmd
{
    SP_Context_t sp_ctx;
    bool (*onDataReceivedFromISR)(char *params, eATCommands cmdToCore, uint16_t size); 

} __attribute__((packed)) AT_cmd_t;

void AT_SendStringResponse(char *response);
void AT_HandleATCommand(uint16_t size);
void AT_HandleUartError(void);
void AT_Init(AT_cmd_t *atCmd);

#endif // AT_CMD_H

