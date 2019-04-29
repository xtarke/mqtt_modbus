/*
 * app_status.c
 *
 *  Created on: Sep 5, 2018
 *      Author: xtarke
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "modbus_reg.h"

struct system_status_t {
	uint8_t my_slave_addr;
	uint16_t registers[N_REGISTERS];
};

volatile struct system_status_t sys_status = { 0 };

SemaphoreHandle_t sys_data_mutex;

void app_status_init(){
	vSemaphoreCreateBinary(sys_data_mutex);

	sys_status.my_slave_addr = 0x15;
}

uint8_t set_register(uint16_t data, uint8_t addr){
	uint8_t ret = -1;

	if (xSemaphoreTake(sys_data_mutex, portMAX_DELAY) == pdTRUE ){
		if (addr < N_REGISTERS)
  			if (addr >= SENSOR_0 && addr <= SENSOR_3) {
  				sys_status.registers[addr] = data;
  				ret = 0;
  			}
		xSemaphoreGive(sys_data_mutex);
  	}

	return ret;
}

uint8_t set_act_register(uint16_t data, uint8_t addr){
	uint8_t ret = -1;

	if (xSemaphoreTake(sys_data_mutex, portMAX_DELAY) == pdTRUE ){
		if (addr < N_REGISTERS)
  			if (addr >= ACTUATOR_0 && addr <= ACTUATOR_3) {
  				sys_status.registers[addr] = data;
  				ret = 0;
  			}
		xSemaphoreGive(sys_data_mutex);
  	}

	return ret;
}

uint16_t get_register(uint8_t addr){
	uint16_t temp = 0;

	if (xSemaphoreTake(sys_data_mutex, portMAX_DELAY) == pdTRUE ){

		if (addr < N_REGISTERS)
			temp = sys_status.registers[addr];

		xSemaphoreGive(sys_data_mutex);
	}

	return temp;
}

void set_slave_addr(uint8_t addr){
	if (xSemaphoreTake(sys_data_mutex, portMAX_DELAY) == pdTRUE ){
		sys_status.my_slave_addr = addr;
		xSemaphoreGive(sys_data_mutex);
	}
}

uint8_t get_slave_addr(){
	uint8_t addr = 0;

	if (xSemaphoreTake(sys_data_mutex, portMAX_DELAY) == pdTRUE ){
		addr = sys_status.my_slave_addr;
		xSemaphoreGive(sys_data_mutex);
	}

	return addr;
}




