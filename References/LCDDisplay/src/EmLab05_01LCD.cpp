#include "board.h"
#include "DigitalIOPin.h"
#include "LiquidCrystal.h"

using namespace std;

void writeLCD(LiquidCrystal lcd, bool state1, bool state2, bool state3) {
	lcd.setCursor(0, 1);
	string myString;
	myString = state1?"Down":"Up  ";
	lcd.print(myString);
	lcd.setCursor(6, 1);
	myString = state2?"Down":"Up  ";
	lcd.print(myString);
	lcd.setCursor(12, 1);
	myString = state3?"Down":"Up  ";
	lcd.print(myString);
}

int main(void) {

#if defined (__USE_LPCOPEN)
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
	// Set up and initialize all required blocks and
	// functions related to the board hardware
	Board_Init();
	// Set the LED to the state of "On"
	//Board_LED_Set(0, true);
#endif
#endif

	Chip_RIT_Init(LPC_RITIMER); // initialize RIT (enable clocking etc.)

	DigitalIOPin led (0, 25, false, true, true);
	DigitalIOPin rs(0,8,false, true, false);
	DigitalIOPin en(1,6,false, true, false);
	DigitalIOPin d4(1,8,false, true, false);
	DigitalIOPin d5(0,5,false, true, false);
	DigitalIOPin d6(0,6,false, true, false);
	DigitalIOPin d7(0,7,false, true, false);

	DigitalIOPin sw1(0,17,true, true, true);
	DigitalIOPin sw2(1,11,true, true, true);
	DigitalIOPin sw3(1,9,true, true, true);

	LiquidCrystal lcd(&rs, &en, &d4, &d5, &d6, &d7);
	// configure display geometry
	lcd.begin(16, 2);
	bool state1 = false;
	bool state2 = false;
	bool state3 = false;

	lcd.setCursor(0, 0);
	lcd.print({'B', '1', ' ', ' ', ' ', ' ', 'B', '2', ' ', ' ', ' ', ' ', 'B', '3'});
	while(1) {
		state1 = sw1.read();
		state2 = sw2.read();
		state3 = sw3.read();
		writeLCD(lcd, state1, state2, state3);
	}
	return 0;
}
