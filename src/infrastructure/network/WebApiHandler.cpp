#include "WebApiHandler.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

WebApiHandler::WebApiHandler(AppStateGetter getter,
                             HistoryRepository& history,
                             PreferencesRepository& config)
    : getter_(getter), history_(history), config_(config),
      server_(new WebServer(80)) {}

WebApiHandler::~WebApiHandler() { delete server_; }

void WebApiHandler::begin() {
    server_->on("/", HTTP_GET, [this]() {
        File f = LittleFS.open("/index.html", "r");
        if (!f) { server_->send(404, "text/plain", "Not found"); return; }
        server_->streamFile(f, "text/html");
        f.close();
    });

    server_->on("/api/status", HTTP_GET, [this]() {
        AppState s = getter_();
        JsonDocument doc;
        doc["voltage"]  = s.measurement.voltage;
        doc["current"]  = s.measurement.current;
        doc["power"]    = s.measurement.power;
        doc["rpm"]      = s.rpm.value;
        doc["whToday"]  = s.energy.whToday;
        doc["whTotal"]  = s.energy.whTotal;
        doc["batPct"]   = s.battery.percent;
        doc["charging"] = s.battery.charging;
        String out;
        serializeJson(doc, out);
        server_->send(200, "application/json", out);
    });

    server_->on("/api/history", HTTP_GET, [this]() {
        server_->send(200, "application/json", "[]");
    });

    server_->on("/api/config", HTTP_POST, [this]() {
        String body = server_->arg("plain");
        JsonDocument doc;
        if (deserializeJson(doc, body)) {
            server_->send(400, "application/json", "{\"error\":\"bad json\"}");
            return;
        }
        if (doc["mqtt_host"].is<const char*>())
            config_.setMqttHost(doc["mqtt_host"].as<std::string>());
        if (doc["mqtt_port"].is<uint16_t>())
            config_.setMqttPort(doc["mqtt_port"].as<uint16_t>());
        if (doc["pulses_rev"].is<uint8_t>())
            config_.setPulsesPerRev(doc["pulses_rev"].as<uint8_t>());
        if (doc["shunt_ohm"].is<float>())
            config_.setShuntOhm(doc["shunt_ohm"].as<float>());
        server_->send(200, "application/json", "{\"ok\":true}");
    });

    server_->begin();
}

void WebApiHandler::handle() {
    server_->handleClient();
}
