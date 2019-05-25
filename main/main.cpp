#include "Certificates.hpp"
#include "WiFi.hpp"

#include <luna/esp32/NetworkManager.hpp>
#include <luna/esp32/HardwareController.hpp>
#include <luna/esp32/StrandWS281x.hpp>
#include <luna/esp32/Outputs.hpp>
#include <luna/esp32/Updater.hpp>
#include <luna/esp32/PWM.hpp>
#include <luna/esp32/PWMLight.hpp>

#include <esp_log.h>
#include <nvs_flash.h>
#include <driver/gpio.h>

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

std::unique_ptr<StrandBase> makeLeftStrand()
{
    return std::make_unique<StrandWS2811>(
        Location{
            {-1.0f, -1.3f},
            {-1.0f, 1.0f}
        },
        120,
        26
    );
}

std::unique_ptr<StrandBase> makeRightStrand()
{
    return std::make_unique<StrandWS2811>(
        Location{
            {1.0f, -1.3f},
            {1.0f, 1.0f}
        },
        120,
        27
    );
}

std::unique_ptr<StrandBase> makeWhite(PWMTimer * timer, float x, int pin)
{
    return std::make_unique<PWMLight>(
        Location{
            {x, -1.3f},
            {x, 1.0f}
        },
        pin,
        timer
    );
}

std::vector<std::unique_ptr<StrandBase>> makeStrands(PWMTimer * ledTimer)
{
    std::vector<std::unique_ptr<StrandBase>> ret;
    ret.emplace_back(makeLeftStrand());
    ret.emplace_back(makeRightStrand());
    ret.emplace_back(makeWhite(ledTimer, -1, 25));
    ret.emplace_back(makeWhite(ledTimer, 1, 33));
    return ret;
}

NetworkManagerConfiguration networkConfig()
{
    return {
        my_key,  static_cast<size_t>(my_key_end - my_key),
        my_cert, static_cast<size_t>(my_cert_end - my_cert),
        ca_cert, static_cast<size_t>(ca_cert_end - ca_cert),
    };
}

struct WiFiLuna : private WiFi::Observer
{
    explicit WiFiLuna() :
        mDummyTimer(0, 40000, 8),
        mLedTimer(1, 19520, 11),
        mDummyPWM(&mDummyTimer, 32),
        mController(makeStrands(&mLedTimer)),
        mLunaNetworkManager(networkConfig(), &mController),
        mWiFi(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD)

    {
        mWiFi.observer(this);

        gpio_config_t io_conf;
        io_conf.intr_type = gpio_int_type_t(GPIO_PIN_INTR_DISABLE);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = 1 << 14;
        io_conf.pull_down_en = gpio_pulldown_t(0);
        io_conf.pull_up_en = gpio_pullup_t(0);
        gpio_config(&io_conf);

        gpio_set_level(gpio_num_t(14), 0);

        mController.onEnabled([this](bool enabled) {
            gpio_set_level(gpio_num_t(14), enabled ? 1 : 0);
            ESP_LOGI(TAG, "Enabled. %d", (enabled ? 1 : 0));

            mDummyPWM.duty(enabled ? 0.75f : 0.0f);
        });

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

    PWMTimer mDummyTimer;
    PWMTimer mLedTimer;
    PWM mDummyPWM;
    HardwareController mController;
    NetworkManager mLunaNetworkManager;
    WiFi mWiFi;
};

extern "C" void app_main()
{
    initializeNonVolatileStorage();

    new WiFiLuna();
}
