#include "Certificates.hpp"

#include <luna/EventLoop.hpp>

#include <luna/ConstantEffect.hpp>
#include <luna/FlameEffect.hpp>
#include <luna/PlasmaEffect.hpp>

#include <luna/EffectsPlugin.hpp>
#include <luna/MqttPlugin.hpp>
#include <luna/UpdatePlugin.hpp>
#include <luna/PersistencyPlugin.hpp>

#include <luna/Infrared.hpp>
#include <luna/Ir40ButtonRemote.hpp>

#include <luna/Luna.hpp>

#include <luna/Device.hpp>
#include <luna/StrandWS281x.hpp>
#include <luna/ATX.hpp>
#include <luna/PWMLight.hpp>
#include <luna/WS281xMeter.hpp>

#include <esp_log.h>

static char const TAG[] = "App";

using namespace luna;

struct Lamp : Device
{
    explicit Lamp() :
        mPWMTimer(0, 19520, 11),
        mWhitePwm(&mPWMTimer, 14),
        mRedPwm(&mPWMTimer, 15),
        mGreenPwm(&mPWMTimer, 13),
        mBluePwm(&mPWMTimer, 2),
        mLight(
            Location{{0, 0, 0}, {0, 0, 0}},
            {
                {0.22271f, 0.26902f},
                {0.68934f, 0.31051f},
                {0.13173f, 0.77457f},
                {0.13450f, 0.04598f}
            },
            {
                {&mWhitePwm, PWMLight::White},
                {&mRedPwm, PWMLight::Red},
                {&mGreenPwm, PWMLight::Green},
                {&mBluePwm, PWMLight::Blue},
            }
        )
    {}

    std::vector<Strand *> strands() final
    {
        return {
            &mLight
        };
    }

    void enabled(bool value) final {}

    void update() final {}
private:
    PWMTimer mPWMTimer;
    PWM mWhitePwm;
    PWM mRedPwm;
    PWM mGreenPwm;
    PWM mBluePwm;
    PWMLight mLight;
};

extern "C" void app_main()
{
    EventLoop mainLoop;

    Lamp hardware;

    ConstantEffect light("light");
    FlameEffect flame("flame");
    PlasmaEffect plasma("plasma");

    EffectPlugin effects(&mainLoop, {&light, &flame, &plasma});
    PersistencyPlugin persistency(&effects.effectEngine());
    MqttPlugin mqttPlugin("pokoj", "mqtt://mqtt.lan", &mainLoop, &effects.effectEngine(), 255.0f);
    UpdatePlugin update;

    Ir40ButtonRemote demux(&effects.effectEngine(), &light, 0.25f);
    Infrared ir(&mainLoop, 12, &demux);

    LunaConfiguration config{
        .plugins = {&effects, &mqttPlugin, &update},
        .hardware = &hardware,
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

    Luna lun(&mainLoop, &config);
    mainLoop.execute();
}
