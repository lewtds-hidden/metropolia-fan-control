/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

#include "ModbusMaster.h"
#include "ITMwraper.h"
#include "I2C.h"
#include "LiquidCrystal.h"


using namespace std;

#define TICKRATE_HZ1 1000
static volatile int counter;
static volatile uint32_t systicks;
const uint8_t STATE_START  = 0;
const uint8_t STATE_AUTOMODE  = 1;
const uint8_t STATE_MANUALMODE  = 2;
const uint8_t STATE_ERROR  = 3;
const int16_t MARGIN = 1;
const uint8_t ARRAY_SIZE  = 7;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief	Handle interrupt from SysTick timer
 * @return	Nothing
 */
void SysTick_Handler(void)
{
	systicks++;
	if(counter > 0) counter--;
}
#ifdef __cplusplus
}
#endif

void Sleep(int ms)
{
	counter = ms;
	while(counter > 0) {
		__WFI();
	}
}

/* this function is required by the modbus library */
uint32_t millis() {
	return systicks;
}

void printRegister(ModbusMaster& node, uint16_t reg) {
	uint8_t result;
	// slave: read 16-bit registers starting at reg to RX buffer
	result = node.readHoldingRegisters(reg, 1);

	// do something with data if read is successful
	if (result == node.ku8MBSuccess)
	{
		printf("R%d=%04X\n", reg, node.getResponseBuffer(0));
	}
	else {
		printf("R%d=???\n", reg);
	}
}

int16_t readPressure() {
	ITM_wraper itm;
	I2C i2c(0, 100000);


	uint8_t pressureData[3];
	uint8_t readPressureCmd = 0xF1;
	int16_t pressure = 0;


	if (i2c.transaction(0x40, &readPressureCmd, 1, pressureData, 3)) {
		pressure = (pressureData[0] << 8) | pressureData[1];
		pressure = pressure*0.95/240;
		itm.print("Pressure data: ");
		itm.print(pressure);
		itm.print("\n");
	}
	else {
		itm.print("Error reading pressure.\r\n");
		pressure = 0;
	}
	return pressure;
}


bool setFrequency(ModbusMaster& node, uint16_t freq) {
	uint8_t result;
	int ctr;
	bool atSetpoint;
	//const int delay = 500;
	const int delay = 1;

	node.writeSingleRegister(1, freq); // set motor frequency

	printf("Set freq = %d\n", freq/40); // for debugging

	// wait until we reach set point or timeout occurs
	ctr = 0;
	atSetpoint = false;
	do {
		Sleep(delay);
		// read status word
		result = node.readHoldingRegisters(3, 1);
		// check if we are at setpoint
		if (result == node.ku8MBSuccess) {
			if(node.getResponseBuffer(0) & 0x0100) atSetpoint = true;
		}
		ctr++;
	} while(ctr < 20 && !atSetpoint);

	printf("Elapsed: %d\n", ctr * delay); // for debugging

	return atSetpoint;
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
	Board_LED_Set(0, true);
#endif
#endif

	/* The sysTick counter only has 24 bits of precision, so it will
	overflow quickly with a fast core clock. You can alter the
	sysTick divider to generate slower sysTick clock rates. */
	Chip_Clock_SetSysTickClockDiv(1);

	SysTick_Config(Chip_Clock_GetSysTickClockRate()/TICKRATE_HZ1);

	//***********START For setFrequency***********
	ModbusMaster node(2); // Create modbus object that connects to slave id 2

	node.begin(9600); // set transmission rate - other parameters are set inside the object and can't be changed here

	printRegister(node, 3); // for debugging

	node.writeSingleRegister(0, 0x0406); // prepare for starting

	printRegister(node, 3); // for debugging

	Sleep(1000); // give converter some time to set up
	// note: we should have a startup state machine that check converter status and acts per current status
	//       but we take the easy way out and just wait a while and hope that everything goes well

	printRegister(node, 3); // for debugging

	node.writeSingleRegister(0, 0x047F); // set drive to start mode

	printRegister(node, 3); // for debugging

	Sleep(1000); // give converter some time to set up
	// note: we should have a startup state machine that check converter status and acts per current status
	//       but we take the easy way out and just wait a while and hope that everything goes well

	printRegister(node, 3); // for debugging
	int j = 0;
	//***********END For setFrequency***********

	//From left to right:
	DigitalIOPin btn1(0,0,true, true, true);
	DigitalIOPin btn2(1,3,true, true, true);
	DigitalIOPin btn3(0,16,true, true, true);
	DigitalIOPin btn4(0,10,true, true, true);

	bool btn1State,btn2State,btn3State,btn4State = false;
	int16_t Kp, Ki, Kd, error, lastError, integral , derivative = 0;
	int32_t speed = 0;
	integral = 0;
	Kp = 200;
	Ki = 20;
	Kd = 100;
	int16_t pressure = 0;
	int16_t pressureArray[ARRAY_SIZE] = {0};
	int16_t pressureArraySorted[ARRAY_SIZE] = {0};


	int16_t targetPressure = 40;
	//***********LCD***********
	Chip_RIT_Init(LPC_RITIMER); // initialize RIT (enable clocking etc.)
	DigitalIOPin rs(0,8,false, true, false);
	DigitalIOPin en(1,6,false, true, false);
	DigitalIOPin d4(1,8,false, true, false);
	DigitalIOPin d5(0,5,false, true, false);
	DigitalIOPin d6(0,6,false, true, false);
	DigitalIOPin d7(0,7,false, true, false);
	LiquidCrystal lcd(&rs, &en, &d4, &d5, &d6, &d7);
	// configure display geometry
	lcd.begin(16, 2);
	lcd.clear();

	uint8_t state = STATE_AUTOMODE;
	char buffer[120] = {0};
	char bufferLCD[10] = {0};
	ITM_wraper itm;
	while (1) {

		if (state==STATE_AUTOMODE) {
			if ((pressure != targetPressure + MARGIN)&&(pressure != targetPressure - MARGIN) && pressure != targetPressure) {
				error = targetPressure - pressureArraySorted[3];
				integral+= error;
				derivative= error - lastError;
				lastError = error;
				if (integral > 500) integral = 500;
				if (integral < -500) integral = -500;
				speed = 10000 + Kp*error + Ki*integral + Kd*derivative;
				if (speed> 20000) speed = 20000;
				if (speed < 0) speed = 0;
			}
			sprintf(buffer,"error: %3d, intergral: %5d, derivative %3d, P: %6d, I: %6d, D: %6d, speed %6d \n", error, integral, derivative, Kp*error, Ki*integral, Kd*derivative, speed);
			itm.print(buffer);
			sprintf(bufferLCD, "P=%3d Target:%3d", pressureArraySorted[3], targetPressure);
			lcd.setCursor(0,1);
			lcd.print(bufferLCD);

		}
		btn1State = btn1.read();
		while (btn1State) {
			btn1State = btn1.read();
			if (!btn1State) {
				// Voice control here?
			}
		}

		btn2State = btn2.read();
		while (btn2State) {
			btn2State = btn2.read();
			if (!btn2State) {
				if (state==STATE_AUTOMODE) {
					state = STATE_MANUALMODE;
				} else if (state==STATE_MANUALMODE) {
					state = STATE_AUTOMODE;
				} else {
					state=STATE_AUTOMODE;
				}
			}
		}

		btn3State = btn3.read();
		while (btn3State) {
			btn3State = btn3.read();
			if (!btn3State) {
				speed+=100;
			}
		}

		btn4State = btn4.read();
		while (btn4State) {
			btn4State = btn4.read();
			if (!btn4State) {
				speed-=100;
			}
		}

		pressure = readPressure();

		//***********GET MEDIAN VALUE***********
		// Load pressure to array
		for (uint8_t i=0; i < ARRAY_SIZE; i++) {
			if (i==6) {
				pressureArray[i] = pressure;
			} else {
				pressureArray[i] = pressureArray[i+1];
			}
			pressureArraySorted[i] = pressureArray[i];
		}

		// Sort array
		for(int x=0; x<ARRAY_SIZE; x++){
			for(int y=0; y<ARRAY_SIZE-1; y++){
				if(pressureArraySorted[y]>pressureArraySorted[y+1]){
					int temp = pressureArraySorted[y+1];
					pressureArraySorted[y+1] = pressureArraySorted[y];
					pressureArraySorted[y] = temp;
				}
			}
		}

		//***********START RUNNING THE FAN***********
		uint8_t result;
		// slave: read (2) 16-bit registers starting at register 102 to RX buffer
		j = 0;
		do {
			result = node.readHoldingRegisters(102, 2);
			j++;
		} while(j < 3 && result != node.ku8MBSuccess);
		// note: sometimes we don't succeed on first read so we try up to threee times
		// if read is successful print frequency and current (scaled values)
		if (result == node.ku8MBSuccess) {
			printf("F=%4d, I=%4d  (ctr=%d)\n", node.getResponseBuffer(0), node.getResponseBuffer(1),j);
		}
		else {
			printf("ctr=%d\n",j);
		}

		Sleep(1);

		// frequency is scaled:
		// 20000 = 50 Hz, 0 = 0 Hz, linear scale 400 units/Hz
		// Set the fan speed

		setFrequency(node, speed);
		//***********END RUNNING THE FAN***********

		sprintf(buffer, "%5d ", speed);
		lcd.setCursor(0,0);
		lcd.print(buffer);
	}
}
