#include "WiFi.hpp"

#include <cstring>

#include <esp_event_loop.h>
#include <esp_wifi.h>

WiFi::WiFi(char const * ssid, char const * password) :
    mObserver(nullptr)
{
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi::eventHandler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi::eventHandler, this));

    static wifi_config_t wifi_config = {};
    memset(&wifi_config, 0, sizeof(wifi_config_t));
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

void WiFi::eventHandler(void * context, esp_event_base_t eventBase, int32_t eventId, void * eventData)
{
    return static_cast<WiFi *>(context)->handleEvent(eventBase, eventId, eventData);
}

void WiFi::handleEvent(esp_event_base_t eventBase, int32_t eventId, void * eventData)
{
    if (eventBase == WIFI_EVENT) {
        switch (eventId) {
        case WIFI_EVENT_STA_DISCONNECTED:
            if (mObserver) {
                mObserver->disconnected();
            }
            // fallthrough
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        default:
            break;
        }
    } else if (eventBase == IP_EVENT) {
        if (eventId == IP_EVENT_STA_GOT_IP) {
            auto event = static_cast<ip_event_got_ip_t const *>(eventData);
            if (mObserver) {
                mObserver->connected(event->ip_info.ip);
            }
        }
    }
}
