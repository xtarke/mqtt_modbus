/*
 * comm.c
 *
 *  Created on: Jan 3, 2018
 *      Author: Renan Augusto Starke
 */

#include "espressif/esp_common.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <stdint.h>
#include <string.h>
#include "esp/uart.h"

#include "comm.h"

#include "sysparam.h"

#include "modbus_reg.h"
#include "network.h"

#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...) printf(fmt, ## __VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define TX_EN_PIN 5
#define RS485_USART 0

TaskHandle_t xHandling_485_cmd_task;
SemaphoreHandle_t rs485_data_mutex;


void comm_init(){
	vSemaphoreCreateBinary(rs485_data_mutex);
}


/**
  * @brief  Send data to RS485. TXEN PIN is asserted when transmitting
  * @param	packet_buffer: pointer to byte package
  * @param  packet_size: packet size
  *
  * @retval None.
  */
static void send_data_rs485(uint8_t *packet_buffer, uint8_t packet_size){
	uint8_t i = 0;

	/* Assert GPIO TX ENABLE PIN */
	GPIO.OUT_SET = BIT(TX_EN_PIN);
	/* Copy data to CPU TX Fifo */
	for (i=0; i < packet_size; i++)
		uart_putc(0, packet_buffer[i]);

	/* Wait data to be sent */
	uart_flush_txfifo(0);
	/* Release RS485 Line */
	GPIO.OUT_CLEAR = BIT(TX_EN_PIN);

}

static uint8_t receive_data_rs485(uint8_t *rx_pkg, uint8_t packet_size, uint16_t timeout){

	uint8_t error = 0;
	TickType_t my_time;

	my_time = xTaskGetTickCount();
	/* Get received data */
	for (int i=0; (i < packet_size);) {
		int data = uart_getc_nowait(0);

		if (data != -1){
			rx_pkg[i] = data;
			i++;
			continue;
		}
		/* Timeout */
		if (xTaskGetTickCount() > (my_time + timeout)){
			error = 1;
			break;
		}
	}

	return error;
}

/**
  * @brief  CRC 16 calculation.
  * @param	buf: pointer to byte package.
  * @param  len: packet size.
  *
  * @retval crc16 value with bytes swapped.
  */
uint16_t CRC16_2(uint8_t *buf, int len)
{
	uint16_t crc = 0xFFFF;
	int i;

	for (i = 0; i < len; i++)
	{
		crc ^= (uint16_t)buf[i];          // XOR byte into least sig. byte of crc

		for (int i = 8; i != 0; i--) {    // Loop over each bit
			if ((crc & 0x0001) != 0) {      // If the LSB is set
				crc >>= 1;                    // Shift right and XOR 0xA001
				crc ^= 0xA001;
			}
			else                            // Else LSB is not set
				crc >>= 1;                    // Just shift right
		}
	}
	// Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
	return crc;
}

void status_task(void *pvParameters)
{
    uint16_t temperature, crc16;
    uint8_t error = 0;
    uint8_t rx_pkg[16], pkg[8] = {0x07, 0x1e, 0x83, 0x88, 0xff};

    memset(rx_pkg, 0, sizeof(rx_pkg));

    /* Enable GPIO for 485 transceiver */
    gpio_enable(TX_EN_PIN, GPIO_OUTPUT);

    while (1) {
       	vTaskDelay(1000 / portTICK_PERIOD_MS);

       	/* Update pkg address and CRC */
       	pkg[0] = get_slave_addr();
       	crc16 = CRC16_2(pkg,2);
       	pkg[2] = crc16 & 0x00ff;
       	pkg[3] = (crc16 >> 8);

       	if( xSemaphoreTake(rs485_data_mutex, portMAX_DELAY) == pdTRUE ) {
			send_data_rs485(pkg,5);
			error = receive_data_rs485(rx_pkg, 13, 50);
			xSemaphoreGive(rs485_data_mutex);
		}
       	else
       		error = 2;

    	/* Check CRC from received package */
		crc16 = CRC16_2(rx_pkg,11);
		if (rx_pkg[12] != (crc16 >> 8) || rx_pkg[11] != (crc16 & 0xff))
			error = 3;

		if (!error) {
#ifdef DEBUGG
			for (i=0; i < 13; i++)
				printf("%x-", rx_pkg[i]);
			puts("");
#endif
			temperature = (rx_pkg[3] << 8) | rx_pkg[4];
			debug("%d.%d\n", temperature/10, temperature % 10);
			//set_register(temperature);
		}else
			printf("status error: %d\n", error);
    }
}

int8_t get_modbus_pkg(uint8_t *rx_pkg, uint8_t packet_size, uint16_t timeout){
	int i = 0;
	uint8_t error = 0;
	TickType_t my_time;

	/* Wait for address */
	uart_rxfifo_wait(0, 1);

	my_time = xTaskGetTickCount();

	/* Get first byte from hardware FIFO */
	rx_pkg[i++] = uart_getc(0);

	/* Invalid address */
	if (rx_pkg[0] != get_slave_addr())
		return -1;

	/* Get received data */
	for (; (i < packet_size); i++) {
		uart_rxfifo_wait(0, 1);
		rx_pkg[i] = uart_getc(0);
	}

	return 0;
}



void rx_commands_task(void *pvParameters){
	uint16_t crc16, pkg_crc16;
	uint8_t pkg[16], ret;

	while (1){
		/* Wait for address byte for custom Modbus package */
		ret = get_modbus_pkg(pkg,8,20);

		if (ret != 0)
			continue;

		/* Check CRC: bytes are inverted */
		pkg_crc16 = (pkg[6] << 8) | (pkg[7] & 0xff);
		crc16 = CRC16_2(pkg, 6);

		if (pkg_crc16 != crc16) {
			pkg[1] = 0xff;
			crc16 = CRC16_2(pkg,2);

			pkg[2] = crc16 >> 8;
			pkg[3] = crc16 & 0xff;

			send_data_rs485(pkg,4);
			puts("crc error");

			continue;
		}

		if (pkg[1] == WRITE_FN_CODE)
		{
			uint16_t data = (pkg[4] << 8 | pkg[5]);
			uint8_t addr = pkg[3];
			int8_t ret = set_register(data, addr);

			if (ret == 0){
				send_data_rs485(pkg,8);
				puts("ok");
			}
			else{
				/* Error writing in registers
				 * Send error package         */
				pkg[1] = 0xfe;
				pkg[2] = crc16 >> 8;
				pkg[3] = crc16 & 0xff;
				send_data_rs485(pkg,4);
				puts("erro");
			}
			printf("Write %x @ %x %d\n", data, addr, ret);
		}

		if (pkg[1] == READ_FN_CODE){
			uint8_t addr = pkg[3];
			uint8_t size = pkg[5]; // Max 4 registers

			if (size > 4)
				size = 4;

			for (int i=addr,j=4; i < (addr + size); i++, j+=2) {
				uint16_t data = get_register(i);
				pkg[j] = data >> 8;
				pkg[j+1] = data & 0xff;
			}

			crc16 = CRC16_2(pkg, size*2 + 4);

			pkg[4+size*2] = crc16 >> 8;
			pkg[4+size*2 + 1] = crc16 & 0xff;

			send_data_rs485(pkg, size*2 + 6);
		}
	}
}
