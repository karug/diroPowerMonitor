#pragma once
#include <string>
#include "../../domain/ports/IMqttClient.h"

class WiFiClient;
class PubSubClient;

class MqttClient : public IMqttClient {
public:
    MqttClient(const std::string& host, uint16_t port);
    ~MqttClient() override;

    void publish(const AppState& state) override;

private:
    void ensureConnected();

    std::string host_;
    uint16_t port_;
    WiFiClient* wifiClient_;
    PubSubClient* mqtt_;
    bool enabled_{false};
};
