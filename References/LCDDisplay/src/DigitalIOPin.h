/*
 * DigitalIOPin.h
 *
 *  Created on: Jan 24, 2018
 *      Author: Usin
 */

#ifndef DIGITALIOPIN_H_
#define DIGITALIOPIN_H_

#include "chip.h"
#include "stdint.h"


class DigitalIOPin {
public:
	DigitalIOPin(uint8_t port, uint8_t pin, bool input = true, bool pullup = true, bool invert = false);
	bool read();
	void write(bool value);
private:
	bool isInput;
	bool isInvert;
	int myPort;
	int myPin;
};

#endif /* DIGITALIOPIN_H_ */
