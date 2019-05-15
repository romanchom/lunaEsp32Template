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
extern "C" void app_main();

static void initializeNonVolatileStorage()
{
    esp_err_t ret = nvs_flash_init();

    if (ESP_ERR_NVS_NO_FREE_PAGES == ret) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

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

#include "Certificates.hpp"

#include <luna/esp32/NetworkManager.hpp>
#include <luna/esp32/HardwareController.hpp>
#include <luna/esp32/StrandWS281x.hpp>
#include <luna/esp32/StrandView.hpp>
#include <luna/esp32/Outputs.hpp>

std::unique_ptr<luna::esp32::NetworkManager> lunaNetworkManager;
luna::esp32::HardwareController controller;

static void wifiConnected()
{
    lunaNetworkManager->enable();
}

static void wifiDisconnected()
{
    lunaNetworkManager->disable();
}

std::shared_ptr<luna::esp32::StrandWS2812> makePhysicalStrand()
{
    luna::esp32::WS2812Configuration config;
    config.pixelCount = 100;
    config.begin = {-1.0f, -1.0f};
    config.end = {-1.0f, 1.0f};
    config.gpioPin = luna::esp32::output5;

    return std::make_shared<luna::esp32::StrandWS2812>(config);
}

std::unique_ptr<luna::esp32::Strand> makeRightStrand(std::shared_ptr<luna::esp32::StrandWS2812> child)
{
    luna::esp32::StrandConfiguration config;
    config.pixelCount = 36;
    config.begin = {1.5f, -3.0f};
    config.end = {1.5f, 2.0f};

    return std::make_unique<luna::esp32::StrandView>(std::move(child), config, 0);
}

std::unique_ptr<luna::esp32::Strand> makeTopStrand(std::shared_ptr<luna::esp32::StrandWS2812> child)
{
    luna::esp32::StrandConfiguration config;
    config.pixelCount = 57;
    config.begin = {1.5f, 2.0f};
    config.end = {-4.0f, 2.0f};

    return std::make_unique<luna::esp32::StrandView>(std::move(child), config, 36);
}

static void configureHardware()
{
    auto physicalStrand = makePhysicalStrand();

    auto & strands = controller.strands();
    strands.emplace_back(makeRightStrand(physicalStrand));
    strands.emplace_back(makeTopStrand(physicalStrand));
}

static void initializeLuna()
{
    luna::esp32::NetworkManagerConfiguration config = {
        my_key,  static_cast<size_t>(my_key_end - my_key),
        my_cert, static_cast<size_t>(my_cert_end - my_cert),
        ca_cert, static_cast<size_t>(ca_cert_end - ca_cert),
    };

    lunaNetworkManager = std::make_unique<luna::esp32::NetworkManager>(config, &controller);
}

void app_main()
{
    initializeNonVolatileStorage();
    configureHardware();
    initializeLuna();
    initializeWiFi();
}
