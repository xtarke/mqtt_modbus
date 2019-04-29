/*
 * comm.h
 *
 *  Created on: Jan 3, 2018
 *      Author: Renan Augusto Starke
 */

#ifndef COMM_H_
#define COMM_H_

#include <stdint.h>
#include "esp/uart.h"

typedef struct command_data{
	uint8_t cmd;
	uint16_t data;
} command_data_t;

void status_task(void *pvParameters);
void rx_commands_task(void *pvParameters);

extern TaskHandle_t xHandling_485_cmd_task;

void comm_init();


enum PKG_DEF {PKG_START = 0x7E, PKG_MAX_SIZE = 24, PKG_HEADER_SIZE = 2, PKG_INFO_SIZE = 3};
enum PKG_TYPE {PWM_DATA = 0x01, ADC_DATA = 0x11, ADC_ALL_DATA = 0x13};
enum PWM_PKG_DEF {SERVO_ID_IDX = 1, SERVO_DATA_IDX = 2, SERVO_PAYLOAD_SIZE = 3, SERVO_ACK_CMD = 0x80,
					PWM_TOTAL_PKG_SIZE = PKG_INFO_SIZE + SERVO_PAYLOAD_SIZE};

enum ADC_SINGLE_PKG_DEF {ADC_ID_IDX = 1, ADC_DATA_IDX = 2, ADC_PAYLOAD_SIZE = 4,
	ADC_ALL_DATA_IDX = 1, ADC_ALL_PAYLOAD_SIZE = 15, ADC_ACK_CMD = 0x90};

void makeAndSend(uint8_t *data, uint8_t paylodsize);

/* Returns the number of bytes actually available for reading.
 * No wait
 */
inline int uart_rxfifo_size(int uart_num) {
  	return FIELD2VAL(UART_STATUS_RXFIFO_COUNT, UART(uart_num).STATUS);
}


#endif /* COMM_H_ */
