#include "Certificates.hpp"

#include <luna/EventLoop.hpp>

#include <luna/ConstantEffect.hpp>
#include <luna/FlameEffect.hpp>
#include <luna/PlasmaEffect.hpp>

#include <luna/EffectPlugin.hpp>
#include <luna/MqttPlugin.hpp>
#include <luna/UpdatePlugin.hpp>
#include <luna/PersistencyPlugin.hpp>

#include <luna/Luna.hpp>

#include <luna/Device.hpp>
#include <luna/StrandWS281x.hpp>

#include <esp_log.h>

static char const TAG[] = "App";

using namespace luna;

struct Lamp : Device
{
    explicit Lamp() :
        mPhysicalStrand(
            14,
            94
        ),
        mStrandTop(
            {{1.0f, 1.0f, 0.0f}, {-1.9f, 1.0f, 0.0f}},
            &mPhysicalStrand,
            58,
            36
        ),
        mStrandSide(
            {{1.0f, -0.9f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            &mPhysicalStrand,
            36,
            0
        )
    {}

    std::vector<Strand *> strands() final
    {
        return {
            &mStrandTop,
            &mStrandSide,
        };
    }

    void enabled(bool value) final
    {}

    void update() final
    {
        mPhysicalStrand.send();
    }
private:
    WS281xDriver mPhysicalStrand;
    StrandWS2811 mStrandTop;
    StrandWS2811 mStrandSide;
};

extern "C" void app_main()
{
    Lamp device;

    ConstantEffect light("light");
    FlameEffect flame("flame");
    PlasmaEffect plasma("plasma");

    EffectPlugin effects({&light, &flame, &plasma});
    PersistencyPlugin persistency(&effects);
    MqttPlugin mqttPlugin("bedroom", "mqtt://192.168.4.8", &effects, 255.0f);
    UpdatePlugin update;

    LunaConfiguration config{
        .plugins = {&effects, &mqttPlugin, &update,},
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
