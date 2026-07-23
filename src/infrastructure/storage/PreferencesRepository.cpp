#include "PreferencesRepository.h"
#include <Preferences.h>

static constexpr char kNs[] = "dpm";

PreferencesRepository::PreferencesRepository() : prefs_(new Preferences()) {}
PreferencesRepository::~PreferencesRepository() { delete prefs_; }

void PreferencesRepository::begin() {
    prefs_->begin(kNs, false);
}

WindConfig PreferencesRepository::loadConfig() {
    WindConfig cfg;
    cfg.mqttHost     = prefs_->getString("mqtt_host", "").c_str();
    cfg.mqttPort     = prefs_->getUShort("mqtt_port", 1883);
    cfg.pulsesPerRev = prefs_->getUChar("pulses_rev", 1);
    cfg.shuntOhm     = prefs_->getFloat("shunt_ohm", 0.1f);
    cfg.otaPass      = prefs_->getString("ota_pass", "").c_str();
    return cfg;
}

void PreferencesRepository::saveConfig(const WindConfig& cfg) {
    prefs_->putString("mqtt_host", cfg.mqttHost.c_str());
    prefs_->putUShort("mqtt_port", cfg.mqttPort);
    prefs_->putUChar("pulses_rev", cfg.pulsesPerRev);
    prefs_->putFloat("shunt_ohm", cfg.shuntOhm);
    prefs_->putString("ota_pass", cfg.otaPass.c_str());
}

void PreferencesRepository::setMqttHost(const std::string& host) {
    prefs_->putString("mqtt_host", host.c_str());
}
void PreferencesRepository::setMqttPort(uint16_t port) {
    prefs_->putUShort("mqtt_port", port);
}
void PreferencesRepository::setPulsesPerRev(uint8_t ppr) {
    prefs_->putUChar("pulses_rev", ppr);
}
void PreferencesRepository::setShuntOhm(float ohm) {
    prefs_->putFloat("shunt_ohm", ohm);
}
void PreferencesRepository::setOtaPass(const std::string& pass) {
    prefs_->putString("ota_pass", pass.c_str());
}
