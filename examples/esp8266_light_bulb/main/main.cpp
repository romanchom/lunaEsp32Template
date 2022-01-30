#include <luna/ConstantEffect.hpp>
#include <luna/FlameEffect.hpp>
#include <luna/PlasmaEffect.hpp>

#include <luna/EffectPlugin.hpp>
#include <luna/MqttPlugin.hpp>
#include <luna/PersistencyPlugin.hpp>
#include <luna/UpdatePlugin.hpp>

#include <luna/Luna.hpp>

#include <luna/Device.hpp>
#include <luna/PWMLight.hpp>
#include <luna/SoftPWM.hpp>

#include <esp_log.h>

static char const TAG[] = "App";

using namespace luna;

enum {
    GPIO_RED = 4,
    GPIO_GREEN = 12,
    GPIO_BLUE = 14,
    GPIO_COOL_WHITE = 5,
    GPIO_WARM_WHITE = 13,
};

struct Lamp : Device
{
    explicit Lamp() :
        mPWM({
            GPIO_RED,
            GPIO_GREEN,
            GPIO_BLUE,
            GPIO_COOL_WHITE,
            GPIO_WARM_WHITE,
        }),
        mLight({{0, 0, 0}, {0, 0, 0}}, prism::sRGB(), {
            {mPWM.getChannel(0), PWMLight::Red},
            {mPWM.getChannel(1), PWMLight::Green},
            {mPWM.getChannel(2), PWMLight::Blue},
            {mPWM.getChannel(3), PWMLight::CoolWhite},
            {mPWM.getChannel(4), PWMLight::WarmWhite},
        })
    {}

    std::vector<Strand *> strands() final
    {
        return {
            &mLight,
        };
    }

    void enabled(bool value) final
    {}

    void update() final
    {
        mPWM.commit();
    }
private:
    SoftPWM mPWM;
    PWMLight mLight;
};

extern "C" void app_main()
{
    Lamp device;

    ConstantEffect light;
    FlameEffect flame;
    PlasmaEffect plasma;

    EffectPlugin effects({
        {"light", &light},
        {"flame", &flame},
        {"plasma",&plasma}
    });
    PersistencyPlugin persistency(&effects);
    MqttPlugin mqttPlugin("mqtt://192.168.4.8", &effects, 255.0f);
    UpdatePlugin update;

    LunaConfiguration config{
        .name = "tableLamp",
        .plugins = {&persistency, &effects, &mqttPlugin, &update},
        .device = &device,
        .wifiCredentials{
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASSWORD
        }
    };

    Luna lun(&config);
    lun.run();
}
