/**
 ******************************************************************************
 * @file    stm8_OTAapp
 * @author  xiang
 * @version V1.0.0
 * @date    18-Dec-2017
 * @brief   First MiCO application to stm8 ota!
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2017 AOYAGI Inc.
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

#define stm8_OTAapp_log(format, ...) custom_log("stm8_OTAapp", format, ##__VA_ARGS__)

extern void uart_rcv_thread1(mico_thread_arg_t *arg);
int application_start(void)
{
    OSStatus err = kNoErr;
    /* Start MiCO system functions according to mico_config.h*/
    //mico_system_init(mico_system_context_init(0));
    mico_rtos_thread_sleep(5);

    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART Recv", uart_rcv_thread1, 0x300, (mico_thread_arg_t)NULL);

    require_noerr_action(err, exit, stm8_OTAapp_log("ERROR: Unable to start the uart recv thread."));

exit:
    return err;
}
