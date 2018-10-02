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

// TODO: insert other include files here

// TODO: insert other definitions and declarations here

#include <cstring>
#include <cstdio>

#include "ModbusMaster.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/


/*****************************************************************************
 * Public functions
 ****************************************************************************/
static volatile int counter;
static volatile uint32_t systicks;

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

/**
 * @brief	Main UART program body
 * @return	Always returns 1
 */
int main(void)
{

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

	/* Set up SWO to PIO1_2 */
	Chip_SWM_MovablePortPinAssign(SWM_SWO_O, 1, 2); // Needed for SWO printf

	/* Enable and setup SysTick Timer at a periodic rate */
	SysTick_Config(SystemCoreClock / 1000);

	Board_LED_Set(0, false);
	Board_LED_Set(1, true);
	printf("Started\n");

	ModbusMaster node(2); // Create modbus object that connects to slave id 2

	node.begin(9600); // set transmission rate - other parameters are set inside the object and can be changed here

	while (1) {
		static uint32_t i;
		uint8_t j, result;
		uint16_t data[6];

		for(j = 0; j < 6; j++) {
			i++;
			// set word(j) of TX buffer to least-significant word of counter (bits 15..0)
			node.setTransmitBuffer(j, i & 0xFFFF);
		}
		// slave: write TX buffer to (6) 16-bit registers starting at register 0
		result = node.writeMultipleRegisters(0, 6);

		// slave: read (6) 16-bit registers starting at register 2 to RX buffer
		result = node.readHoldingRegisters(2, 6);

		// do something with data if read is successful
		if (result == node.ku8MBSuccess)
		{
			for (j = 0; j < 6; j++)
			{
				data[j] = node.getResponseBuffer(j);
			}
			printf("%6d, %6d, %6d, %6d, %6d, %6d\n", data[0], data[1], data[2], data[3], data[4], data[5]);
		}
		Sleep(1000);
	}


	return 1;
}

