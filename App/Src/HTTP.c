#include "HTTP.h"

#include <string.h>
#include <stdlib.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "CircularBuffer.h"

Circular_Buffer_char24_t inBuffer;

#define WEB_URL "https://api.streamelements.com/kappa/v2/activities/" CONFIG_CHANNEL_ID "?after=2023-09-26&before=2023-09-27&limit=10&mincheer=0&minhost=0&minsub=0&mintip=0&origin=0&types=follow"
#define WEB_URL_PART1 "https://api.streamelements.com/kappa/v2/activities/" CONFIG_CHANNEL_ID "?after="
#define WEB_URL_PART2 "&before="
#define WEB_URL_PART3 "&limit=10&mincheer=0&minhost=0&minsub=0&mintip=0&origin=0&types=follow"

#define WEB_SERVER "api.streamelements.com"
#define RXBUF_SIZE  1024

typedef enum
{
    Polling_Send,
    Polling_GetBody,
    Polling_GetLength,
    Polling_GetData,
    Polling_Finished,
} Polling_State_t;

Polling_State_t _state = Polling_Send;

esp_http_client_handle_t http_client;
    
esp_tls_cfg_t cfg = {
    .crt_bundle_attach = esp_crt_bundle_attach,
};

esp_tls_t* tls;
static char rxBuf[RXBUF_SIZE];

// // static char buf[1024];
// int16_t len;
// uint8_t responseBodyStartFlag;
// uint8_t responseLengthCaptured;

// static const char SSL_REQUEST[] = "GET " WEB_URL " HTTP/1.1\r\n"
//                              "Host: "WEB_SERVER"\r\n"
//                              "User-Agent: esp-idf/1.0 esp32\r\n"
//                             "Authorization: Bearer "CONFIG_SE_BEARER_TOKEN"\r\n"
//                             "Accept: application/json; charset=utf-8, application/json\r\n"
//                              "\r\n";

void HTTP_Innit(void)
{
    tls = esp_tls_init();
        if (esp_tls_conn_http_new_sync(WEB_URL, &cfg, tls) == 1)
        {
        ESP_LOGI("TLS", "Connection established...");
        }
}

uint8_t HTTP_Poll(void)
{
    
    uint8_t ret = 0;
    static uint16_t chunkLength = 0;
    size_t written_bytes = 0;
    static uint16_t dataLength = 0;
    static char* readPtr = NULL;
    static uint16_t remainingPacketLength = 0;
    char* endSequence = NULL;
    char strftime_buf[24];
    switch (_state)
    {
        case Polling_Send:
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);
            timeinfo.tm_hour += 1;
            
            static char request[2048] = {0};
            memset (request, 0, 2048);
            strcat(request, "GET " WEB_URL_PART1);
            // TIME AFTER
            strftime(strftime_buf, sizeof(strftime_buf), "%F+%H%%3A%M%%3A%S", &timeinfo);
            strcat(request, strftime_buf);
            strcat(request, WEB_URL_PART2);
            // TIME BEFORE
            timeinfo.tm_min -= 1;
            strftime(strftime_buf, sizeof(strftime_buf), "%F+%H%%3A%M%%3A%S", &timeinfo);
            strcat(request, strftime_buf);
            strcat(request, WEB_URL_PART3);
            strcat(request, " HTTP/1.1\r\n"
                                "Host: "WEB_SERVER"\r\n"
                                "User-Agent: esp-idf/1.0 esp32\r\n"
                                "Authorization: Bearer "CONFIG_SE_BEARER_TOKEN"\r\n"
                                "Accept: application/json; charset=utf-8, application/json\r\n"
                                "\r\n");

            while (written_bytes < strlen(request))
            {
                int ret = esp_tls_conn_write(tls, request + written_bytes, strlen(request) - written_bytes);
                if (ret >= 0) {
                    ESP_LOGI("TX", "%d bytes written", ret);
                    written_bytes += ret;
                } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
                    ESP_LOGE("TX", "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
                }
            } 

            _state = Polling_GetBody;
        break;

        case Polling_GetBody:
            memset(rxBuf, 0, RXBUF_SIZE);
            chunkLength = esp_tls_conn_read(tls, rxBuf, RXBUF_SIZE);
            endSequence = strstr(rxBuf, "0\r\n\r\n");
            if ((endSequence != NULL) && (chunkLength == 5))
            {
                ESP_LOGI("RX", "endSeq found, I'mma head out");
                _state = Polling_Send;
                break;
            }

            char* body = strstr(rxBuf, "\r\n\r\n");
            if (body != NULL)
            {
                ESP_LOGI("RX", "got body");
                body +=4;
                remainingPacketLength = chunkLength - (body - rxBuf);
                readPtr = (remainingPacketLength > 0) ? body : rxBuf;
                
                _state = Polling_GetLength;
            }
        break;

        case Polling_GetLength:
            if (readPtr == rxBuf)
            {
                ESP_LOGI("RX", "new read for data length");
                memset(rxBuf, 0, RXBUF_SIZE);
                chunkLength = esp_tls_conn_read(tls, rxBuf, RXBUF_SIZE);
            }
            if (strstr(readPtr, "[]") != NULL)
            {
                ESP_LOGI("RX", "no data");
                _state = Polling_Finished;
                break;
            }
            dataLength = (uint16_t)strtol(readPtr, NULL, 16) + 2; // the body has \r\n at the end which isn't counted towards the length
            ESP_LOGI("RX", "packet length is: %d", dataLength);
            readPtr = strstr(readPtr, "\r\n");
            remainingPacketLength = chunkLength -  (readPtr + 2 - rxBuf); 
            ESP_LOGD("RX", "REMAINING packet length is: %d", remainingPacketLength);
            _state = Polling_GetData;
        break;

        case Polling_GetData:
            readPtr = strstr(readPtr, "\"_id\":\"");
            if (readPtr != NULL)
            {
                readPtr += 7;
                buffer_put(&inBuffer, readPtr);
                ESP_LOGI("RX", "found ID, placed in buffer");
            }
            else
            {
                ESP_LOGI("RX", "rest of chunk is empty, dl: %d, rpl: %d", dataLength, remainingPacketLength);

                dataLength -= remainingPacketLength;
                ESP_LOGI("RX", "remaining length is: %d", dataLength);
                if (dataLength != 0)
                {
                    memset(rxBuf, 0, RXBUF_SIZE);
                    chunkLength = esp_tls_conn_read(tls, rxBuf, RXBUF_SIZE);
                    readPtr = rxBuf;
                ESP_LOGI("RX", "incoming chunk length is: %d", chunkLength);
                    remainingPacketLength = chunkLength;
                }
                else
                {
                    readPtr = NULL;
                    _state = Polling_Finished;
                }
            }
        break;

        case Polling_Finished:
            chunkLength = 0;
            dataLength = 0;
            readPtr = NULL;
            remainingPacketLength = 0;
            _state = Polling_Send;
            ret = 1;
        break;
    }

     return ret;
}