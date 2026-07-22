#include "MqttClient.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "../../Logger.h"

static constexpr char kClientId[] = "wind-monitor";

MqttClient::MqttClient(const std::string& host, uint16_t port)
    : host_(host), port_(port),
      wifiClient_(new WiFiClient()),
      mqtt_(new PubSubClient(*wifiClient_)) {
    if (!host_.empty()) {
        mqtt_->setServer(host_.c_str(), port_);
        enabled_ = true;
    }
}

MqttClient::~MqttClient() {
    delete mqtt_;
    delete wifiClient_;
}

void MqttClient::ensureConnected() {
    if (!enabled_ || mqtt_->connected()) return;
    if (WiFi.status() != WL_CONNECTED) return;
    mqtt_->connect(kClientId);
}

void MqttClient::publish(const AppState& state) {
    ensureConnected();
    if (!enabled_ || !mqtt_->connected()) return;

    char buf[16];

    snprintf(buf, sizeof(buf), "%.2f", state.measurement.voltage);
    mqtt_->publish("wind/voltage", buf);

    snprintf(buf, sizeof(buf), "%.3f", state.measurement.current);
    mqtt_->publish("wind/current", buf);

    snprintf(buf, sizeof(buf), "%.1f", state.measurement.power);
    mqtt_->publish("wind/power", buf);

    snprintf(buf, sizeof(buf), "%u", state.rpm.value);
    mqtt_->publish("wind/rpm", buf);

    snprintf(buf, sizeof(buf), "%.1f", state.energy.whToday);
    mqtt_->publish("wind/energy/today", buf);

    snprintf(buf, sizeof(buf), "%.3f", state.energy.whTotal / 1000.0f);
    mqtt_->publish("wind/energy/total", buf);

    snprintf(buf, sizeof(buf), "%u", state.battery.percent);
    mqtt_->publish("wind/battery", buf);

    mqtt_->loop();
}
