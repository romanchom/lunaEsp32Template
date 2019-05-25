#pragma once

#include <esp_event.h>
#include <lwip/ip4_addr.h>

struct WiFi
{
    struct Observer
    {
        virtual ~Observer() = default;
        virtual void connected(ip4_addr_t address) = 0;
        virtual void disconnected() = 0;
    };

    explicit WiFi(char const * ssid, char const * password);
    void observer(Observer * value);
    void enabled(bool value);
private:
    Observer * mObserver;
    static esp_err_t eventHandler(void * context, system_event_t * event);
    esp_err_t handleEvent(system_event_t * event);
};
