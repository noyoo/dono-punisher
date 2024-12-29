#include "App.h"
#include "Wifi.h"
#include "HTTP.h"
#include "CircularBuffer.h"
#include "esp_netif_sntp.h"
#include "lwip\sys.h"
#include "lwip\err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <string.h>

#define POLL_DELAY_S 10

static Circular_Buffer_idFrame_t parsedFrames;
extern Circular_Buffer_char24_t inBuffer;

static State_t _state = WifiInnit;
void stateMachine(void)
{
    IdFrame_t newFrame = {0};
    static int64_t lastTick = 0;
    switch (_state)
    {
    case WifiInnit:
        WIFI_Innit();
        wifi_start_sta();
        _state = WaitForWifiInnitComplete;
        break;
    case WaitForWifiInnitComplete:
        if (wifiConnected)
        {
            _state = SyncTime;
        }
        break;
    case SyncTime:
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        esp_netif_sntp_init(&config);

        if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(100000)) != ESP_OK) {
            ESP_LOGE("TIME_SYNC", "Failed to update system time within 100s timeout");
        }
        _state = HttpInnit;
        break;
    case HttpInnit:
        HTTP_Innit();
        _state = WaitForHttpInnitComplete;
        break;
    case WaitForHttpInnitComplete:
        lastTick = esp_timer_get_time();
        _state = Poll;
        break;
    case Poll:
        if (HTTP_Poll() == 1)
        {
        _state = Wait;
        }
        break;
    case Wait:
        if (esp_timer_get_time() - lastTick > POLL_DELAY_S * 1000000)
        {
            lastTick = esp_timer_get_time();
            _state = Poll;
        }

        buffer_get(&inBuffer, newFrame.id);
        if (newFrame.id[0] != 0)
        {
            newFrame.parsedAt = esp_timer_get_time();
            ESP_LOGI("APP", "ACTION!");
            buffer_put_idframe(&parsedFrames, &newFrame);
        }

        if(esp_timer_get_time()-parsedFrames.buffer[parsedFrames.tail].parsedAt > POLL_DELAY_S*1000000 * 2)
        {
            buffer_get_idframe(&parsedFrames, &newFrame); // discard
        }
        break;
    }
}

// init wifi - done
// prepare http - done
// synchronise time to RTC
// poll
    // open conn 
    // get rx length
    // if theres \r\n\r\n then its start of body
    // get first line of body(until \r\n) - its resp lenght in hex - convert with strtol
    // prepare a buffer
    // place everything into the buffer
    // close conn
    // extract id - that's the only thing we care for bcuz time range will take care of the rest
// if theres an id that did not show in the last [INTERVAL] seconds then trigger an action and save that ID