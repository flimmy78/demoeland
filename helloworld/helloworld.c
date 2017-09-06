/**
 ******************************************************************************
 * @file    hello_world.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   First MiCO application to say hello world!
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
 *
 ******************************************************************************
 */

#include "mico.h"

#define os_helloworld_log(format, ...) custom_log("HW", format, ##__VA_ARGS__)

static uint16_t wifi_connect_count = 0;
static uint16_t wifi_lost_count = 0;

static void micoNotify_ConnectFailedHandler(OSStatus err, void *inContext)
{
    os_helloworld_log("join Wlan failed Err: %d", err);
}

static void micoNotify_WifiStatusHandler(WiFiEvent event, void *inContext)
{
    switch (event)
    {
    case NOTIFY_STATION_UP:
        os_helloworld_log("Station up");
        wifi_connect_count++;
        break;
    case NOTIFY_STATION_DOWN:
        os_helloworld_log("Station down");
        wifi_lost_count++;
        break;
    default:
        break;
    }
}
int application_start(void)
{
    OSStatus err;
    mico_rtc_time_t cur_time = {0};
    char cur_time_print[20];
    char start_time_print[20];
    memset(cur_time_print, 0, 20);
    memset(start_time_print, 0, 20);

    /* Register user function when wlan connection status is changed */
    err = mico_system_notify_register(mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler, NULL);
    require_noerr(err, exit);

    /* Register user function when wlan connection is faile in one attempt */
    err = mico_system_notify_register(mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_ConnectFailedHandler, NULL);
    require_noerr(err, exit);

    err = mico_system_init(mico_system_context_init(0));
    require_noerr(err, exit);

    /* Output on debug serial port */
    os_helloworld_log("Hello world!");

    cur_time.year = 17; //设置时间
    cur_time.month = 9;
    cur_time.date = 5;
    cur_time.weekday = 4;
    cur_time.hr = 17;
    cur_time.min = 30;
    cur_time.sec = 0;
    sprintf(start_time_print, "20%02d-%02d-%02d %02d:%02d:%02d", cur_time.year, cur_time.month, cur_time.date, cur_time.hr, cur_time.min, cur_time.sec);

    err = MicoRtcSetTime(&cur_time); //初始化 RTC 时钟的时间
    require_noerr(err, exit);

    /* Trigger MiCO system led available on most MiCOKit */
    while (1)
    {
        MicoGpioOutputTrigger(MICO_SYS_LED);
        mico_thread_sleep(5);
        MicoRtcGetTime(&cur_time); //返回新的时间值
        sprintf(cur_time_print, "20%02d-%02d-%02d %02d:%02d:%02d", cur_time.year, cur_time.month, cur_time.date, cur_time.hr, cur_time.min, cur_time.sec);
        os_helloworld_log("up_count = %d,dowm_count = %d %16s--%16s",
                          wifi_connect_count, wifi_lost_count, start_time_print, cur_time_print);
    }
exit:
    mico_rtos_delete_thread(NULL);
    return kGeneralErr;
}
