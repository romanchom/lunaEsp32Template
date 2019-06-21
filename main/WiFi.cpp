#include "WiFi.hpp"

#include <cstring>

#include <esp_event_loop.h>
#include <esp_wifi.h>

WiFi::WiFi(char const * ssid, char const * password) :
    mObserver(nullptr)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(WiFi::eventHandler, this) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strcpy(reinterpret_cast<char *>(wifi_config.sta.ssid), ssid);
    strcpy(reinterpret_cast<char *>(wifi_config.sta.password), password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

void WiFi::observer(Observer * value)
{
    mObserver = value;
}

void WiFi::enabled(bool value)
{
    if (value) {
        ESP_ERROR_CHECK(esp_wifi_start());
    } else {
        
    }
}

esp_err_t WiFi::eventHandler(void * context, system_event_t * event)
{
    return static_cast<WiFi *>(context)->handleEvent(event);
}

esp_err_t WiFi::handleEvent(system_event_t * event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        if (mObserver) {
            mObserver->connected(event->event_info.got_ip.ip_info.ip);
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        if (mObserver) {
            mObserver->disconnected();
        }
        esp_wifi_connect();
        break;
    default:
        break;
    }
    return ESP_OK;
}
