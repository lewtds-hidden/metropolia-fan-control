/*
 * SerialPort.h
 *
 *  Created on: 10.2.2016
 *      Author: krl
 */

#ifndef NEWSERIALPORT_H_
#define NEWSERIALPORT_H_

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif


class NewSerialPort {
public:
	NewSerialPort();
	virtual ~NewSerialPort();
	int available();
	void begin(int speed = 9600);
	int read();
	int write(const char* buf, int len);
	int print(int val, int format);
	void flush();
private:
	static const int UART_RB_SIZE = 128;
	/* Transmit and receive ring buffers */
	RINGBUFF_T txring;
	RINGBUFF_T rxring;
	uint8_t rxbuff[UART_RB_SIZE];
	uint8_t txbuff[UART_RB_SIZE];


};

#endif /* NEWSERIALPORT_H_ */
