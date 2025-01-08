/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <sys/types.h>
#include <platform/lock.h>
#include <platform/cmos.h>
#include <platform/x86_64.h>
#include <platform/platform.h>

static lock_t lock = LOCK_INITIAL;
static time_t initialTimestamp = 0;
static uint64_t initialUptime = 0;
static const int daysPerMonth[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31  // common year
};

/* cmosRead(): reads from a CMOS register
 * params: index - index to read from
 * returns: value from register
 */

uint8_t cmosRead(uint8_t index) {
    outb(CMOS_INDEX, index);
    return inb(CMOS_DATA);
}

/* cmosWrite(): writes to a CMOS register
 * params: index - index to write to
 * params: value - value to write
 * returns: nothing
 */

void cmosWrite(uint8_t index, uint8_t value) {
    outb(CMOS_INDEX, index);
    outb(CMOS_DATA, value);
}

/* platformTimestamp(): returns the Unix timestamp in seconds
 * params: none
 * returns: timestamp in seconds
 */

time_t platformTimestamp() {
    if(initialTimestamp)
        return initialTimestamp + ((platformUptime()-initialUptime) / PLATFORM_TIMER_FREQUENCY);

    acquireLockBlocking(&lock);

    uint8_t format = cmosRead(CMOS_RTC_STATUS_B);
    while(cmosRead(CMOS_RTC_STATUS_A) & CMOS_STATUS_A_UPDATE);

    uint16_t year = cmosRead(CMOS_RTC_YEAR);
    uint8_t month = cmosRead(CMOS_RTC_MONTH);
    uint8_t day = cmosRead(CMOS_RTC_DAY);
    uint8_t hour = cmosRead(CMOS_RTC_HOURS);
    uint8_t pm = hour & 0x80;
    hour &= 0x7F;
    uint8_t min = cmosRead(CMOS_RTC_MINS);
    uint8_t sec = cmosRead(CMOS_RTC_SECS);

    if(!(format & CMOS_STATUS_B_BINARY)) {
        year = (((year >> 4) & 0x0F) * 10) + (year & 0x0F);
        month = (((month >> 4) & 0x0F) * 10) + (month & 0x0F);
        day = (((day >> 4) & 0x0F) * 10) + (day & 0x0F);
        hour = (((hour >> 4) & 0x0F) * 10) + (hour & 0x0F);
        min = (((min >> 4) & 0x0F) * 10) + (min & 0x0F);
        sec = (((sec >> 4) & 0x0F) * 10) + (sec & 0x0F);
    }

    if(!(format & CMOS_STATUS_B_24HR)) {
        if(hour == 12) {
            if(!pm) hour = 0;
        } else if(pm) {
            hour += 12;
        }
    }

    year += 2000;
    int yearDay = 0;
    int leap = (!(year % 4) && (year % 100)) || (!(year % 400));

    for(int i = 1; i < month; i++) {
        if(i == 2 && leap) yearDay += 29;   // special case for february
        else yearDay += daysPerMonth[i-1];
    }

    yearDay += day - 1;

    // calculation source:
    // https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
    year -= 1900;       // depends on years since 1900

    initialTimestamp = sec + (min*60) + (hour*3600) + (yearDay * 86400)
                        + ((year-70) * 31536000) + (((year-69)/4) * 86400)
                        - (((year-1)/100) * 86400) + (((year+299)/400) * 86400);
    initialUptime = platformUptime();

    releaseLock(&lock);
    return initialTimestamp;
}