/*
 * ITMwraper.h
 *
 *  Created on: Mar 1, 2018
 *      Author: Usin
 */

#ifndef ITMWRAPER_H_
#define ITMWRAPER_H_

#include "stdint.h"
#include <ctime>
#include "ITM_write.h"
#include "stdio.h"
#include "string"

class ITM_wraper{
public:
	ITM_wraper();
	void print(const char *myCha);
	void print(int number);
	void print(std::string str);
};

#endif /* ITMWRAPER_H_ */
