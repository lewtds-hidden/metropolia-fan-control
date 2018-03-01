/*
 * ITMwraper.cpp
 *
 *  Created on: Mar 1, 2018
 *      Author: Usin
 */

#include "ITMwraper.h"

ITM_wraper::ITM_wraper() {
	ITM_init();
}


void ITM_wraper::print(const char *myCha){
	ITM_write(myCha);
}

void ITM_wraper::print(int number){
	char buffer [16];
	snprintf(buffer, 16, "%d", number);
	ITM_write(buffer);
}

void ITM_wraper::print(std::string str){
	const char *myChar = str.c_str();
	ITM_write(myChar);
}

