#pragma once
#include <string>

class OtaManager {
public:
    // Initializes ArduinoOTA with password from NVS.
    // If password is empty, OTA is disabled (not started).
    void begin(const std::string& otaPass);

    // Must be called every loop iteration to handle OTA events
    void handle();

    bool enabled() const { return enabled_; }

private:
    bool enabled_{false};
};
