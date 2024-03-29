/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "bacnet.h" // added by chelsea
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "timesync.h"
#include "handlers.h"

/** @file h_ts.c  Handles TimeSync requests. */
#if BAC_TIMESYNC

#if PRINT_ENABLED
static void show_bacnet_date_time(
    BACNET_DATE * bdate,
    BACNET_TIME * btime)
{
    /* show the date received */
    fprintf(stderr, "%u", (unsigned) bdate->year);
    fprintf(stderr, "/%u", (unsigned) bdate->month);
    fprintf(stderr, "/%u", (unsigned) bdate->day);
    /* show the time received */
    fprintf(stderr, " %02u", (unsigned) btime->hour);
    fprintf(stderr, ":%02u", (unsigned) btime->min);
    fprintf(stderr, ":%02u", (unsigned) btime->sec);
    fprintf(stderr, ".%02u", (unsigned) btime->hundredths);
    fprintf(stderr, "\r\n");
}
#endif

void handler_timesync(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src)
{
    int len = 0;
    BACNET_DATE bdate;
    BACNET_TIME btime;

    (void) src;
    (void) service_len;
    len =
        timesync_decode_service_request(service_request, service_len, &bdate,
        &btime);
    if (len > 0) {
#if PRINT_ENABLED
        fprintf(stderr, "Received TimeSyncronization Request\r\n");
        show_bacnet_date_time(&bdate, &btime);
#endif
        /* FIXME: set the time? */
			// decode bacnet_data and bacnet_time

			if(service_request[0] == 0xa4 && service_request[5] == 0xb4) // date & time
			{
				Rtc.Clk.year = service_request[1] - 100;
				Rtc.Clk.mon = service_request[2];
				Rtc.Clk.day = service_request[3];
				//Rtc.Clk.day_of_year = 
				
				Rtc.Clk.hour = service_request[6];
				Rtc.Clk.min = service_request[7];
				Rtc.Clk.sec = service_request[8];
				Rtc_Set(Rtc.Clk.year,Rtc.Clk.mon,Rtc.Clk.day,Rtc.Clk.hour,Rtc.Clk.min,Rtc.Clk.sec,0);
#if ASIX
					flag_Updata_Clock = 1;
//				Updata_Clock(0);
#endif

			}
			
    }

    return;
}

void handler_timesync_utc(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src)
{
    int len = 0;
    BACNET_DATE bdate;
    BACNET_TIME btime;

    (void) src;
    (void) service_len;
    len =
        timesync_decode_service_request(service_request, service_len, &bdate,
        &btime);
    if (len > 0) {
#if PRINT_ENABLED
        fprintf(stderr, "Received TimeSyncronization Request\r\n");
        show_bacnet_date_time(&bdate, &btime);
#endif
        /* FIXME: set the time? */
			if(service_request[0] == 0xa4 && service_request[5] == 0xb4) // date & time
			{
				Rtc.Clk.year = service_request[1] - 100;
				Rtc.Clk.mon = service_request[2];
				Rtc.Clk.day = service_request[3];
				//Rtc.Clk.day_of_year = 
				
				Rtc.Clk.hour = service_request[6];
				Rtc.Clk.min = service_request[7];
				Rtc.Clk.sec = service_request[8];
				Rtc_Set(Rtc.Clk.year,Rtc.Clk.mon,Rtc.Clk.day,Rtc.Clk.hour,Rtc.Clk.min,Rtc.Clk.sec,0);
#if ASIX
					flag_Updata_Clock = 1;
//				Updata_Clock(0);
#endif

			}	
    }

    return;
}

#endif