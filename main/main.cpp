#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <driver/ledc.h>

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
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
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

std::unique_ptr<luna::esp32::StrandWS2812> makeLeftStrand()
{
    luna::esp32::WS2812Configuration config;
    config.pixelCount = 120;
    config.begin = {-1.0f, -1.2f};
    config.end = {-1.0f, 1.0f};
    config.gpioPin = 26;

    return std::make_unique<luna::esp32::StrandWS2812>(config);
}

std::unique_ptr<luna::esp32::StrandWS2812> makeRightStrand()
{
    luna::esp32::WS2812Configuration config;
    config.pixelCount = 120;
    config.begin = {1.0f, -1.2f};
    config.end = {1.0f, 1.0f};
    config.gpioPin = 27;

    return std::make_unique<luna::esp32::StrandWS2812>(config);
}

static void configureHardware()
{
    auto & strands = controller.strands();
    strands.emplace_back(makeLeftStrand());
    strands.emplace_back(makeRightStrand());

    gpio_config_t io_conf;
    io_conf.intr_type = gpio_int_type_t(GPIO_PIN_INTR_DISABLE);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1 << 14;
    io_conf.pull_down_en = gpio_pulldown_t(0);
    io_conf.pull_up_en = gpio_pullup_t(0);
    gpio_config(&io_conf);

    gpio_set_level(gpio_num_t(14), 0);


    ledc_timer_config_t ledc_timer;
    ledc_timer.duty_resolution = ledc_timer_bit_t(8);
    ledc_timer.freq_hz = 40000;
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_timer.timer_num = LEDC_TIMER_0;
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel;
    ledc_channel.channel = LEDC_CHANNEL_0;
    ledc_channel.duty = 0;
    ledc_channel.gpio_num = gpio_num_t(32);
    ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel.hpoint = 0;
    ledc_channel.timer_sel = LEDC_TIMER_0;
    ledc_channel_config(&ledc_channel);




    controller.onEnabled([](bool enabled) {
        gpio_set_level(gpio_num_t(14), enabled ? 1 : 0);
        ESP_LOGI(TAG, "Enabled. %d", (enabled ? 1 : 0));

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, enabled ? 90 : 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    });
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
