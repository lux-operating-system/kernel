/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#pragma once

#include <sys/types.h>
#include <platform/x86_64.h>

#define CMOS_INDEX              0x70
#define CMOS_DATA               0x71

#define CMOS_RTC_SECS           0x00
#define CMOS_RTC_MINS           0x02
#define CMOS_RTC_HOURS          0x04
#define CMOS_RTC_DAY            0x07
#define CMOS_RTC_MONTH          0x08
#define CMOS_RTC_YEAR           0x09
#define CMOS_RTC_STATUS_A       0x0A
#define CMOS_RTC_STATUS_B       0x0B

#define CMOS_STATUS_A_UPDATE    0x80
#define CMOS_STATUS_B_24HR      0x02
#define CMOS_STATUS_B_BINARY    0x04

uint8_t cmosRead(uint8_t);
void cmosWrite(uint8_t, uint8_t);
