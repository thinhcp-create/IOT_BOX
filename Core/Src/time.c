/*
 * time.c
 *
 *  Created on: Dec 10, 2024
 *      Author: Salmon1611
 */
#include "time.h"

// Kiểm tra xem năm có phải là năm nhuận hay không
uint8_t is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Hàm trả về số ngày trong tháng
uint8_t days_in_month(uint8_t month, uint16_t year) {
    // Mảng số ngày trong các tháng (tháng 2 mặc định là 28 ngày)
	uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // Nếu là tháng 2 và năm nhuận thì trả về 29 ngày
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }

    // Trả về số ngày của tháng
    return days[month - 1];
}

TimeDifference calculate_time_difference(Time t1, Time t2) {
    TimeDifference diff = {0};

    if (t2.second < t1.second) {
        t2.second += 60;
        t2.minute -= 1;
    }
    diff.seconds = t2.second - t1.second;

    if (t2.minute < t1.minute) {
        t2.minute += 60;
        t2.hour -= 1;
    }
    diff.minutes = t2.minute - t1.minute;

    if (t2.hour < t1.hour) {
        t2.hour += 24;
        t2.day -= 1;
    }
    diff.hours = t2.hour - t1.hour;

    if (t2.day < t1.day) {
        // Giả sử hàm days_in_month trả về số ngày trong tháng
        t2.day += days_in_month(t1.month, t1.year);
        t2.month -= 1;
    }
    diff.days = t2.day - t1.day;

    if (t2.month < t1.month) {
        t2.month += 12;
        t2.year -= 1;
    }
    diff.months = t2.month - t1.month;

    diff.years = t2.year - t1.year;

    return diff;
}

Time add_time_difference(Time base, TimeDifference diff) {
    Time result = base;

    // Cộng giây
    result.second += diff.seconds;
    if (result.second >= 60) {
        result.minute += result.second / 60;
        result.second %= 60;
    }

    // Cộng phút
    result.minute += diff.minutes;
    if (result.minute >= 60) {
        result.hour += result.minute / 60;
        result.minute %= 60;
    }

    // Cộng giờ
    result.hour += diff.hours;
    if (result.hour >= 24) {
        result.day += result.hour / 24;
        result.hour %= 24;
    }

    // Cộng ngày
    result.day += diff.days;
    while (result.day > days_in_month(result.month, result.year)) {
        result.day -= days_in_month(result.month, result.year);
        result.month++;
        if (result.month > 12) {
            result.month = 1;
            result.year++;
        }
    }

    // Cộng tháng
    result.month += diff.months;
    while (result.month > 12) {
        result.month -= 12;
        result.year++;
    }

    // Cộng năm
    result.year += diff.years;

    return result;
}

