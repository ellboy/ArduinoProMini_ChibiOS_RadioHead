/*
 * mavlink.c
 *
 *  Created on: 18.10.2016
 *      Author: ellboy
 */

#include "mavlink_interface.h"

#include "RHReliableDatagram_wrapper.h"

#include <debug.h>

mavlink_system_t mavlink_system;

// Define the system type, in this case an airplane
uint8_t system_type = MAV_TYPE_COAXIAL;
uint8_t autopilot_type = MAV_AUTOPILOT_GENERIC;

uint8_t system_mode = MAV_MODE_PREFLIGHT; ///< Booting up
uint32_t custom_mode = 0;   ///< Custom mode, can be defined by user/adopter
uint8_t system_state = MAV_STATE_STANDBY; ///a < System ready for flight

static int comms_packet_drops, comms_packet_success = 0;

//uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];

#define MAVLINK_MESSAGE_INFO_CUSTOM {MAVLINK_MESSAGE_INFO_HEARTBEAT, MAVLINK_MESSAGE_INFO_SYS_STATUS}
static const mavlink_message_info_t message_info[20] = MAVLINK_MESSAGE_INFO_CUSTOM;

#if 0
static void print_one_field(mavlink_message_t *msg, const mavlink_field_info_t *f, int idx) {

#define PRINT_FORMAT(f, def) (f->print_format?f->print_format:def)

	switch (f->type) {
	case MAVLINK_TYPE_CHAR:
		print_std(PRINT_FORMAT(f, "%c"),
				_MAV_RETURN_char(msg, f->wire_offset + idx * 1));
		break;
	case MAVLINK_TYPE_UINT8_T:
		print_std(PRINT_FORMAT(f, "%u"),
				_MAV_RETURN_uint8_t(msg, f->wire_offset + idx * 1));
		break;
	case MAVLINK_TYPE_INT8_T:
		print_std(PRINT_FORMAT(f, "%d"),
				_MAV_RETURN_int8_t(msg, f->wire_offset + idx * 1));
		break;
	case MAVLINK_TYPE_UINT16_T:
		print_std(PRINT_FORMAT(f, "%u"),
				_MAV_RETURN_uint16_t(msg, f->wire_offset + idx * 2));
		break;
	case MAVLINK_TYPE_INT16_T:
		print_std(PRINT_FORMAT(f, "%d"),
				_MAV_RETURN_int16_t(msg, f->wire_offset + idx * 2));
		break;
	case MAVLINK_TYPE_UINT32_T:
		print_std(PRINT_FORMAT(f, "%lu"),
				(unsigned long) _MAV_RETURN_uint32_t(msg,
						f->wire_offset + idx * 4));
		break;
	case MAVLINK_TYPE_INT32_T:
		print_std(PRINT_FORMAT(f, "%ld"),
				(long) _MAV_RETURN_int32_t(msg, f->wire_offset + idx * 4));
		break;
	case MAVLINK_TYPE_UINT64_T:
		print_std(PRINT_FORMAT(f, "%llu"),
				(unsigned long long) _MAV_RETURN_uint64_t(msg,
						f->wire_offset + idx * 8));
		break;
	case MAVLINK_TYPE_INT64_T:
		print_std(PRINT_FORMAT(f, "%lld"),
				(long long) _MAV_RETURN_int64_t(msg, f->wire_offset + idx * 8));
		break;
	case MAVLINK_TYPE_FLOAT:
		print_std(PRINT_FORMAT(f, "%f"),
				(double) _MAV_RETURN_float(msg, f->wire_offset + idx * 4));
		break;
	case MAVLINK_TYPE_DOUBLE:
		print_std(PRINT_FORMAT(f, "%f"),
				_MAV_RETURN_double(msg, f->wire_offset + idx * 8));
		break;
	}
}

static void print_field(mavlink_message_t *msg, const mavlink_field_info_t *f) {
	print_std("%s: ", f->name);
	if (f->array_length == 0) {
		print_one_field(msg, f, 0);
		print_std(" ");
	} else {
		unsigned i;
		/* print an array */
		if (f->type == MAVLINK_TYPE_CHAR) {
			print_std("'%.*s'", f->array_length,
					f->wire_offset + (const char *) _MAV_PAYLOAD(msg));
		} else {
			print_std("[ ");
			for (i = 0; i < f->array_length; i++) {
				print_one_field(msg, f, i);
				if (i < f->array_length) {
					print_std(", ");
				}
			}
			print_std("]");
		}
	}
	print_std(" ");
}

void print_message(mavlink_message_t *msg) {
	//print_std("print_message\r\n");
	const mavlink_message_info_t *m = &message_info[msg->msgid];
	const mavlink_field_info_t *f = m->fields;
	unsigned i;
	print_std("%s { ", m->name);
	for (i = 0; i < m->num_fields; i++) {
		print_field(msg, &f[i]);
	}
	print_std("}\r\n");
}
#endif

void print_raw(uint8_t *buf, uint16_t len) {

	int i;

	for (i = 0; i < len; i++) {
		chSequentialStreamPut(OUTPUT_STD, buf[i]);
	}

}

void mavlink_reliable_send(uint8_t *data, uint16_t dataLen, uint8_t targetAddress) {

	// Send a message to manager_server
	if (RHReliableDatagram_sendtoWait(data, dataLen, targetAddress)) {

	} else {
		print_dbg("Debug: %s: %s\r\n", __func__, "sendtoWait failed");
	}

}

void mavlink_client(uint8_t targetAddress) {
	mavlink_heart(targetAddress);
}

void mavlink_server() {
	//chprintf((BaseChannel *) &SDU1, "NRF test thread created\r\n");
	uint8_t buf[RH_NRF24_maxMessageLength()];
	uint8_t len;
	int i;
	mavlink_message_t msg;
	mavlink_status_t status;

	if (RHReliableDatagram_available()) {
		// Wait for a message addressed to us from the client
		uint8_t len = sizeof(buf);
		uint8_t from;
		if (RHReliableDatagram_recvfromAck(buf, &len, &from, NULL, NULL, NULL)) {
			//print_dbg("Debug: %s : got request from : %d : %s\r\n", __func__, from, (char* )buf);

//				len = nrf_recv(nrf, buf, sizeof(buf));
//
//				//print_std("NRF RECV: len=%d, val=%s\r\n", len, buf);

			for (i = 0; i < len; i++) {
				if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) {
					//gwinPrintf(GW1,"MAVLINK RECV %d: msg id=%d, sys id=%d\r\n", msg.seq, msg.msgid, msg.sysid);
					//print_message(&msg);

					switch (msg.msgid) {
					case MAVLINK_MSG_ID_HEARTBEAT: {
						// E.g. read GCS heartbeat and go into
						// comm lost mode if timer times out
						//print_std("heart\r\n");
						print_dbg("Debug: heart\r\n");
					}
						break;
					case MAVLINK_MSG_ID_SYS_STATUS:
						// EXECUTE ACTION
						print_dbg("Debug: status\r\n");
						break;
					case MAVLINK_MSG_ID_ATTITUDE:
						print_dbg("Debug: attitude\r\n");
//							gwinPrintf(GW3, "attitude\r\n");
//							mavlink_msg_attitude_decode(&msg, &attitude);
//							gwinPrintf(GW3, "r: %d, p: %d, y: %d\r\n", FLOAT_TO_FLOAT_T(ToDeg(attitude.roll)), FLOAT_TO_FLOAT_T(ToDeg(attitude.pitch)), FLOAT_TO_FLOAT_T(ToDeg(attitude.yaw)));
//							drawYaw_float(80, 180, 50, FLOAT_TO_FLOAT_T(ToDeg(attitude.yaw)));
//							drawRollPitch_float(80, 80, 68, 120, FLOAT_TO_FLOAT_T(ToDeg(attitude.pitch)), FLOAT_TO_FLOAT_T(ToDeg(attitude.roll)));
						break;
					default:
						//Do nothing
						break;
					}

					//				comms_packet_drops += status.packet_rx_drop_count;
					//				comms_packet_success += status.packet_rx_success_count;

				}
			}

		}

	}
}


void mavlink_heart(uint8_t targetAddress) {

	mavlink_system.sysid = 20;                   ///< ID 20 for this airplane
	mavlink_system.compid = MAV_COMP_ID_IMU; ///< The component sending the message is the IMU, it could be also a Linux process
	mavlink_system.type = MAV_TYPE_GENERIC; ///< This system is an airplane / fixed wing

	// Initialize the required buffers
	mavlink_message_t msg;
	uint8_t buf[MAVLINK_MAX_PACKET_LEN];

// Pack the message
	mavlink_msg_heartbeat_pack(mavlink_system.sysid, mavlink_system.compid,
			&msg, system_type, autopilot_type, system_mode, custom_mode,
			system_state);

// Copy the message to the send buffer
	uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

// Send the message with the standard UART send function
// uart0_send might be named differently depending on
// the individual microcontroller / library in use.
//uart0_send(buf, len);

//	print_message(&msg);


	print_raw(buf, len);
	mavlink_reliable_send(buf, len, targetAddress);

}

