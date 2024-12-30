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

typedef struct {
	int16_t years;
    int8_t months;
    int8_t days;
    int8_t hours;
    int8_t minutes;
    int8_t seconds;
} TimeDifference;

TimeDifference calculate_time_difference(Time t1, Time t2);
Time add_time_difference(Time base, TimeDifference diff);
void load_time_difference(TimeDifference* diff);

#endif /* INC_TIME_H_ */
