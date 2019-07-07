#include "Certificates.hpp"
#include "WiFi.hpp"

#include <luna/esp32/StrandWS281x.hpp>
#include <luna/esp32/WS281xMeter.hpp>
#include <luna/esp32/PWMLight.hpp>

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
        "Piwnica",
        "mqtt://192.168.1.1/",
        my_key,  static_cast<size_t>(my_key_end - my_key),
        my_cert, static_cast<size_t>(my_cert_end - my_cert),
        ca_cert, static_cast<size_t>(ca_cert_end - ca_cert),
    };
}

constexpr float rgbLedCurrentDraw = 0.02f;
constexpr float whiteLedCurrentDraw = 4.8f;

struct LightEdge
{
    explicit LightEdge(Location const & location, int rgbPin, int whitePin, PWMTimer * pwmTimer) :
        mRGBDriver(rgbPin, 120),
        mRGBMeter(&mRGBDriver, rgbLedCurrentDraw),
        mRGB(location, &mRGBDriver, 120, 0),
        mWhite(location, whitePin, pwmTimer)
    {}

    StrandWS2812 * rgb() { return &mRGB; }
    WS281xMeter * rgbMeter() { return &mRGBMeter; }
    PWMLight * white() { return &mWhite; }
    void update()
    {
        mRGBMeter.measure();
        mRGBDriver.send();
    }
private:
    WS281xDriver mRGBDriver;
    WS281xMeter mRGBMeter;
    StrandWS2812 mRGB;
    PWMLight mWhite;
};

struct BasementLightController : HardwareController
{
    explicit BasementLightController() :
        mPWMTimer(0, 19520, 11),
        mPowerSupply(&mPWMTimer, 14, 32, 4.7f, 0.15f),
        mLeft(
            {{-1.0f, -1.3f, 0.0f}, {-1.0f, 1.0f, 0.0f}},
            27,
            25,
            &mPWMTimer
        ),
        mRight(
            {{1.0f, -1.3f, 0.0f}, {1.0, 1.0f, 0.0f}},
            26,
            33,
            &mPWMTimer
        )
    {
        mPowerSupply.observe5V({mLeft.rgbMeter(), mRight.rgbMeter()});
        mPowerSupply.observe12V({mLeft.white(), mRight.white()});
    }
    
    std::vector<Strand *> strands() override
    {
        return {
            mLeft.rgb(),
            mRight.rgb(),
            mLeft.white(),
            mRight.white(),
        };
    }
    
    void enabled(bool value) override
    {
        mPowerSupply.enabled(value);
    }
    
    void update()
    {   
        mLeft.update();
        mRight.update();
        mPowerSupply.balanceLoad();
    }
private:
    PWMTimer mPWMTimer;
    ATX mPowerSupply;
    LightEdge mLeft;
    LightEdge mRight;
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
