/*
 * radioUser.h
 *
 *  Created on: Nov 15, 2023
 *      Author: Uzivatel
 */

#ifndef SEMTECHRADIO_RADIOUSER_H_
#define SEMTECHRADIO_RADIOUSER_H_

#include "main.h"
#include "RF_Task.h"


#define RF_HEART_BEAT_TIMEOUT_MS			(3*CONST_1_MIN)
#define RF_EVENT_LED_TIMEOUT_MS				(100)


/**
 * @brief 
 * 
 */
typedef enum
{
	RADIO_CMD_NONE=0,
	RADIO_CMD_INIT,
	RADIO_CMD_SLEEP,
	RADIO_CMD_STANDBY,
	RADIO_CMD_STOP_RX,
	RADIO_CMD_START_RX,
	RADIO_CMD_SEND_UNIVERSAL_PAYLOAD_LBT,
	RADIO_CMD_SEND_UNIVERSAL_PAYLOAD_NOW,
	RADIO_CMD_TX_CW,
	RADIO_CMD_START_CAD,
	RADIO_CMD_STOP_TX_AND_DISCARD,

}RFCommands_e;


void ru_radio_process_commands( RFCommands_e cmd,radio_context_t *ctx, const dataQueue_t *rxm);
void ru_radio_process_IRQ( radio_context_t *ctx);
radio_modes_e	ru_get_radio_last_status( radio_context_t	*ctx);
bool ru_radio_send_packet(uint8_t *data, uint8_t size, radio_context_t	*ctx);
void ru_radio_send_packet_lbt(packet_info_t *pkt, uint32_t max_delay_ms, bool tx_on_timeout, radio_context_t *ctx);
void ru_radio_lbt_timer_event(radio_context_t *ctx);
void ru_radio_lbt_abort(radio_context_t *ctx);
bool ru_radioCleanAndStandby(ral_standby_cfg_t standbyMode, radio_context_t *ctx);
bool ru_radioCleanAndSleep( bool onlySleep, radio_context_t *ctx);
bool ru_radioInit(radio_context_t	*ctx);
void ru_sx1262_assign( radio_context_t	*ctx);
void ru_radio_rfSwitch(bool tx,radio_context_t	*ctx);
void ru_radio_start_CAD(radio_context_t	*ctx);
void ru_radio_start_rx(radio_context_t	*ctx);
uint32_t ru_calculate_toa_ms(uint8_t packetSize);
uint32_t ru_calculate_symbol_time_us(void);


#endif /* SEMTECHRADIO_RADIOUSER_H_ */
