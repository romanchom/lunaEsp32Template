#include "Certificates.hpp"

#include <luna/Main.hpp>
#include <luna/HardwareController.hpp>
#include <luna/StrandWS281x.hpp>
#include <luna/ATX.hpp>
#include <luna/PWMLight.hpp>
#include <luna/WS281xMeter.hpp>

#include <esp_log.h>

static char const TAG[] = "App";

using namespace luna;
using namespace std::literals;

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

struct CinemaLightController : HardwareController
{
    explicit CinemaLightController() :
        mPWMTimer(0, 19520, 11),
        mPowerSupply(&mPWMTimer, 14, 32, 4.7f, 0.20f),
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

Configuration const config{
    .wifi{
        .ssid = CONFIG_ESP_WIFI_SSID,
        .password = CONFIG_ESP_WIFI_PASSWORD
    },
    .network{
        .name = "Piwnica"sv,
        .mqttAddress = "mqtt://192.168.1.1"sv,
        .tls{
            .ownKey{myKey, size_t(myKeyEnd - myKey)},
            .ownCertificate{myCert, size_t(myCertEnd - myCert)},
            .caCertificate{caCert, size_t(caCertEnd - caCert)},
        },
    },
};

extern "C" void app_main()
{
    new Main(config, new CinemaLightController());
}
