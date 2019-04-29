/*
 * app_status.h
 *
 *  Created on: Sep 5, 2018
 *      Author: xtarke
 */

#ifndef MODBUS_REG_H_
#define MODBUS_REG_H_

#define READ_FN_CODE 0x02
#define WRITE_FN_CODE 0x01

enum {
	GATAWAY_STATUS,
	ACTUATOR_0,
	ACTUATOR_1,
	ACTUATOR_2,
	ACTUATOR_3,
	SENSOR_0,
	SENSOR_1,
	SENSOR_2,
	SENSOR_3,
	N_REGISTERS
};

void app_status_init();

uint8_t set_register(uint16_t data, uint8_t addr);
uint8_t set_act_register(uint16_t data, uint8_t addr);
uint16_t get_register(uint8_t addr);

uint8_t get_slave_addr();
void set_slave_addr(uint8_t addr);



#endif /* MODBUS_REG_H_ */
