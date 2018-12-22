#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include <cstring>
#include <memory>

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD

static char const TAG[] = "App";

static void wifiConnected();
static void wifiDisconnected();
extern "C" void app_main();

static void initializeNonVolatileStorage()
{
    esp_err_t ret = nvs_flash_init();

    if (ESP_ERR_NVS_NO_FREE_PAGES == ret) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        wifiConnected();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        wifiDisconnected();
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initializeWiFi()
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strcpy(reinterpret_cast<char *>(wifi_config.sta.ssid), EXAMPLE_ESP_WIFI_SSID);
    strcpy(reinterpret_cast<char *>(wifi_config.sta.password), EXAMPLE_ESP_WIFI_PASS);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized.");
}

#include "lwip_async/DtlsInputOutput.hpp"
#include "Certificates.hpp"
#include "DiscoveryResponder.hpp"
#include <luna/proto/Discovery_generated.h>
#include <luna/proto/Commands_generated.h>
#include <WS281xDriver.hpp>
#include <unordered_map>

auto ownKey = tls::PrivateKey(my_key, my_key_end - my_key);
auto ownCertificate = tls::Certificate(my_cert, my_cert_end - my_cert);
auto caCertificate = tls::Certificate(ca_cert, ca_cert_end - ca_cert);


struct Point {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct UV {
    float u = 0.0f;
    float v = 0.0f;
};

struct ColorSpace {
    UV white;
    UV red;
    UV green;
    UV blue;
};

struct StrandConfig {
    uint16_t pixelCount;
    uint8_t channels;
    Point begin;
    Point end;
    ColorSpace colorSpace;
};

class Strand {
public:
    virtual ~Strand() {}
    virtual void setData(luna::proto::StrandData const * data) = 0;

    StrandConfig const & config() const
    {
        return mConfig;
    }
protected:
    explicit Strand(StrandConfig const & config) :
        mConfig(config)
    {}
    StrandConfig mConfig;
};

struct StrandWS2812Config : StrandConfig {
    int GPIO;
};

class StrandWS2812 : public Strand {
public:
    explicit StrandWS2812(StrandWS2812Config const & config) :
        Strand(config),
        mDriver(config.GPIO, config.pixelCount)
    {
        mConfig.channels = 7;
    }

    ~StrandWS2812() override {};
    
    void setData(luna::proto::StrandData const * data) override
    {
        auto src = data->data_as_RGBData();
        if (src) {
            auto dest = mDriver.data().data();
            memcpy(dest, reinterpret_cast<uint8_t const*>(src), mConfig.pixelCount * 3);
            mDriver.send();
        }
    }

private:
    WS281xDriver mDriver;
};


luna::proto::Point toProto(Point const & point) {
    return luna::proto::Point(point.x, point.y, point.z);
}

luna::proto::UV toProto(UV const & uv) {
    return luna::proto::UV(uv.u, uv.v);
}   

luna::proto::ColorSpace toProto(ColorSpace const & colorSpace) {
    return luna::proto::ColorSpace(
        toProto(colorSpace.white),
        toProto(colorSpace.red),
        toProto(colorSpace.green),
        toProto(colorSpace.blue)
    );
}

std::vector<Strand *> strands;

std::unordered_map<luna::proto::AnyCommand, std::function<void(void const *)>, std::hash<int>> commandMap;

void dispatchCommand(lwip_async::DtlsInputOutput& sender, uint8_t const* data, size_t size)
{
    auto command = luna::proto::GetCommand(data);
    auto it = commandMap.find(command->command_type());
    if (it != commandMap.end()) {
        auto & executor = it->second;
        executor(command->command());
    } else {
        ESP_LOGI(TAG, "No executor.");
    }
}

static void wifiConnected()
{
    commandMap.emplace(luna::proto::AnyCommand_SetColor, [](void const * data){

        auto cmd = static_cast<luna::proto::SetColor const *>(data);
        auto const strands = cmd->strands();
        if (strands == nullptr) return;

        for (auto strandData : *strands) {
            auto index = strandData->id();
            if (index >= ::strands.size()) continue;
            
            auto strand = ::strands[index];
            strand->setData(strandData);
        }
    });

    auto dtls = new lwip_async::DtlsInputOutput(ownKey, ownCertificate, caCertificate);
    dtls->onDataRead(dispatchCommand);

    StrandWS2812Config config;
    config.pixelCount = 8;
    config.begin = {-1.0f, -1.0f};
    config.end = {-1.0f, 1.0f};
    config.GPIO = 14;
    strands.push_back(new StrandWS2812(config));

    std::vector<luna::proto::Strand> strandConfigs;
    for (int i = 0; i < strands.size(); ++i) {
        auto const & config = strands[i]->config();
        strandConfigs.emplace_back(
            i,
            (luna::proto::ColorChannels)config.channels,
            config.pixelCount,
            toProto(config.begin),
            toProto(config.end),
            toProto(config.colorSpace)
        );
    }
    
    auto discResp = new luna::esp32::DiscoveryResponder(dtls->port(), "Loszek", strandConfigs);
}

static void wifiDisconnected()
{

}

static void initializeLuna()
{
    
}

void app_main()
{
    initializeNonVolatileStorage();
    initializeLuna();
    initializeWiFi();
}
