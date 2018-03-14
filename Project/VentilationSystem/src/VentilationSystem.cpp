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
#include <string.h>

#include "ModbusMaster.h"
#include "ITMwraper.h"
#include "I2C.h"
#include "LiquidCrystal.h"
#include "NewSerialPort.h"


using namespace std;

#define TICKRATE_HZ1 1000
static volatile int counter;
static volatile int timeCounter;

static volatile uint32_t systicks;
const uint8_t STATE_START  = 0;
const uint8_t STATE_AUTOMODE  = 1;
const uint8_t STATE_MANUALMODE  = 2;
const uint8_t STATE_ERROR  = 3;
const int16_t MARGIN = 1;
const uint8_t ARRAY_SIZE  = 11;

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
	if(timeCounter > 0) timeCounter--;
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

bool readPressure(int16_t& pressure) {
	ITM_wraper itm;
	I2C i2c(0, 100000);

	uint8_t pressureData[3];
	uint8_t readPressureCmd = 0xF1;

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
		return false;
	}

	return true;
}


bool setFrequency(ModbusMaster& node, uint16_t freq) {
	uint8_t result;
	int ctr;
	bool atSetpoint;
	//const int delay = 500;
	const int delay = 1; // original value = 500; should be higher so setFreq can work with speed = 0;

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

int32_t setSpeed(int32_t newSpeed) {
	if (newSpeed > 20000) {
		return 20000;
	} else if (newSpeed < 0) {
		return 0;
	} else {
		return newSpeed;
	}
}

void printStep(uint8_t state, uint8_t step, LiquidCrystal lcd) {
	if (state==STATE_AUTOMODE) {
		lcd.setCursor(13,1);
		if (step < 10) {
			lcd.print("-+" + to_string(step));
		} else {
			lcd.print("!" + to_string(step));
		}

	} else {
		lcd.setCursor(11,1);
		if (step>=2) {

			lcd.print("-+" + to_string(step*5));
		} else {
			lcd.print("-+ " + to_string(step*5));
		}
	}
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
	DigitalIOPin btn1(0,24,true, true, true);
	DigitalIOPin btn2(0,0,true, true, true);
	DigitalIOPin btn3(1,3,true, true, true);
	DigitalIOPin btn4(0,16,true, true, true);
	NewSerialPort srl;

	bool btn1State,btn2State,btn3State,btn4State = false;
	int16_t Kp, Ki, Kd, error, lastError, integral , derivative = 0;
	int32_t speed = 0;
	uint8_t step  = 1;
	integral = 0;
	Kp = 120;  //200
	//50: 120
	Ki = 10;
	Kd = 100;	//100
	int16_t pressure = 0;
	int16_t targetPressure = 40;
	int16_t pressureArray[ARRAY_SIZE] = {0};
	int16_t pressureArraySorted[ARRAY_SIZE] = {0};
	int c = 0;

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
	char bufferLCD[17] = {0};
	ITM_wraper itm;
	bool isError = false, isOff = false;

	timeCounter = 15000;
	lcd.setCursor(13,1);
	lcd.print("-+" + to_string(step));
	while (1) {
		//********************VOICE CONTROL********************
		c = srl.read();
		if(c != EOF) {
			if (c == '1') {
				step = 1;
			} else if (c == '2'){
				step = 2;
			} else if (c == '3') {
				step = 3;
			} else if (c == '4') {
				step = 4;
			} else if (c == '5') {
				step = 5;
			} else if (c == '6') {
				step = 6;
			} else if (c == '7') {
				step = 7;
			} else if (c == '8') {
				step = 8;
			} else if (c == '9') {
				step = 9;
			} else if (c == 'T') {
				step = 10;
			} else if (c == 'X') {
				isOff = true;
			} else if (c == 'O') {
				if (isOff) {
					lcd.clear();
				}
				isOff = false;
			} else if (c == 'A') {
				if (state==STATE_MANUALMODE) {
					state = STATE_AUTOMODE;
					lcd.clear();
					speed = 0;
					integral = 0;
					error = 0;
					lastError = 0;
					timeCounter = 15000;
				}
			} else if (c == 'M') {
				if (state==STATE_AUTOMODE) {
					state = STATE_MANUALMODE;
					lcd.clear();
					speed = 0;
					integral = 0;
					error = 0;
					lastError = 0;
				}
			}
			printStep(state, step, lcd);
		}
		//********************VOICE CONTROL********************

		if (!isOff) {
			//********************AUTO MODE********************
			if (state==STATE_AUTOMODE) {
				if ((pressureArraySorted[ARRAY_SIZE/2] != targetPressure + MARGIN)&&(pressureArraySorted[ARRAY_SIZE/2] != targetPressure - MARGIN) && pressureArraySorted[ARRAY_SIZE/2] != targetPressure && !isError) {
					error = targetPressure - pressureArraySorted[ARRAY_SIZE/2];
					integral+= error;
					derivative= error - lastError;
					lastError = error;

					speed =  Kp*error + Ki*integral + Kd*derivative;
					speed = setSpeed(speed);

					if (timeCounter<=0) {
						isError = true;
						lcd.setCursor(0,0);
						lcd.print("ERROR! TIME OUT!");
						speed = 0;
						integral = 0;
						timeCounter=0;
					}

				} else {
					timeCounter = 15000; //15 seconds
					//lcd.setCursor(0,0);
					//lcd.print("                ");
				}
				sprintf(buffer,"error: %3d, integral: %5d, derivative %3d, P: %6d, I: %6d, D: %6d, speed %6ld\n", error, integral, derivative, Kp*error, Ki*integral, Kd*derivative, speed);
				itm.print(buffer);
				sprintf(bufferLCD, "P=%3d Tgt:%3d", pressureArraySorted[ARRAY_SIZE/2], targetPressure);
				lcd.setCursor(0,1);
				lcd.print(bufferLCD);
			}

			btn1State = btn1.read();
			while (btn1State) {
				btn1State = btn1.read();
				if (!btn1State) {
					step+=1;
					if (step>=4) {
						step= 1;
					}
					printStep(state, step, lcd);
				}
			}

			btn2State = btn2.read();
			while (btn2State) {
				btn2State = btn2.read();
				if (!btn2State) {
					lcd.clear();
					speed = 0;
					integral = 0;
					error = 0;
					lastError = 0;
					if (state==STATE_AUTOMODE) {
						state = STATE_MANUALMODE;
						printStep(state, step, lcd);
					} else if (state==STATE_MANUALMODE) {
						timeCounter = 15000;
						state = STATE_AUTOMODE;
						printStep(state, step, lcd);
					}
				}
			}

			btn3State = btn3.read();
			while (btn3State) {
				btn3State = btn3.read();
				if (!btn3State) {
					if (state==STATE_AUTOMODE) {
						targetPressure-=step;
						if (targetPressure < 0) {
							targetPressure = 0;
						}
					} else if (state==STATE_MANUALMODE){
						speed = setSpeed(speed - step*1000);
					} else {

					}
				}
			}

			btn4State = btn4.read();
			while (btn4State) {
				btn4State = btn4.read();
				if (!btn4State) {
					if (state==STATE_AUTOMODE) {
						targetPressure+=step;
						if (targetPressure > 128) {
							targetPressure = 128;
						}
					} else if (state==STATE_MANUALMODE){
						speed = setSpeed(speed + step*1000);
					} else {

					}
				}
			}

			int16_t pressure = 0;
			bool readPressureOk = readPressure(pressure);

			//***********GET MEDIAN VALUE***********
			// Load pressure to array
			if (readPressureOk) {
				for (uint8_t i=0; i < ARRAY_SIZE; i++) {
					if (i==ARRAY_SIZE-1) {
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
			}

			//********************MANUAL MODE********************
			if (state==STATE_MANUALMODE) {
				timeCounter = 15000;
				lcd.setCursor(0,0);
				memset(bufferLCD, 0, sizeof(bufferLCD));

				if (readPressureOk) {
					sprintf(bufferLCD, "Pressure:%4d", pressureArraySorted[ARRAY_SIZE/2]);
				} else {
					sprintf(buffer, "P: ?");
				}

				lcd.print(bufferLCD);
				lcd.setCursor(0, 1);

				memset(bufferLCD, 0, sizeof(bufferLCD));
				sprintf(bufferLCD, "Speed: %3d%%", speed / 200);
				lcd.print(bufferLCD);
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
			} else {
				printf("ctr=%d\n",j);
			}

			Sleep(1);
			// frequency is scaled:
			// 20000 = 50 Hz, 0 = 0 Hz, linear scale 400 units/Hz
			// Set the fan speed
			setFrequency(node, speed);
		} else {
			setFrequency(node, 0);
			timeCounter = 15000;
			integral = 0;
			error = 0;
			lastError = 0;
			lcd.setCursor(0,0);
			lcd.print("System is off!  ");
			lcd.setCursor(0,1);
			lcd.print("Say ON to start!");
		}
		//***********END RUNNING THE FAN***********
	}
}
