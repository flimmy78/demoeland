/**
 ******************************************************************************
 * @file    http_client.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   MiCO http client demo to read data from www.baidu.com
 ******************************************************************************
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

#include "mico.h"
#include "HTTPUtils.h"
#include "SocketUtils.h"
#include "StringUtils.h"

#define http_client_log(M, ...) custom_log("HTTP", M, ##__VA_ARGS__)

static OSStatus onReceivedData(struct _HTTPHeader_t *httpHeader,
                               uint32_t pos,
                               uint8_t *data,
                               size_t len,
                               void *userContext);
static void onClearData(struct _HTTPHeader_t *inHeader, void *inUserContext);

static mico_semaphore_t wait_sem = NULL;

typedef struct _http_context_t
{
    char *content;
    uint64_t content_length;
} http_context_t;

void simple_http_get(char *host, char *query);
void simple_https_get(char *host, char *query);

char *certificate =
    "-----BEGIN CERTIFICATE-----\n\
MIIESzCCAzOgAwIBAgIJAMuHDGTAVpi1MA0GCSqGSIb3DQEBCwUAMIGTMQswCQYD\n\
VQQGEwJKUDEOMAwGA1UECAwFVG9reW8xGjAYBgNVBAoMEUtJTkcgSklNIENPLixM\n\
VEQuMRcwFQYDVQQLDA5SJkQgRGVwYXJ0bWVudDEZMBcGA1UEAwwQRUxBTkQgUHJp\n\
dmF0ZSBDQTEkMCIGCSqGSIb3DQEJARYVd2VibWFzdGVyQGV4YW1wbGUuY29tMCAX\n\
DTE3MDkwNjExNDIxNloYDzIxMTcwODEzMTE0MjE2WjCBoTELMAkGA1UEBhMCSlAx\n\
DjAMBgNVBAgMBVRva3lvMRMwEQYDVQQHDApDaGl5b2RhLWt1MRowGAYDVQQKDBFL\n\
SU5HIEpJTSBDTy4sTFRELjEXMBUGA1UECwwOUiZEIERlcGFydG1lbnQxEjAQBgNV\n\
BAMMCWVsYW5kLmNvbTEkMCIGCSqGSIb3DQEJARYVd2VibWFzdGVyQGV4YW1wbGUu\n\
Y29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnqwIBQSjPrp3HRvi\n\
O0h9+qWcKEkBNrhxhSiKzRN33cad/EdSwSv4ZQWBy0TggbMPDt5DPfw24BzvmQwW\n\
sl/FCUcQ70Xr+65pmdnp06Jobe90zH8YsSPyeSXta23zmvHJ2W1+7au9fRJQazRl\n\
4jBar6KCmoqFgg/Bw7qnEhI3fwRRly8LddVMj/emEmFEoiIJq5J09ZUo7fOmfCKr\n\
R0BVx8iKz6n1Cgt1/kpYJfnIkAYhTe0Ed4T2dJIVQkKjcSdW3dPFx86aJhFPcPa9\n\
3Mqv9aLSDh8A0YD0k0ohYCwvCe0g8FOMny66+Jdxbp0ybyDPZ4wvMP2sHOf7sTrg\n\
jv43iwIDAQABo4GPMIGMMAkGA1UdEwQCMAAwEQYJYIZIAYb4QgEBBAQDAgSwMCwG\n\
CWCGSAGG+EIBDQQfFh1PcGVuU1NMIEdlbmVyYXRlZCBDZXJ0aWZpY2F0ZTAdBgNV\n\
HQ4EFgQUHCxe533Sc0pFzudFwDZjpPfe6FkwHwYDVR0jBBgwFoAUWcY/DuhU0R7S\n\
cdwSiBFkabVvvKowDQYJKoZIhvcNAQELBQADggEBAGQKScFpkqqshnzMNEUVieP/\n\
CVjOIMxE2uxmhDeeQsWFR7odKnetFvHW9p2Lp2mXEGO6E0du5SKCryzD2ZTXiMsW\n\
SLdSau0G9591ROQok4F/6jn1zYkiL9GdsZBsxBdOGVl7JgO3eqYHsDkYA7vUHVoE\n\
WPt/Id+U2bmjuB+GHINNAL68+OZf+0BPt3w5UaGlaMhJRlO/f+PMJ97ZvrXnmRDj\n\
sa8GJLhffe4SpyHR168UGrrce26SJjlIzVsWB9uqK5bWG2WpqmMcz8Susn8EQFvH\n\
yQyH15iLII+5FwBQCDBjrKm3VzI7/2BD1rbbkqxv20fPTkYOAoh6vYd78aUYIBE=\n\
-----END CERTIFICATE-----";
char *private_key =
    "-----BEGIN PRIVATE KEY-----\n\
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCerAgFBKM+uncd\n\
G+I7SH36pZwoSQE2uHGFKIrNE3fdxp38R1LBK/hlBYHLROCBsw8O3kM9/DbgHO+Z\n\
DBayX8UJRxDvRev7rmmZ2enTomht73TMfxixI/J5Je1rbfOa8cnZbX7tq719ElBr\n\
NGXiMFqvooKaioWCD8HDuqcSEjd/BFGXLwt11UyP96YSYUSiIgmrknT1lSjt86Z8\n\
IqtHQFXHyIrPqfUKC3X+Slgl+ciQBiFN7QR3hPZ0khVCQqNxJ1bd08XHzpomEU9w\n\
9r3cyq/1otIOHwDRgPSTSiFgLC8J7SDwU4yfLrr4l3FunTJvIM9njC8w/awc5/ux\n\
OuCO/jeLAgMBAAECggEABi76GQf3PKiTn8TIajsG/c+aaE+ABpvlgKT108wgbboh\n\
ygUVioWmJnmydzN19FgADDpJMI81rEI0bCh2cfkdeqEUXd7BtYs0flRpsl+v5ijg\n\
yl9hnPWjq2j4+ajNR4qIrTqBKc35kng2PhdKqSftQM76e/9N+KWYjYImpKOlGgQH\n\
WSkPEBmTo7sIzNPwuye88bYY6oGh62nCIfvuwyfoTnlCQ3Gz8gDMFlqJgm/0J6Vb\n\
JYM8r1UCJyd2hKHuB0quuBEoRbNKm0agzx9Xe0KZ455GextTkm2JXvZlb957UrO/\n\
mzHs/G2MjrmmVJ8b/iusXSa9CKxQGdKXPmzwAq314QKBgQDR9wILEtJFNYmmtlv5\n\
cI9GQdPOtpMwpcPC13IdQ8KIuuTqblvy10BJLqMKXlDkhr8ZDqJR32Nhtl4pn5Kl\n\
61ZoEytjUpvb85Uv8Wvtyg/+/Lb9y/eUcSN5NI5kT+iJPTJOksDCd/6M/U16ZdMm\n\
cD0R4p32d+n3CjEZ+F8bKVRc2QKBgQDBdglLpbRFKtnMDZAQJUwwH3drcqyG6WWp\n\
Ica7aUNRUn7qrxckG3b3+ONCDhQVMQT/ijqj5yWkXQNwiZV2SJoAwkAbNH18yYPu\n\
zx0YYegVL26527uz+AJ9H88k4VzJkK6P6lmjOfqOLfytopsTSOV9r+vMIJW8E3Jq\n\
I+QlqTmJAwKBgQCPeit7RbFKeftGYPcYzUIa0IDckQakB6JuUqs4NEWLCavERwWu\n\
PElBuQzQ2QKOJ0YO6WEicXSIIQbXiqO7ncW9+Nt9U8YN17XqvR7zr1Ce/jJN3EOi\n\
vG1xNejXw4MzxQ3Lg50VRso7rhxzt4FCkxAoWKN4+Rh4KA7FoGPdO7DagQKBgB7M\n\
7hnvHc5NTjOgjSkk5wZaXCbtMO6hxh+xUvSPg7o0yiQPED4daUl9hKEFoMjm7wbI\n\
OSHTMTkD3gJSxUr5sBsi0hYCu1/crXad3uH85HhK/vP0OeQjPjIxmEck4iLtN/2N\n\
sAu+tVdhlvMGCm59kpv6IC51maFB71tar34XfSOFAoGAWoMPyUkwvlstoj8jYW1i\n\
5GDSj3MnwcYDkMsVZKVMQRMIgZCFHHIHDvtOS5pDcap41OdYMoLZjGzC7+Cp4kF1\n\
xzBtqSA0mrUr91R/KUi+Fg2pfssocYurFzgee/206/UrnLJ2a1uOTezt9p5vFBr+\n\
j1RnT/plaAAX6tMYz7zLsWY=\n\
-----END PRIVATE KEY-----";

#define SIMPLE_GET_REQUEST                                     \
    "GET /api/download.php?vid=taichi_16_024kbps HTTP/1.1\r\n" \
    "Host: 160.16.237.210\r\n"                                 \
    "\r\n"                                                     \
    ""

static void micoNotify_WifiStatusHandler(WiFiEvent status, void *const inContext)
{
    UNUSED_PARAMETER(inContext);
    switch (status)
    {
    case NOTIFY_STATION_UP:
        mico_rtos_set_semaphore(&wait_sem);
        break;
    case NOTIFY_STATION_DOWN:
    case NOTIFY_AP_UP:
    case NOTIFY_AP_DOWN:
        break;
    }
}

int application_start(void)
{
    OSStatus err = kNoErr;

    mico_rtos_init_semaphore(&wait_sem, 1);

    /*Register user function for MiCO nitification: WiFi status changed */
    err = mico_system_notify_register(mico_notify_WIFI_STATUS_CHANGED,
                                      (void *)micoNotify_WifiStatusHandler, NULL);
    require_noerr(err, exit);

    /* Start MiCO system functions according to mico_config.h */
    err = mico_system_init(mico_system_context_init(0));
    require_noerr(err, exit);

    /* Wait for wlan connection*/
    mico_rtos_get_semaphore(&wait_sem, MICO_WAIT_FOREVER);
    http_client_log("wifi connected successful");

    /* Read http data from server */
    //simple_http_get("www.baidu.com", SIMPLE_GET_REQUEST);
    simple_https_get("160.16.237.210", SIMPLE_GET_REQUEST);

exit:
    mico_rtos_delete_thread(NULL);
    return err;
}

void simple_http_get(char *host, char *query)
{
    OSStatus err;
    int client_fd = -1;
    fd_set readfds;
    char ipstr[16];
    struct sockaddr_in addr;
    HTTPHeader_t *httpHeader = NULL;
    http_context_t context = {NULL, 0};
    struct hostent *hostent_content = NULL;
    char **pptr = NULL;
    struct in_addr in_addr;

    hostent_content = gethostbyname(host);
    require_action_quiet(hostent_content != NULL, exit, err = kNotFoundErr);
    pptr = hostent_content->h_addr_list;
    in_addr.s_addr = *(uint32_t *)(*pptr);
    strcpy(ipstr, inet_ntoa(in_addr));
    http_client_log("HTTP server address: %s, host ip: %s", host, ipstr);

    /*HTTPHeaderCreateWithCallback set some callback functions */
    httpHeader = HTTPHeaderCreateWithCallback(1024, onReceivedData, onClearData, &context);
    require_action(httpHeader, exit, err = kNoMemoryErr);

    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr.sin_family = AF_INET;
    addr.sin_addr = in_addr;
    addr.sin_port = htons(80);
    err = connect(client_fd, (struct sockaddr *)&addr, sizeof(addr));
    require_noerr_string(err, exit, "connect http server failed");

    /* Send HTTP Request */
    send(client_fd, query, strlen(query), 0);

    FD_ZERO(&readfds);
    FD_SET(client_fd, &readfds);

    select(client_fd + 1, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(client_fd, &readfds))
    {
        /*parse header*/
        err = SocketReadHTTPHeader(client_fd, httpHeader);
        switch (err)
        {
        case kNoErr:
            PrintHTTPHeader(httpHeader);
            err = SocketReadHTTPBody(client_fd, httpHeader); /*get body data*/
            require_noerr(err, exit);
            /*get data and print*/
            http_client_log("Content Data: %s", context.content);
            break;
        case EWOULDBLOCK:
        case kNoSpaceErr:
        case kConnectionErr:
        default:
            http_client_log("ERROR: HTTP Header parse error: %d", err);
            break;
        }
    }

exit:
    http_client_log("Exit: Client exit with err = %d, fd: %d", err, client_fd);
    SocketClose(&client_fd);
    HTTPHeaderDestory(&httpHeader);
}

void simple_https_get(char *host, char *query)
{
    OSStatus err;
    int client_fd = -1;
    int ssl_errno = 0;
    mico_ssl_t client_ssl = NULL;
    fd_set readfds;
    char ipstr[16];
    struct sockaddr_in addr;
    HTTPHeader_t *httpHeader = NULL;
    http_context_t context = {NULL, 0};
    struct hostent *hostent_content = NULL;
    char **pptr = NULL;
    struct in_addr in_addr;

    hostent_content = gethostbyname(host);
    require_action_quiet(hostent_content != NULL, exit, err = kNotFoundErr);
    pptr = hostent_content->h_addr_list;
    in_addr.s_addr = *(uint32_t *)(*pptr);
    strcpy(ipstr, inet_ntoa(in_addr));
    http_client_log("HTTP server address: host:%s, ip: %s", host, ipstr);

    /*HTTPHeaderCreateWithCallback set some callback functions */
    httpHeader = HTTPHeaderCreateWithCallback(1024, onReceivedData, onClearData, &context);
    require_action(httpHeader, exit, err = kNoMemoryErr);

    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr.sin_family = AF_INET;
    addr.sin_addr = in_addr;
    addr.sin_port = htons(443);

    err = connect(client_fd, (struct sockaddr *)&addr, sizeof(addr));

    require_noerr_string(err, exit, "connect https server failed");

    ssl_set_client_cert(certificate, private_key);

    ssl_set_client_version(TLS_V1_2_MODE);

    client_ssl = ssl_connect(client_fd, 0, NULL, &ssl_errno);
    http_client_log("ssl_errno = %d client_fd = %d", ssl_errno, client_fd);
    require_string(client_ssl != NULL, exit, "ERROR: ssl disconnect");

    // client_ssl = ssl_connect(client_fd, strlen(http_server_ssl_cert_str), http_server_ssl_cert_str, &ssl_errno);
    // http_client_log("ssl_errno = %d client_fd = %d", ssl_errno, client_fd);
    // require_string(client_ssl != NULL, exit, "ERROR: ssl disconnect");

    // client_ssl = ssl_connect(client_fd, strlen(private_key), private_key, &ssl_errno);
    // http_client_log("ssl_errno = %d client_fd = %d", ssl_errno, client_fd);
    // require_string(client_ssl != NULL, exit, "ERROR: ssl disconnect");
    /* Send HTTP Request */
    ssl_send(client_ssl, query, strlen(query));

    FD_ZERO(&readfds);
    FD_SET(client_fd, &readfds);

    select(client_fd + 1, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(client_fd, &readfds))
    {
        /*parse header*/
        err = SocketReadHTTPSHeader(client_ssl, httpHeader);
        switch (err)
        {
        case kNoErr:
            PrintHTTPHeader(httpHeader);
            err = SocketReadHTTPSBody(client_ssl, httpHeader); /*get body data*/
            require_noerr(err, exit);
            /*get data and print*/
            http_client_log("Content Data: %s", context.content);
            break;
        case EWOULDBLOCK:
        case kNoSpaceErr:
        case kConnectionErr:
        default:
            http_client_log("ERROR: HTTP Header parse error: %d", err);
            break;
        }
    }

exit:
    http_client_log("Exit: Client exit with err = %d, fd: %d", err, client_fd);
    if (client_ssl)
        ssl_close(client_ssl);
    SocketClose(&client_fd);
    HTTPHeaderDestory(&httpHeader);
}

/*one request may receive multi reply*/
static OSStatus onReceivedData(struct _HTTPHeader_t *inHeader, uint32_t inPos, uint8_t *inData,
                               size_t inLen, void *inUserContext)
{
    OSStatus err = kNoErr;
    http_context_t *context = inUserContext;
    if (inHeader->chunkedData == false)
    { //Extra data with a content length value
        if (inPos == 0 && context->content == NULL)
        {
            context->content = calloc(inHeader->contentLength + 1, sizeof(uint8_t));
            require_action(context->content, exit, err = kNoMemoryErr);
            context->content_length = inHeader->contentLength;
        }
        memcpy(context->content + inPos, inData, inLen);
    }
    else
    { //extra data use a chunked data protocol
        http_client_log("This is a chunked data, %d", inLen);
        if (inPos == 0)
        {
            context->content = calloc(inHeader->contentLength + 1, sizeof(uint8_t));
            require_action(context->content, exit, err = kNoMemoryErr);
            context->content_length = inHeader->contentLength;
        }
        else
        {
            context->content_length += inLen;
            context->content = realloc(context->content, context->content_length + 1);
            require_action(context->content, exit, err = kNoMemoryErr);
        }
        memcpy(context->content + inPos, inData, inLen);
    }

exit:
    return err;
}

/* Called when HTTPHeaderClear is called */
static void onClearData(struct _HTTPHeader_t *inHeader, void *inUserContext)
{
    UNUSED_PARAMETER(inHeader);
    http_context_t *context = inUserContext;
    if (context->content)
    {
        free(context->content);
        context->content = NULL;
    }
}
