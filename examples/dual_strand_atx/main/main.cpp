#include "Certificates.hpp"

#include <luna/Device.hpp>
#include <luna/PWMLight.hpp>
#include <luna/StrandWS281x.hpp>
#include <luna/WS281xMeter.hpp>
#include <luna/ATX.hpp>

#include <luna/ConstantEffect.hpp>
#include <luna/FlameEffect.hpp>
#include <luna/PlasmaEffect.hpp>

#include <luna/EffectPlugin.hpp>
#include <luna/MqttPlugin.hpp>
#include <luna/UpdatePlugin.hpp>
#include <luna/PersistencyPlugin.hpp>
#include <luna/RealtimePlugin.hpp>

#include <luna/Luna.hpp>

#include <esp_log.h>

static char const TAG[] = "App";

using namespace luna;

constexpr float rgbLedCurrentDraw = 0.02f;
constexpr float whiteLedCurrentDraw = 4.8f;

struct LightEdge
{
    explicit LightEdge(Location const & location, int rgbPin, int whitePin, PWMTimer * pwmTimer) :
        mRGBDriver(rgbPin, 120),
        mRGBMeter(&mRGBDriver, rgbLedCurrentDraw),
        mRGB(location, &mRGBDriver, 120, 0),
        mWhitePwm(pwmTimer, whitePin),
        mWhite(location, prism::sRGB(), {{&mWhitePwm, PWMLight::White, whiteLedCurrentDraw}})
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
    PWM mWhitePwm;
    PWMLight mWhite;
};

struct CinemaLightController : Device
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

extern "C" void app_main()
{
    CinemaLightController device;

    ConstantEffect light("light");
    FlameEffect flame("flame");
    PlasmaEffect plasma("plasma");

    EffectPlugin effects({&light, &flame, &plasma});
    PersistencyPlugin persistency(&effects);
    MqttPlugin mqttPlugin("Piwnica", "mqtt://192.168.1.1", &effects, 255.0f);
    UpdatePlugin update;
    RealtimePlugin realtime("Piwnica");

    LunaConfiguration config{
        .plugins = {&effects, &persistency, &mqttPlugin, &update, &realtime},
        .device = &device,
        .wifiCredentials{
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASSWORD
        },
        .tlsCredentials{
            .ownKey{myKey, size_t(myKeyEnd - myKey)},
            .ownCertificate{myCert, size_t(myCertEnd - myCert)},
            .caCertificate{caCert, size_t(caCertEnd - caCert)},
        }
    };

    Luna lun(&config);
    lun.run();
}
