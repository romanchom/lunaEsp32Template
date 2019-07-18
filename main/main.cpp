#include "Certificates.hpp"

#include <luna/Main.hpp>
#include <luna/NetworkManager.hpp>
#include <luna/HardwareController.hpp>
#include <luna/StrandWS281x.hpp>

#include <esp_log.h>

static char const TAG[] = "App";

using namespace luna;
using namespace std::literals;

struct RoomLightController : public HardwareController
{
    explicit RoomLightController() :
        physicalStrand(
            14,
            93
        ),
        rightStrand(
            {{1.5f, -3.0f, 0.0f}, {1.5f, 2.0f, 0.0f}},
            &physicalStrand,
            36,
            0
        ),
        topStrand(
            {{1.5f, 2.0f, 0.0f}, {-4.0f, 2.0f, 0.0f}},
            &physicalStrand,
            57,
            36
        )
    {}

    std::vector<Strand *> strands() final
    {
        return {
            &rightStrand,
            &topStrand
        };
    }

    void enabled(bool value) final
    {}

    void update() final
    {
        physicalStrand.send();
    }
private:
    WS281xDriver physicalStrand;
    StrandWS2811 rightStrand;
    StrandWS2811 topStrand;
};

Configuration const config{
    .wifi{
        .ssid = CONFIG_ESP_WIFI_SSID,
        .password = CONFIG_ESP_WIFI_PASSWORD
    },
    .network{
        .name = "pokoj"sv,
        .mqttAddress = "mqtts://192.168.0.233:8883"sv,
        .tls{
            .ownKey{myKey, size_t(myKeyEnd - myKey)},
            .ownCertificate{myCert, size_t(myCertEnd - myCert)},
            .caCertificate{caCert, size_t(caCertEnd - caCert)},
        },
    },
};

extern "C" void app_main()
{
    new Main(config, new RoomLightController());
}
