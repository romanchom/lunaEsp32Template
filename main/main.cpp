#include "Certificates.hpp"
#include "WiFi.hpp"

#include <luna/esp32/StrandWS281x.hpp>
#include <luna/esp32/PWMLight.hpp>
#include <luna/esp32/Located.hpp>
#include <luna/esp32/Metered.hpp>

#include <luna/esp32/NetworkManager.hpp>
#include <luna/esp32/HardwareController.hpp>
#include <luna/esp32/ATX.hpp>

#include <esp_log.h>
#include <nvs_flash.h>

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

NetworkManagerConfiguration networkConfig()
{
    return {
        my_key,  static_cast<size_t>(my_key_end - my_key),
        my_cert, static_cast<size_t>(my_cert_end - my_cert),
        ca_cert, static_cast<size_t>(ca_cert_end - ca_cert),
    };
}

constexpr float rgbLedCurrentDraw = 0.02f;
constexpr float whiteLedCurrentDraw = 4.8f;

struct BasementLightController : HardwareController
{
    explicit BasementLightController() :
        mPWMTimer(0, 19520, 11),
        mPowerSupply(&mPWMTimer, 14, 32, 4.7f, 0.15f),
        mLeftRGB(
            rgbLedCurrentDraw,
            Location{
                {-1.0f, -1.3f},
                {-1.0f, 1.0f}
            },
            120,
            27
        ),
        mRightRGB(
            rgbLedCurrentDraw,
            Location{
                {1.0f, -1.3f},
                {1.0f, 1.0f}
            },
            120,
            26
        ),
        mLeftWhite(
            whiteLedCurrentDraw,
            Location{
                {-1, -1.3f},
                {-1, 1.0f}
            },
            25,
            &mPWMTimer
        ),
        mRightWhite(
            whiteLedCurrentDraw,
            Location{
                {1, -1.3f},
                {1, 1.0f}
            },
            33,
            &mPWMTimer
        )
    {
        mPowerSupply.observe5V({&mLeftRGB, &mRightRGB});
        mPowerSupply.observe12V({&mLeftWhite, &mRightWhite});
    }
    
    std::vector<StrandBase *> strands() override
    {
        return {
            &mLeftRGB,
            &mRightRGB,
            &mLeftWhite,
            &mRightWhite,
        };
    }
    
    void enabled(bool value) override
    {
        mPowerSupply.enabled(value);
    }
    
    void update()
    {   
        mPowerSupply.balanceLoad();
    }
private:
    PWMTimer mPWMTimer;
    ATX mPowerSupply;
    Metered<Located<StrandWS2812>> mLeftRGB;
    Metered<Located<StrandWS2812>> mRightRGB;
    Metered<Located<PWMLight>> mLeftWhite;
    Metered<Located<PWMLight>> mRightWhite;
};

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
    void connected(ip4_addr_t address) override
    {
        ESP_LOGI(TAG, "Connected");
        mLunaNetworkManager.enable();
    }

    void disconnected() override
    {
        mLunaNetworkManager.disable();
    }

    BasementLightController mController;
    NetworkManager mLunaNetworkManager;
    WiFi mWiFi;
};

extern "C" void app_main()
{
    initializeNonVolatileStorage();

    new WiFiLuna();
}
