#include "unity.h"
#include <time.h>
#include <stdint.h>
#include "rtc.h"

// Extern function from main/timegm.c
extern time_t timegm(struct tm *tm);
extern uint8_t RTC_Get_Week(uint16_t year, uint8_t month, uint8_t day);



void test_timegm_epoch_start(void) {
    struct tm t = {0};
    t.tm_year = 1970 - 1900; // 70
    t.tm_mon = 0;            // Jan
    t.tm_mday = 1;           // 1st
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;

    time_t result = timegm(&t);
    TEST_ASSERT_EQUAL_INT64(0, (int64_t)result);
}

void test_timegm_known_date(void) {
    struct tm t = {0};
    // 2026-06-23 14:30:00 UTC
    t.tm_year = 2026 - 1900; // 126
    t.tm_mon = 5;            // June (0-indexed)
    t.tm_mday = 23;
    t.tm_hour = 14;
    t.tm_min = 30;
    t.tm_sec = 0;

    // Expected timestamp: 1782225000
    // Let's verify: 
    // June 23, 2026 14:30:00 UTC = 1782225000
    time_t result = timegm(&t);
    TEST_ASSERT_EQUAL_INT64(1782225000, (int64_t)result);
}

void test_timegm_invalid_month(void) {
    struct tm t = {0};
    t.tm_year = 126;
    t.tm_mon = 12; // Invalid month (0-11 is valid)
    t.tm_mday = 1;

    time_t result = timegm(&t);
    TEST_ASSERT_EQUAL_INT64(-1, (int64_t)result);
}

void test_timegm_before_epoch(void) {
    struct tm t = {0};
    t.tm_year = 1969 - 1900; // 69 (before 1970)
    t.tm_mon = 11;
    t.tm_mday = 31;

    time_t result = timegm(&t);
    TEST_ASSERT_EQUAL_INT64(-1, (int64_t)result);
}

void test_rtc_get_week_known_friday(void) {
    /* 2026-11-06 is Friday. 0 = Sunday … 6 = Saturday. */
    TEST_ASSERT_EQUAL_UINT8(5, RTC_Get_Week(2026, 11, 6));
}

void test_rtc_get_week_invalid_month(void) {
    TEST_ASSERT_EQUAL_UINT8(0, RTC_Get_Week(2026, 0, 1));
    TEST_ASSERT_EQUAL_UINT8(0, RTC_Get_Week(2026, 13, 1));
}

void test_pcf_set_datetime_and_system_rejects_null(void) {
    TEST_ASSERT_EQUAL_INT(-2, PCF_SetDateTimeAndSystem(NULL));
}
