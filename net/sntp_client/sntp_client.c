/**
 ******************************************************************************
 * @file    sntp_client
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   Time sync with MTP server
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */

#include <time.h>
#include "mico.h"
#include "sntp.h"

#define TIME_SYNC_PERIOD    (1 * SECONDS)

#define sntp_demo_log(M, ...) custom_log("SNTP DEMO", M, ##__VA_ARGS__)

static mico_semaphore_t wifi_sem;
void appNotify_WifiStatusHandler(WiFiEvent status, void *const inContext)
{
    switch (status)
    {
    case NOTIFY_STATION_UP:
        sntp_demo_log("Wi-Fi connected.");
        mico_rtos_set_semaphore(&wifi_sem);
        break;
    case NOTIFY_STATION_DOWN:
        sntp_demo_log("Wi-Fi disconnected.");
        break;
    default:
        break;
    }
}

/**/
static void read_utc_time_from_rtc( struct tm *utc_time )
{
    mico_rtc_time_t time;

    /*Read current time from RTC.*/
    if ( MicoRtcGetTime( &time ) == kNoErr )
    {
        utc_time->tm_sec = time.sec;
        utc_time->tm_min = time.min;
        utc_time->tm_hour = time.hr;
        utc_time->tm_mday = time.date;
        utc_time->tm_wday = time.weekday;
        utc_time->tm_mon = time.month - 1;
        utc_time->tm_year = time.year + 100;
        sntp_demo_log("Current RTC Time: %s",asctime(utc_time));
    }
    else
    {
        sntp_demo_log("RTC function unsupported");
    }
}

/* Callback function when MiCO UTC time in sync to NTP server */
static void sntp_time_synced( void )
{
    struct tm *     currentTime;
    iso8601_time_t  iso8601_time;
    mico_utc_time_t utc_time;
    mico_rtc_time_t rtc_time;

    mico_time_get_iso8601_time( &iso8601_time );
    sntp_demo_log("sntp_time_synced: %.26s", (char*)&iso8601_time);

    mico_time_get_utc_time( &utc_time );

    currentTime = localtime( (const time_t *)&utc_time );
    rtc_time.sec = currentTime->tm_sec;
    rtc_time.min = currentTime->tm_min;
    rtc_time.hr = currentTime->tm_hour;

    rtc_time.date = currentTime->tm_mday;
    rtc_time.weekday = currentTime->tm_wday;
    rtc_time.month = currentTime->tm_mon + 1;
    rtc_time.year = (currentTime->tm_year + 1900) % 100;

    MicoRtcSetTime( &rtc_time );
}

int application_start( void )
{
    OSStatus           err = kNoErr;
    struct tm          utc_time;
    mico_utc_time_ms_t utc_time_ms;

    err = mico_rtos_init_semaphore(&wifi_sem, 1);
    require_noerr(err, exit);

    /* Register user function for MiCO notification: WiFi status changed */
    err = mico_system_notify_register(mico_notify_WIFI_STATUS_CHANGED, (void *)appNotify_WifiStatusHandler, NULL);
    require_noerr(err, exit);

    /* Read UTC time from RTC hardware */
    read_utc_time_from_rtc( &utc_time );

    /* Set UTC time to MiCO system */
    utc_time_ms = (uint64_t) mktime( &utc_time ) * (uint64_t) 1000;
    mico_time_set_utc_time_ms( &utc_time_ms );

    /* Start MiCO system functions according to mico_config.h*/
    err = mico_system_init( mico_system_context_init( 0 ) );
    require_noerr( err, exit );

    /* wait for wifi on */
    mico_rtos_get_semaphore(&wifi_sem, MICO_WAIT_FOREVER);

    /* Start auto sync with NTP server */
    sntp_start_auto_time_sync( TIME_SYNC_PERIOD, sntp_time_synced );

    /* Print current time from MiCO system every 10 seconds */
//    while ( 1 )
//    {
//        mico_time_get_iso8601_time( &iso8601_time );
//        sntp_demo_log("Current time: %.26s", (char*)&iso8601_time);
//        mico_rtos_delay_milliseconds( 10 * 1000 );
//    }

    exit:
    sntp_demo_log("mico_rtos_delete_thread");
    mico_rtos_delete_thread( NULL );
    return 0;
}

