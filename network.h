/*
 * network.h
 *
 *  Created on: May 2, 2018
 *      Author: xtarke
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include <semphr.h>

#define PUB_MSG_LEN 8
#define PUB_TPC_LEN 24
#define CFG_TOPICS 4

typedef struct publisher_data{
	char topic[PUB_TPC_LEN];
	char data[PUB_MSG_LEN];
} publisher_data_t;


extern SemaphoreHandle_t wifi_alive;
extern QueueHandle_t publish_queue;
extern QueueHandle_t command_queue;

void  mqtt_task(void *pvParameters);


#endif /* NETWORK_H_ */
