/*
 * DigitalIOPin.cpp
 *
 *  Created on: Jan 24, 2018
 *      Author: Usin
 */

#include "DigitalIOPin.h"

DigitalIOPin::DigitalIOPin(uint8_t port, uint8_t pin, bool input, bool pullup, bool invert) {
	uint32_t mode = IOCON_DIGMODE_EN;

	if (input) {

		if (pullup) {
			mode = mode | IOCON_MODE_PULLUP;
		}

		if (invert) {
			mode = mode | IOCON_INV_EN;
		}


		Chip_IOCON_PinMuxSet(LPC_IOCON, port, pin, mode);
		Chip_GPIO_SetPinDIRInput(LPC_GPIO, port, pin);

	} else {
		// Remember to invert in software for output
		Chip_IOCON_PinMuxSet(LPC_IOCON, port, pin, mode);
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, port, pin);
	}

	isInput = input;
	isInvert = invert;
	myPort = port;
	myPin = pin;
}

bool DigitalIOPin::read() {
	//return Chip_GPIO_GetPinState(LPC_GPIO, myPort, myPin);
	if (!isInput&&isInvert) {
		return !Chip_GPIO_GetPinState(LPC_GPIO, myPort, myPin);
	} else {
		return Chip_GPIO_GetPinState(LPC_GPIO, myPort, myPin);
	}
}

void DigitalIOPin::write(bool value) {
	if (!isInput&&isInvert) {
		Chip_GPIO_SetPinState(LPC_GPIO, myPort, myPin, !value);
	} else {
		Chip_GPIO_SetPinState(LPC_GPIO, myPort, myPin, value);
	}
}



