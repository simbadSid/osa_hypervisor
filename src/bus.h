/*
 * bus.h
 *
 *  Created on: Sep 26, 2016
 *      Author: ogruber
 */

#ifndef BUS_H_
#define BUS_H_


#define TIMER_READ  0x1000
#define TIMER_WRITE 0x1004

#define SERIAL_RX     0x1010
#define SERIAL_TX     0x1014
#define SERIAL_STATUS 0x1018

struct bus
{
	// serial line
	struct
	{
		uint32_t RX;
		uint32_t TX;
		#define RX_AVAILABLE 0x01
		#define TX_AVAILABLE 0x02
		uint32_t status;
		#define TX_WDELAY 10
		uint32_t wdelay;
		int sock;
		int ssock;
		int servermode;
		char name[64];
	}serial;

	// timer
	struct
	{
		uint8_t read_state;
		uint8_t write_state;
		uint64_t timer; // in milliseconds
		uint64_t buffer;
	}timer;
};


#endif /* BUS_H_ */
