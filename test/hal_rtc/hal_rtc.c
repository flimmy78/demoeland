/*
 * hal_rtc.c
 *
 *  Created on: 2017年7月5日
 *      Author: ceeu
 */

#include "mico.h"
#define rtc_log(M, ...) custom_log("RTC", M, ##__VA_ARGS__)
void print_time(mico_rtc_time_t time) //打印时间
{
    rtc_log("year:%d", time.year);
    rtc_log("month:%d", time.month);
    rtc_log("date:%d", time.date);
    rtc_log("weekday:%d", time.weekday);
    rtc_log("hr:%d", time.hr);
    rtc_log("min:%d", time.min);
    rtc_log("sec:%d", time.sec);
}
OSStatus application_start(void)
{
    OSStatus status = kNoErr;
    mico_rtc_time_t cur_time = {0};
    cur_time.year = 16; //设置时间
    cur_time.month = 2;
    cur_time.date = 18;
    cur_time.weekday = 4;
    cur_time.hr = 17;
    cur_time.min = 30;
    cur_time.sec = 0;
    status = MicoRtcSetTime(&cur_time); //初始化 RTC 时钟的时间
    if (status == kNoErr)
        while (1)
        {
            print_time(cur_time);      //打印时间
            mico_thread_msleep(1000);  //等待 1s
            MicoRtcGetTime(&cur_time); //返回新的时间值
        }
    return status;
}
