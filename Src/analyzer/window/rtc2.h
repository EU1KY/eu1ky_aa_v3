/*
 * DS RTC Library: DS1307 and DS3231 driver library
 * (C) 2011 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

#ifndef DS1307_H
#define DS1307_H

#include <stdbool.h>
#include "config.h"
//#include "twi.h"

#define DS1307_SLAVE_ADDR 0b11010000

/** Time structure
 *
 * Both 24-hour and 12-hour time is stored, and is always updated when rtc_get_time is called.
 *
 * When setting time and alarm, 24-hour mode is always used.
 *
 * If you run your clock in 12-hour mode:
 * - set time hour to store in twelveHour and set am to true or false.
 * - call rtc_12h_translate (this will put the correct value in hour, so you don't have to
 *   calculate it yourself.
 * - call rtc_set_alarm or rtc_set_clock
 *
 * Note that rtc_set_clock_s, rtc_set_alarm_s, rtc_get_time_s, rtc_set_alarm_s always operate in 24-hour mode
 * and translation has to be done manually (you can call rtc_24h_to_12h to perform the calculation)
 *
 */
struct tm {
	int sec;      // 0 to 59
	int min;      // 0 to 59
	int hour;     // 0 to 23
	int mday;     // 1 to 31
	int mon;      // 1 to 12
	int year;     // year-99
	int wday;     // 1-7

    // 12-hour clock data
    bool am; // true for AM, false for PM
    int twelveHour; // 12 hour clock time
};

// statically allocated
extern struct tm _tm;

// Initialize the RTC and autodetect type (DS1307 or DS3231)
void rtc_init(void);

// Autodetection
bool rtc_is_ds1307(void);
bool rtc_is_ds3231(void);

void rtc_set_ds1307(void);
void rtc_set_ds3231(void);

// Get/set time
// Gets the time: Supports both 24-hour and 12-hour mode
struct tm* rtc_get_time(void);
// Gets the time: 24-hour mode only
void rtc_get_time_s(uint8_t* hour, uint8_t* min, uint8_t* sec);
// Sets the time: Supports both 24-hour and 12-hour mode
void rtc_set_time(struct tm* tm_);
// Sets the time: Supports 12-hour mode only
void rtc_set_time_s(uint8_t hour, uint8_t min, uint8_t sec);

// start/stop clock running (DS1307 only)
void rtc_run_clock(bool run);
bool rtc_is_clock_running(void);

// Read Temperature (DS3231 only)
void  ds3231_get_temp_int(int8_t* i, uint8_t* f);
void rtc_force_temp_conversion(uint8_t block);

// SRAM read/write DS1307 only
void rtc_get_sram(uint8_t* data);
void rtc_set_sram(uint8_t *data);
uint8_t rtc_get_sram_byte(uint8_t offset);
void rtc_set_sram_byte(uint8_t b, uint8_t offset);

  // Auxillary functions
enum RTC_SQW_FREQ { FREQ_1 = 0, FREQ_1024, FREQ_4096, FREQ_8192 };

void rtc_SQW_enable(bool enable);
void rtc_SQW_set_freq(enum RTC_SQW_FREQ freq);
void rtc_osc32kHz_enable(bool enable);

// Alarm functionality
void rtc_reset_alarm(void);
void rtc_set_alarm(struct tm* tm_);
void rtc_set_alarm_s(uint8_t hour, uint8_t min, uint8_t sec);
struct tm* rtc_get_alarm(void);
void rtc_get_alarm_s(uint8_t* hour, uint8_t* min, uint8_t* sec);
bool rtc_check_alarm(void);

#endif
