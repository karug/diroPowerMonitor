#include "WebApiHandler.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

WebApiHandler::WebApiHandler(AppStateGetter getter,
                             HistoryRepository& history,
                             PreferencesRepository& config)
    : getter_(getter), history_(history), config_(config),
      server_(new AsyncWebServer(80)) {}

WebApiHandler::~WebApiHandler() { delete server_; }

void WebApiHandler::begin() {
    // Serve dashboard from LittleFS
    server_->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // GET /api/status — current AppState as JSON
    server_->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
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
        req->send(200, "application/json", out);
    });

    // GET /api/history — placeholder (full impl in Task 12)
    server_->on("/api/history", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", "[]");
    });

    // POST /api/config — update NVS config keys
    server_->on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
               size_t index, size_t total) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                req->send(400, "application/json", "{\"error\":\"bad json\"}");
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
            req->send(200, "application/json", "{\"ok\":true}");
        });

    server_->begin();
}
