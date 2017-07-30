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

#define os_helloworld_log(format, ...) custom_log("helloworld", format, ##__VA_ARGS__)

static mico_Context_t *mico_context;
volatile ring_buffer_t rx_buffer;
volatile uint8_t rx_data[2048];
#define UART_BUFFER_LENGTH 2048
#define UART_ONE_PACKAGE_LENGTH 1024
#define STACK_SIZE_UART_RECV_THREAD 0x900

int uart_get_one_packet(uint8_t *inBuf, int inBufLen)
{
    OSStatus err = kNoErr;
    char datalen;
    uint8_t *p;
    p = inBuf;
    //MicoGpioOutputTrigger(MICO_SYS_LED);
    err = MicoUartRecv(MICO_UART_2, p, 1, MICO_WAIT_FOREVER);
    MicoGpioOutputTrigger(MICO_SYS_LED);
    require_noerr(err, exit);
    require((*p == 0xA0), exit);
    for (int i = 0; i < 7; i++)
    {
        p++;
        err = MicoUartRecv(MICO_UART_2, p, 1, 500);

        require_noerr(err, exit);
    }
    datalen = *p;
    p++;
    err = MicoUartRecv(MICO_UART_2, p, datalen + 1, 500);
    require_noerr(err, exit);
    require(datalen + 9 <= inBufLen, exit);
    return datalen + 9; //返回帧的长度
exit:
    return -1;
}
void uart_recv_thread_DDE(uint32_t arg)
{
    int8_t recvlen;
    uint8_t *inDataBuffer;
    inDataBuffer = malloc(UART_ONE_PACKAGE_LENGTH);
    require(inDataBuffer, exit);
    while (1)
    {
        recvlen = uart_get_one_packet(inDataBuffer, UART_ONE_PACKAGE_LENGTH);
        if (recvlen <= 0)
        {
            MicoGpioOutputTrigger(MICO_SYS_LED);
            continue;
        }
        //printf( "\r\n" );
//        for (int i = 0; i < recvlen; i++)
//        {
//            //printf( "%02x ", inDataBuffer[i] );
//            MicoUartSend(MICO_UART_2, &(inDataBuffer[i]), 1);
//        }
      MicoUartSend(MICO_UART_2, &(inDataBuffer[0]), recvlen);
        MicoGpioOutputTrigger(MICO_SYS_LED);
        //printf( "\r\n\r\n" );
        //uart_cmd_process( inDataBuffer, recvlen );
        //mico_rtos_set_semaphore( &postfog_sem );
    }
exit:
    if (inDataBuffer)
        free(inDataBuffer);
    mico_rtos_delete_thread(NULL);
}

int application_start(void)
{
    mico_context = mico_system_context_init(0);
    mico_uart_config_t uart_config;
    /*UART receive thread*/
    uart_config.baud_rate = 115200;
    uart_config.data_width = DATA_WIDTH_8BIT;
    uart_config.parity = NO_PARITY;
    uart_config.stop_bits = STOP_BITS_1;
    uart_config.flow_control = FLOW_CONTROL_DISABLED;
    if (mico_context->micoSystemConfig.mcuPowerSaveEnable == true)
        uart_config.flags = UART_WAKEUP_ENABLE;
    else
        uart_config.flags = UART_WAKEUP_DISABLE;
    MicoGpioOutputTrigger(MICO_SYS_LED);
    ring_buffer_init((ring_buffer_t *)&rx_buffer, (uint8_t *)rx_data, UART_BUFFER_LENGTH);
    MicoUartInitialize(MICO_UART_2, &uart_config, (ring_buffer_t *)&rx_buffer);
    mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART Recv", uart_recv_thread_DDE,
                            STACK_SIZE_UART_RECV_THREAD, 0);

    /* Output on debug serial port */
    // os_helloworld_log("Hello world!");

    /* Trigger MiCO system led available on most MiCOKit */

    mico_rtos_delete_thread(NULL);
    return kGeneralErr;
}
