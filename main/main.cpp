#include "Certificates.hpp"
#include "WiFi.hpp"

#include <luna/esp32/NetworkManager.hpp>
#include <luna/esp32/HardwareController.hpp>
#include <luna/esp32/StrandWS281x.hpp>
#include <luna/esp32/Outputs.hpp>
#include <luna/esp32/Updater.hpp>
#include <luna/esp32/PWM.hpp>
#include <luna/esp32/PWMLight.hpp>
#include <luna/esp32/GPIO.hpp>

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

NetworkManagerConfiguration networkConfig()
{
    return {
        my_key,  static_cast<size_t>(my_key_end - my_key),
        my_cert, static_cast<size_t>(my_cert_end - my_cert),
        ca_cert, static_cast<size_t>(ca_cert_end - ca_cert),
    };
}

struct BasementLightController : HardwareController
{
    explicit BasementLightController() :
        mPowerEnable(14),
        mPWMTimer(0, 19520, 11),
        mDummyPWM(&mPWMTimer, 32),
        leftRGB(
            Location{
                {-1.0f, -1.3f},
                {-1.0f, 1.0f}
            },
            120,
            27
        ),
        rightRGB(
            Location{
                {1.0f, -1.3f},
                {1.0f, 1.0f}
            },
            120,
            26
        ),
        leftWhite(
            Location{
                {-1, -1.3f},
                {-1, 1.0f}
            },
            25,
            &mPWMTimer
        ),
        rightWhite(
            Location{
                {1, -1.3f},
                {1, 1.0f}
            },
            33,
            &mPWMTimer
        )
    {}
    
    std::vector<StrandBase *> strands() final
    {
        return {
            &leftRGB,
            &rightRGB,
            &leftWhite,
            &rightWhite,
        };
    }
    
    void enabled(bool value) final
    {
        mPowerEnable.out(value);
        mDummyPWM.duty(value ? 0.75f : 0.0f);
    }
private:
    luna::esp32::GPIO mPowerEnable;
    PWMTimer mPWMTimer;
    PWM mDummyPWM;
    StrandWS2812 leftRGB;
    StrandWS2812 rightRGB;
    PWMLight leftWhite;
    PWMLight rightWhite;
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
    void connected(ip4_addr_t address) final
    {
        ESP_LOGI(TAG, "Connected");
        mLunaNetworkManager.enable();
    }

    void disconnected() final
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
