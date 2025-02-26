/*
 * time.c
 *
 *  Created on: Dec 10, 2024
 *      Author: Salmon1611
 */
#include "time.h"
#include "FLASH_PAGE_F1.h"

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

Time decreaseTimeSeconds(Time t, uint8_t second_down) {
    // Đảm bảo giá trị second_down hợp lệ
    if (second_down > 60) {
        second_down = 60;
    }

    // Giảm giây
    if (t.second < second_down) {
        second_down -= t.second;
        t.second = 60 - second_down % 60;
        t.minute--;
    } else {
        t.second -= second_down;
        return t;
    }

    // Xử lý tràn phút
    while (t.minute < 0) {
        t.minute += 60;
        t.hour--;
    }

    // Xử lý tràn giờ
    while (t.hour < 0) {
        t.hour += 24;
        t.day--;
    }

    // Xử lý tràn ngày
    while (t.day < 1) {
        if (t.month == 1) {
            t.month = 12;
            t.year--;
        } else {
            t.month--;
        }
        t.day = days_in_month(t.month, t.year);
    }

    return t;
}


