#include "Certificates.hpp"
#include "WiFi.hpp"

#include <luna/esp32/NetworkManager.hpp>
#include <luna/esp32/HardwareController.hpp>
#include <luna/esp32/StrandWS281x.hpp>
#include <luna/esp32/Outputs.hpp>
#include <luna/esp32/Updater.hpp>

#include <esp_log.h>
#include <nvs_flash.h>

#include <memory>

static char const TAG[] = "App";

static void initializeNonVolatileStorage()
{
    esp_err_t ret = nvs_flash_init();

    if (ESP_ERR_NVS_NO_FREE_PAGES == ret) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

using namespace luna::esp32;
using namespace luna::proto;

struct RoomLightController : public luna::esp32::HardwareController
{
    explicit RoomLightController() :
        physicalStrand(
            output5,
            93
        ),
        rightStrand(
            {{1.5f, -3.0f, 0.0f}, {1.5f, 2.0f, 0.0f}},
            &physicalStrand,
            36,
            0
        ),
        topStrand(
            {{1.5f, 2.0f, 0.0f}, {-4.0f, 2.0f, 0.0f}},
            &physicalStrand,
            57,
            36
        )
    {}

    std::vector<Strand *> strands() final
    {
        return {
            &rightStrand,
            &topStrand
        };
    }

    void enabled(bool value) final
    {}

    void update() final
    {
        physicalStrand.send();
    }
private:
    WS281xDriver physicalStrand;
    StrandWS2811 rightStrand;
    StrandWS2811 topStrand;
};

NetworkManagerConfiguration networkConfig()
{
    return {
        "Pokoj",
        "mqtt://192.168.0.110:1883",
        my_key,  static_cast<size_t>(my_key_end - my_key),
        my_cert, static_cast<size_t>(my_cert_end - my_cert),
        ca_cert, static_cast<size_t>(ca_cert_end - ca_cert),
    };
}

struct WiFiLuna : private WiFi::Observer
{
    explicit WiFiLuna() :
        mController(),
        mLunaNetworkManager(networkConfig(), &mController),
        mWiFi(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD)
    {
        mWiFi.observer(this);
        mWiFi.enabled(true);
    }
private:
    void connected(ip4_addr_t address) final
    {
        mLunaNetworkManager.enable();
    }

    void disconnected() final
    {
        mLunaNetworkManager.disable();
    }

    RoomLightController mController;
    NetworkManager mLunaNetworkManager;
    WiFi mWiFi;
};

extern "C" void app_main()
{
    initializeNonVolatileStorage();

    new WiFiLuna();
}
