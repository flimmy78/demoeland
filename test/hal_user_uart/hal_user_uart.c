#include "mico.h"
#define wifi_user_uart_log(M, ...) custom_log("HAL", M, ##__VA_ARGS__)
//new add
#define USER_UART_BUFFER_LENGTH 256
static ring_buffer_t user_rx_buffer;
static uint8_t user_rx_data[USER_UART_BUFFER_LENGTH] = {0};
//初始化用户串口
OSStatus user_uart_init(void)
{
    mico_uart_config_t uart_config;
    memset(&uart_config, 0, sizeof(mico_uart_config_t));
    uart_config.baud_rate = 115200;
    uart_config.data_width = DATA_WIDTH_8BIT;
    uart_config.parity = NO_PARITY;
    uart_config.stop_bits = STOP_BITS_1;
    uart_config.flow_control = FLOW_CONTROL_DISABLED;
    uart_config.flags = UART_WAKEUP_DISABLE;
    ring_buffer_init((ring_buffer_t *)&user_rx_buffer, (uint8_t *)user_rx_data,
                     USER_UART_BUFFER_LENGTH);
    MicoUartInitialize(MICO_UART_1, &uart_config, (ring_buffer_t *)&user_rx_buffer);
    return kNoErr;
}
//return len
//arg buf to fill
int get_one_packet(unsigned char *buf)
{
    char *p = buf;
    int len = 0;
    while (1)
    {
        unsigned char byte;
        MicoUartRecv(MICO_UART_1, &byte, 1, MICO_WAIT_FOREVER);
        *p = byte;
        if (*p == 0x0D) //It Depends on the uart command protocol
        {
            break;
        }
        p++;
        len++;
    }
    return len;
}
//Thread fun
void uart_rcv_thread(void *arg)
{
    unsigned char buffer[256] = {0};
    user_uart_init();
    //char bbuf[] = {'1', '2', '3', '4', '\r', '\n'};
    while (1)
    {
        wifi_user_uart_log("waiting user uart ....");
        memset(buffer, 0, sizeof(buffer));
        int len = get_one_packet(buffer); //get uart data one by one according to protocol
        wifi_user_uart_log("len=%d", len);
        //Send back to Debug Uart
        MicoUartSend(MICO_UART_1, buffer, len);
        MicoUartSend(MICO_UART_1, "\r\n", 2);
        //mico_thread_sleep(1);
    }
}
int application_start(void)
{
    mico_thread_t uart_rcv_handle;
    //mico_thread_t uart_send_handle;
    OSStatus err = kNoErr;
    /* mico system initialize */

    err =  mico_system_init( mico_system_context_init( 0 ) );
    require_noerr( err, exit );

    mico_rtos_create_thread(&uart_rcv_handle, MICO_APPLICATION_PRIORITY, "uart_rcv_thread", uart_rcv_thread, 0x300, NULL);
    mico_rtos_delete_thread(NULL);
    exit:
    return err;
}
