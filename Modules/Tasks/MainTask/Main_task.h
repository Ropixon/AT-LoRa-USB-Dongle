/*
 * main_task.h
 *
 *  Created on: Dec 13, 2023
 *      Author: Uzivatel
 */

#ifndef INC_MAIN_TASK_H_
#define INC_MAIN_TASK_H_

#include "at_cmd.h"
#include "main.h"
/**
 * @brief 
 * 
 */
typedef enum
{
	MAIN_TASK_OFF = 0,
	MAIN_TASK_ON = 1,

}task_state_e;

/**
 * @brief 
 * 
 */
typedef struct
{
	TimerResource_t	LED_alive;
	TimerResource_t	LED_AT_RX_done;
	TimerResource_t	IWDG_timer;
	TimerResource_t Periodic_RF_TX;

}main_timers_t;

/**
 * @brief Heartbeat status for IWDG
 */
typedef struct
{
	bool rf_task_alive;    // RF task responded to heartbeat
	bool hb_pending;       // Heartbeat request is pending
} heartbeat_status_t;


typedef struct
{
	task_state_e		task_state;
	main_timers_t		timers;
	heartbeat_status_t	heartbeat;

}main_ctx_t;


void main_task(void);
void irq_RELE_falling(void);
bool MT_SendDataToMainTask(dataQueue_t *data, TickType_t timeout);
bool AT_CustomCommandHandler(char *data,eATCommands atCmd, uint16_t size);

#endif /* INC_MAIN_TASK_H_ */
