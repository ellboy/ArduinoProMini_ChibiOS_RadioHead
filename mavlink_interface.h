/*
 * mavlink_interface.h
 *
 *  Created on: 18.10.2016
 *      Author: drakon
 */

#ifndef MAVLINK_INTERFACE_H_
#define MAVLINK_INTERFACE_H_

#include "hal.h"
#include "nil.h"

/**
 * The other important Mavlink setting it know about when using a uC is at the top of main.c, before the include of mavlink.h.
 * If you don’t set the number of buffers to 1 before you include mavlink.h, you will waste 768 bytes of SRAM, which the AVR does not have to spare.
 */
// Save memory, MAVLINK by default allocates 4 256 byte buffers
#define MAVLINK_COMM_NUM_BUFFERS 1

#include <mavlink/common/mavlink.h>

void mavlink_client(uint8_t targetAddress);
void mavlink_heart(uint8_t targetAddress);
void mavlink_server(void);
//void print_message(mavlink_message_t *msg);
//void mavlink_status(nrf24l01_t *nrf);
//void mavlink_attitude(nrf24l01_t *nrf, float roll, float pitch, float yaw);




#endif /* MAVLINK_INTERFACE_H_ */
