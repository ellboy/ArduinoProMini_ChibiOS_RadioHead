/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "hal.h"
#include "nil.h"

#include "debug.h"


#define USE_MAVLINK

#define RELIABLE
//#define NRF24_CLIENT
#define NRF24_SERVER

#ifdef RELIABLE
#include "RHReliableDatagram_wrapper.h"
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2
#else
#include "RH_NRF24_wrapper.h"
#endif

#ifdef USE_MAVLINK
#include "mavlink_interface.h"
#endif

/*
 * Thread 1.
 */
THD_WORKING_AREA(waThread1, 128);
THD_FUNCTION(Thread1, arg) {

  (void)arg;



  while (true) {
//    palTogglePad(IOPORT2, PORTB_LED1);
    chThdSleepMilliseconds(500);
  }
}


void nrf24_init(void) {
#ifdef RELIABLE
	RH_NRF24_setChipEnablePin(14);
	RH_NRF24_setSlaveSelectPin(10);
#else
	if (!RH_NRF24_init(14, 10, 0)) {
		print_dbg("NRF24 init NOK\r\n");

		while (true) {
			chThdSleepMilliseconds(200);
		}
	}

	else {
		print_dbg("NRF24 init OK\r\n");

	}

	if (!RH_NRF24_setChannel(1)) {
		print_dbg("Debug: %s: %s\r\n", __func__, "setChannel failed");
		while (true) {
			chThdSleepMilliseconds(200);
		}
	}

	if (!RH_NRF24_setRF(DataRate2Mbps, TransmitPower0dBm)) {
		print_dbg("Debug: %s: %s\r\n", __func__, "setRF failed");
		while (true) {
			chThdSleepMilliseconds(200);
		}
	}
#endif
}

void nrf24_server(void) {
	print_dbg("Debug: %s: %s\r\n", __func__, "RX wait loop");



#ifdef RELIABLE
	RHReliableDatagram_setThisAddress(SERVER_ADDRESS);
	RHReliableDatagram_init();

#ifndef USE_MAVLINK
	uint8_t data[] = "And hello back to you";
	// Dont put this on the stack:
	uint8_t buf[RH_NRF24_maxMessageLength()];
#endif

	while (1) {
#ifdef USE_MAVLINK
		mavlink_server();
#else
		if (RHReliableDatagram_available()) {
			// Wait for a message addressed to us from the client
			uint8_t len = sizeof(buf);
			uint8_t from;
			if (RHReliableDatagram_recvfromAck(buf, &len, &from, NULL, NULL,
					NULL)) {
				print_dbg("Debug: %s : got request from : %d : %s\r\n",
						__func__, from, (char* )buf);

				// Send a reply back to the originator client
				if (!RHReliableDatagram_sendtoWait(data, sizeof(data), from))
					print_dbg("Debug: %s: %s\r\n", __func__,
							"sendtoWait failed");
			}
		}
#endif
	}

#else
	while (true) {
		//chnWrite(&SD1, (const uint8_t *)"Hello World!\r\n", 14);
		chThdSleepMilliseconds(200);

		if (RH_NRF24_available()) {
			// Should be a message for us now
			uint8_t buf[RH_NRF24_maxMessageLength()];
			uint8_t len = sizeof(buf);
			if (RH_NRF24_recv(buf, &len)) {
				//      NRF24::printBuffer("request: ", buf, len);
				print_dbg("Debug: %s: got request: %s\r\n", __func__, (char* )buf);

				// Send a reply
				uint8_t data[] = "And hello back to you";
				RH_NRF24_send(data, sizeof(data));
				RH_NRF24_waitPacketSent();
				print_dbg("Debug: %s: %s\r\n", __func__, "Sent a reply");

			} else {
				print_dbg("Debug: %s: %s\r\n", __func__, "recv failed");
			}
		}

	}
#endif
}



void nrf24_client(void) {
#ifdef RELIABLE

#ifndef USE_MAVLINK
	uint8_t data[] = "Hello World!";
	// Dont put this on the stack:
	uint8_t buf[RH_NRF24_maxMessageLength()];
#endif

	RHReliableDatagram_setThisAddress(CLIENT_ADDRESS);
	RHReliableDatagram_init();

	print_dbg("Debug: %s: %s\r\n", __func__,
			"Sending to rf24_reliable_datagram_server");

	while (1) {
#ifdef USE_MAVLINK
		mavlink_client(SERVER_ADDRESS);
#else
		// Send a message to manager_server
		if (RHReliableDatagram_sendtoWait(data, sizeof(data), SERVER_ADDRESS)) {
			// Now wait for a reply from the server
			uint8_t len = sizeof(buf);
			uint8_t from;
			if (RHReliableDatagram_recvfromAckTimeout(buf, &len, 2000, &from, NULL, NULL, NULL)) {

				print_dbg("Debug: %s : got reply from : %d: %s\r\n", __func__, from, (char*) buf);

			} else {
				print_dbg("Debug: %s: %s\r\n", __func__,
						"No reply, is rf24_reliable_datagram_server running?");
			}
		} else {
			print_dbg("Debug: %s: %s\r\n", __func__, "sendtoWait failed");
		}
#endif

		chThdSleepMilliseconds(500);
	}



#else
	print_dbg("Debug: %s: %s\r\n", __func__, "TX wait loop");
	while (true) {
		chThdSleepMilliseconds(200);

		print_dbg("Debug: %s: %s\r\n", __func__, "Sending to rf24_server");

		// Send a message to rf24_server
		uint8_t data[] = "Hello World!";
		RH_NRF24_send(data, sizeof(data));

		RH_NRF24_waitPacketSent();
		// Now wait for a reply
		uint8_t buf[RH_NRF24_maxMessageLength()];
		uint8_t len = sizeof(buf);

		//NRF24_waitAvailableTimeout(500)
		if (RH_NRF24_waitAvailable()) {
			// Should be a reply message for us now
			if (RH_NRF24_recv(buf, &len)) {
				print_dbg("Debug: %s: got reply: %s\r\n", __func__, buf);
			} else {
				print_dbg("Debug: %s: %s\r\n", __func__, "recv failed");
			}
		} else {
			print_dbg("Debug: %s: %s\r\n", __func__,
					"No reply, is rf24_server running?");
		}

	}
#endif
}

/*
 * Thread 2.
 */
THD_WORKING_AREA(waThread2, 128);
THD_FUNCTION(Thread2, arg) {

  (void)arg;

  sdStart(&SD1, NULL);
//  chnWrite(&SD1, (const uint8_t *)__func__, 30);
//  chnWrite(&SD1, (const uint8_t *)"\r\n", 30);

  /*
   * Activates the serial driver 1 using the driver default configuration.
   * PA9 and PA10 are routed to USART1.
   */

//#define OUTPUT_DBG  (BaseSequentialStream *)&SD1
//#define print_dbg(fmt, ...)  chprintf(OUTPUT_DBG, fmt, ##__VA_ARGS__)


//  chnWrite(&SD1, (const uint8_t *)"S", 30);
//  chnWrite(&SD1, (const uint8_t *)"\r\n", 14);
  nrf24_init();



  //(void)nrf;



#ifdef NRF24_SERVER
nrf24_server();
#endif

#ifdef NRF24_CLIENT
nrf24_client();
#endif

}

/*
 * Threads static table, one entry per thread. The number of entries must
 * match NIL_CFG_NUM_THREADS.
 */
THD_TABLE_BEGIN
  THD_TABLE_ENTRY(waThread1, "blinker", Thread1, NULL)
  THD_TABLE_ENTRY(waThread2, "hello", Thread2, NULL)
THD_TABLE_END

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  //sdStart(&SD1, NULL);

  /**
   * 102  palSetPadMode(IOPORT2, LED, PAL_MODE_OUTPUT_PUSHPULL);
103  palSetPadMode(IOPORT4, 4, PAL_MODE_OUTPUT_PUSHPULL);
104  palClearPad(IOPORT2, LED);
105  palClearPad(IOPORT4, 4);
   */

//  chThdSleepMilliseconds(2000);





	palClearPad(IOPORT2, PORTB_LED1);

//	palSetPad(IOPORT2, PORTB_LED1);



  /* This is now the idle thread loop, you may perform here a low priority
     task but you must never try to sleep or wait in this loop. Note that
     this tasks runs at the lowest priority level so any instruction added
     here will be executed after all other tasks have been started.*/
  while (true) {
  }
}
