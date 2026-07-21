#pragma once
#include <cstdint>
#include <string>

struct WindConfig {
    std::string mqttHost;
    uint16_t    mqttPort{1883};
    uint8_t     pulsesPerRev{1};
    float       shuntOhm{0.1f};
    std::string otaPass;   // stored in NVS, never hardcoded
};

class Preferences;

class PreferencesRepository {
public:
    PreferencesRepository();
    ~PreferencesRepository();

    void begin();
    WindConfig loadConfig();
    void saveConfig(const WindConfig& cfg);

    // Convenience setters for individual keys (used by /api/config endpoint)
    void setMqttHost(const std::string& host);
    void setMqttPort(uint16_t port);
    void setPulsesPerRev(uint8_t ppr);
    void setShuntOhm(float ohm);
    void setOtaPass(const std::string& pass);

private:
    Preferences* prefs_;
};
