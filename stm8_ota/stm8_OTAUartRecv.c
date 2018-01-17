/**
 ******************************************************************************
 * @file    stm8_OTAUartRecv.c
 * @author  xiang
 * @version V1.0.0
 * @date    18-Dec-2017
 * @brief   This file create a UART recv thread.
 ******************************************************************************
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
 ******************************************************************************
 */

#include "mico.h"
#include "stm8_bin.h"

#define stm8_OTAUartRecv_log(M, ...) custom_log("UART", M, ##__VA_ARGS__)

#define Eland_Firmware_Version ("elend_firmware 01.12") //Eland?????
#define UART_ONE_PACKAGE_LENGTH 150

//new add
#define USER_UART_BUFFER_LENGTH 256
static ring_buffer_t user_rx_buffer;
static uint8_t user_rx_data[USER_UART_BUFFER_LENGTH] = {0};

/*
 * uart init
*/
OSStatus OTA_uart_init(void)
{
    mico_uart_config_t uart_config;
    memset(&uart_config, 0, sizeof(mico_uart_config_t));

    uart_config.baud_rate = 115200;
    uart_config.data_width = DATA_WIDTH_8BIT;
    uart_config.parity = EVEN_PARITY;
    uart_config.stop_bits = STOP_BITS_1;
    uart_config.flow_control = FLOW_CONTROL_DISABLED;
    uart_config.flags = UART_WAKEUP_DISABLE;

    ring_buffer_init((ring_buffer_t *)&user_rx_buffer, (uint8_t *)user_rx_data, USER_UART_BUFFER_LENGTH);
    MicoUartInitialize(MICO_UART_2, &uart_config, (ring_buffer_t *)&user_rx_buffer);
    return kNoErr;
}

/*
 * get copy to buf, return len
*/
size_t OTA_uart_get_one_packet(uint8_t *inBuf, int inBufLen, uint8_t timeout)
{
    int datalen;

    while (1)
    {
        if (MicoUartRecv(MICO_UART_2, inBuf, inBufLen, timeout) == kNoErr)
        {
            return inBufLen;
        }
        else
        {
            datalen = MicoUartGetLengthInBuffer(MICO_UART_2);
            if (datalen)
            {
                MicoUartRecv(MICO_UART_2, inBuf, datalen, timeout);
                return datalen;
            }
            else
                return 0;
        }
    }
}
/*
 * command once, Uart Send and recv
 */
OSStatus stm8_ota_command_once(uint8_t *inBuf, int inBufLen, uint8_t timeout)
{
    int recv_len;

    MicoUartSend(MICO_UART_2, inBuf, inBufLen);
    recv_len = OTA_uart_get_one_packet(inBuf, UART_ONE_PACKAGE_LENGTH, timeout);
    if (recv_len <= 0)
        return kGeneralErr;
    if (inBuf[0] == 0x79)
        return kNoErr;
    else
        return kGeneralErr;
}
/*
 * Checksum   XOR
 */
uint8_t stm8_ota_Checksum(uint8_t *inBuf, int inBufLen)
{
    uint8_t j, Checksum = 0;

    for (j = 0; j < inBufLen; j++)
        Checksum = Checksum ^ inBuf[j];
    return Checksum;
}

OSStatus stm8_ota_start_command(void)
{
    uint8_t inDataBuffer[50];
    uint8_t outDataBuffer[50];
    uint8_t times = 10;
    int recv_len;
    mico_uart_config_t uart_config;

    memset(&uart_config, 0, sizeof(mico_uart_config_t));
    memset(&inDataBuffer, 0, sizeof(inDataBuffer));
    memset(&outDataBuffer, 0, sizeof(outDataBuffer));

    uart_config.baud_rate = 115200;
    uart_config.data_width = DATA_WIDTH_8BIT;
    uart_config.parity = NO_PARITY;
    uart_config.stop_bits = STOP_BITS_1;
    uart_config.flow_control = FLOW_CONTROL_DISABLED;

    ring_buffer_init((ring_buffer_t *)&user_rx_buffer, (uint8_t *)user_rx_data, USER_UART_BUFFER_LENGTH);
    MicoUartInitialize(MICO_UART_2, &uart_config, (ring_buffer_t *)&user_rx_buffer);

    inDataBuffer[0] = 0x55;
    inDataBuffer[1] = 0x09;
    inDataBuffer[2] = 0x00;
    inDataBuffer[3] = 0xaa;
    do
    {
        times--;
        MicoUartSend(MICO_UART_2, inDataBuffer, 4);
        recv_len = OTA_uart_get_one_packet(outDataBuffer, UART_ONE_PACKAGE_LENGTH, 2);
        if (recv_len > 0)
        {
            if (outDataBuffer[0] == 0x55)
                break;
            else
                outDataBuffer[0] = 0;
        }
        mico_rtos_thread_msleep(10);
    } while (times > 0);
    OTA_uart_init();
    if (times)
        return kNoErr;
    else
        return kGeneralErr;
}
/*
 * Thread fun
 */
void uart_rcv_thread1(mico_thread_arg_t *arg)
{
    int i, stm8FW_len, stm8FW_page, OTA_Step_No = 0;
    uint8_t *inDataBuffer;

    inDataBuffer = malloc(UART_ONE_PACKAGE_LENGTH);
    require(inDataBuffer, exit);

    OTA_uart_init();
    stm8_OTAUartRecv_log("uart init ....");
    while (1)
    {
        switch (OTA_Step_No)
        {
        case 0:
            /*The bootloader polls all peripherals waiting for a synchronization
                     byte/message (SYNCHR = 0x7F) within a timeout of 1 second. If a timeout occurs,
                     either the Flash program memory is virgin in which case it waits for a synchronization
                     byte/message in an infinite loop through a software reset, or the Flash program
                     memory is not virgin and the bootloader restores the registers�� reset status and jumps
                     to the memory address given by the reset vector (located at 0x00 8000).*/

            //stm8 mcu reset  (power off-->power on)
            // MicoGpioOutputLow((mico_gpio_t)MICO_ADF7030_POWER);
            // mico_rtos_delay_milliseconds(200);
            // MicoGpioOutputHigh((mico_gpio_t)MICO_ADF7030_POWER);
            // mico_rtos_delay_milliseconds(200);
            stm8_ota_start_command();
            mico_rtos_thread_msleep(50);
            inDataBuffer[0] = 0x7f; //Synchronization byte
            if (kNoErr != stm8_ota_command_once(inDataBuffer, 1, 2))
                goto exit_err;
            OTA_Step_No = 1;
            break;
        case 1:
            /*Get Command
                Gets the version and the allowed commands supported by the current version of
                     the bootloader*/

            inDataBuffer[0] = 0x00;
            inDataBuffer[1] = 0xff; //Get command
            if (kNoErr != stm8_ota_command_once(inDataBuffer, 2, 2))
                goto exit_err;
            OTA_Step_No = 2;
            break;
        case 2:
            /*Write Command
                Writes up to 128 bytes to RAM or the Flash program memory/data EEPROM
                       starting from an address specified by the host
                The host sends the bytes as follows
                Byte 1:  0x31  - Command ID
                Byte 2:  0xCE  - Complement
                      Wait for ACK or NACK......
                Bytes 3-6:  The start address (32-bit address)
                Byte 3 = MSB
                Byte 6 = LSB
                Byte 7:  Checksum = XOR (byte 3, byte 4, byte 5, byte 6)
                      Wait for ACK or NACK......
                Byte 8:  The number of bytes to be received -1: N = 0 ... 127
                If N > 127, a cmd_error occurs in the bootloader.
                N+1 bytes: Max 128 data bytes
                Checksum byte: XOR (N,[N+1 data bytes])
                      Wait for ACK or NACK......*/

            stm8FW_len = sizeof(FW_C_TABLE);
            stm8FW_page = stm8FW_len / 128;
            if (stm8FW_len % 128 != 0)
                stm8FW_page += 1; //if Not enough one page, make up one page
            for (i = 0; i < stm8FW_page; i++)
            {
                inDataBuffer[0] = 0x31;
                inDataBuffer[1] = 0xce; //Write Command
                if (kNoErr != stm8_ota_command_once(inDataBuffer, 2, 2))
                    goto exit_err;

                inDataBuffer[0] = 0x00;
                inDataBuffer[1] = 0x00;
                inDataBuffer[2] = 0x80 + i / 2;
                if (i % 2 == 0)
                    inDataBuffer[3] = 0x00;
                else
                    inDataBuffer[3] = 0x80; //flash address
                inDataBuffer[4] = stm8_ota_Checksum(inDataBuffer, 4);
                if (kNoErr != stm8_ota_command_once(inDataBuffer, 5, 2))
                    goto exit_err;

                inDataBuffer[0] = 0x7f;
                memcpy(&inDataBuffer[1], &FW_C_TABLE[i * 128], 128); //flash data
                inDataBuffer[129] = stm8_ota_Checksum(inDataBuffer, 129);
                if (kNoErr != stm8_ota_command_once(inDataBuffer, 130, 10))
                    goto exit_err;
            }
            memset(inDataBuffer, 0, 130);
            inDataBuffer[0] = 0x31;
            inDataBuffer[1] = 0xce; //Write Command
            if (kNoErr != stm8_ota_command_once(inDataBuffer, 2, 2))
                goto exit_err;

            inDataBuffer[0] = 0x00;
            inDataBuffer[1] = 0x00;
            inDataBuffer[2] = 0x10;
            inDataBuffer[3] = 0x00; //flash/eeprom address
            inDataBuffer[4] = stm8_ota_Checksum(inDataBuffer, 4);
            if (kNoErr != stm8_ota_command_once(inDataBuffer, 5, 2))
                goto exit_err;

            inDataBuffer[0] = 0x7f;
            memcpy(&inDataBuffer[1], Eland_Firmware_Version, strlen(Eland_Firmware_Version)); //flash data
            inDataBuffer[129] = stm8_ota_Checksum(inDataBuffer, 129);
            if (kNoErr != stm8_ota_command_once(inDataBuffer, 130, 10))
                goto exit_err;
            OTA_Step_No = 3;
            break;
        case 3:
            /*Go Command
                Jumps to an address specified by the host to execute a loaded code
                The host sends the bytes as follows
                Byte 1:  0x21  - Command ID
                Byte 2:  0xDE - Complement
                     Wait for ACK or NACK......
                Bytes 3-6:  The start address (32-bit address)
                Byte 3 = MSB
                Byte 6 = LSB
                Byte 7: Checksum = XOR (byte 3, byte 4, byte 5, byte 6).
                     Wait for ACK or NACK......*/

            inDataBuffer[0] = 0x21;
            inDataBuffer[1] = 0xde; //Go command
            if (kNoErr != stm8_ota_command_once(inDataBuffer, 2, 2))
                goto exit_err;

            inDataBuffer[0] = 0x00;
            inDataBuffer[1] = 0x00;
            inDataBuffer[2] = 0x80;
            inDataBuffer[3] = 0x00; //flash address
            inDataBuffer[4] = stm8_ota_Checksum(inDataBuffer, 4);
            if (kNoErr != stm8_ota_command_once(inDataBuffer, 5, 2))
                goto exit_err;
            OTA_Step_No = 4;
            break;
        case 4:
            goto exit;
            break;
        default:
            break;
        }
    }

exit_err:
    stm8_OTAUartRecv_log("stm8 flash program Fail ....");
    if (inDataBuffer)
        free(inDataBuffer);
    mico_rtos_delete_thread(NULL);
    return;
exit:
    stm8_OTAUartRecv_log("stm8 flash program OK ....");
    if (inDataBuffer)
        free(inDataBuffer);
    mico_rtos_delete_thread(NULL);
    return;
}
