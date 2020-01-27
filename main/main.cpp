#include "Certificates.hpp"

#include <luna/EventLoop.hpp>

#include <luna/ConstantEffect.hpp>
#include <luna/FlameEffect.hpp>
#include <luna/PlasmaEffect.hpp>

#include <luna/EffectsPlugin.hpp>
#include <luna/MqttPlugin.hpp>
#include <luna/UpdatePlugin.hpp>
#include <luna/PersistencyPlugin.hpp>
#include <luna/RealtimePlugin.hpp>

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
        mPhysicalStrand(
            32,
            100
        ),
        mStrand(
            {{-3.0f, 2.0f, 0.0f}, {2.0f, 2.0f, 0.0f}},
            &mPhysicalStrand,
            100,
            0
        )
    {}

    std::vector<Strand *> strands() final
    {
        return {
            &mStrand
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
    StrandWS2811 mStrand;
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
    MqttPlugin mqttPlugin("pokoj", "mqtt://192.168.1.100", &mainLoop, &effects.effectEngine());
    UpdatePlugin update;
    RealtimePlugin realtime("pokoj", &hardware);

    Ir40ButtonRemote demux(&effects.effectEngine(), &light);
    Infrared ir(&mainLoop, 12, &demux);

    LunaConfiguration config{
        .plugins = {&effects, &mqttPlugin, &update, &realtime},
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
