#include "Certificates.hpp"

#include <luna/esp32/ControlChannelConfig.hpp>
#include <luna/esp32/Manager.hpp>
#include <luna/esp32/StrandWS281x.hpp>
#include <luna/esp32/Outputs.hpp>

#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include <cstring>
#include <memory>

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD

static char const TAG[] = "App";

static void wifiConnected();
static void wifiDisconnected();

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        wifiConnected();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        wifiDisconnected();
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initializeWiFi()
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strcpy(reinterpret_cast<char *>(wifi_config.sta.ssid), EXAMPLE_ESP_WIFI_SSID);
    strcpy(reinterpret_cast<char *>(wifi_config.sta.password), EXAMPLE_ESP_WIFI_PASS);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized.");
}

std::unique_ptr<luna::esp32::Manager> lunaEsp;
luna::server::StrandDecoder * strandDecoder = nullptr;

static void wifiConnected()
{
    lunaEsp.reset();

    luna::esp32::ControlChannelConfig conf;
    conf.listenPort = 9510;

    conf.privateKey = my_key;
    conf.privateKeySize = static_cast<size_t>(my_key_end - my_key);

    conf.certificate = my_cert;
    conf.certificateSize = static_cast<size_t>(my_cert_end - my_cert);

    conf.caCertificate = ca_cert;
    conf.caCertificateSize = static_cast<size_t>(ca_cert_end - ca_cert);

    lunaEsp.reset(new luna::esp32::Manager(conf, strandDecoder));
}

static void wifiDisconnected()
{
    lunaEsp.reset();
}

extern "C" {
    void app_main();
}

static void initializeNonVolatileStorage()
{
    esp_err_t ret = nvs_flash_init();

    if (ESP_ERR_NVS_NO_FREE_PAGES == ret) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

static void initializeLuna()
{
    strandDecoder = new luna::server::StrandDecoder();
    luna::esp32::WS2812Configuration config;
    config.pixelCount = 8;
    config.begin = {-1.0f, -1.0f};
    config.end = {-1.0f, 1.0f};
    config.gpioPin = luna::esp32::output5;
    strandDecoder->addStrand(new luna::esp32::StrandWS2812(config));
}

void app_main()
{
    initializeNonVolatileStorage();
    initializeLuna();
    initializeWiFi();
}
