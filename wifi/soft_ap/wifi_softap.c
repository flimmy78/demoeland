/**
 ******************************************************************************
 * @file    wifi_softap.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   Create an access point!
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2014 MXCHIP Inc.
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

#define wifi_softap_log(M, ...) custom_log("WIFI", M, ##__VA_ARGS__)

static char *ap_ssid = "mxchip_zfw";
static char *ap_key = "12345678";

static void micoNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            wifi_softap_log("NOTIFY_STATION_UP");
            break;
        case NOTIFY_STATION_DOWN:
            wifi_softap_log("NOTIFY_STATION_DOWN");
            break;
        case NOTIFY_AP_UP:
            wifi_softap_log("NOTIFY_AP_UP");
           break;
        case NOTIFY_AP_DOWN:
            wifi_softap_log("NOTIFY_AP_DOWN");
           break;
    }
}

int application_start( void )
{
  OSStatus err = kNoErr;
  network_InitTypeDef_st wNetConfig;

  /*Register user function for MiCO nitification: WiFi status changed */
  err = mico_system_notify_register( mico_notify_WiFI_PARA_CHANGED,
                                     (void *) micoNotify_WifiStatusHandler, NULL );
  require_noerr( err, exit );

  /* Start MiCO system functions according to mico_config.h */
  err = mico_system_init( mico_system_context_init( 0 ) );
  require_noerr( err, exit );



  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));

  strcpy((char*)wNetConfig.wifi_ssid, ap_ssid);
  strcpy((char*)wNetConfig.wifi_key, ap_key);

  wNetConfig.wifi_mode = Soft_AP;
  wNetConfig.dhcpMode = DHCP_Server;
  wNetConfig.wifi_retry_interval = 100;
  strcpy((char*)wNetConfig.local_ip_addr, "192.168.0.1");
  strcpy((char*)wNetConfig.net_mask, "255.255.255.0");
  strcpy((char*)wNetConfig.dnsServer_ip_addr, "192.168.0.1");

  wifi_softap_log("ssid:%s  key:%s", wNetConfig.wifi_ssid, wNetConfig.wifi_key);
  micoWlanStart(&wNetConfig);
  wifi_softap_log("ssid:  key:");
  /* Wait for wlan connection*/

  while(1)
  {
      MicoGpioOutputTrigger( MICO_SYS_LED );
      mico_thread_sleep(1);
  }
exit:
wifi_softap_log("exit");
  mico_rtos_delete_thread(NULL);
  return err;
}


