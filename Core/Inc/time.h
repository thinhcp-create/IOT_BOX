/*
 * time.h
 *
 *  Created on: Dec 10, 2024
 *      Author: Salmon1611
 */

#ifndef TIME_H_
#define TIME_H_
#include "stdint.h"
#include "main.h"

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} Time;

Time decreaseTimeSeconds(Time t, uint8_t second_down) ;

#endif /* INC_TIME_H_ */
