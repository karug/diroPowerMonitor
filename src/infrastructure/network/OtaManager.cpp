#include "OtaManager.h"
#include <ArduinoOTA.h>
#include "../../Logger.h"

void OtaManager::begin(const std::string& otaPass) {
    if (otaPass.empty()) {
        LOG_INFO("OTA disabled: no password in NVS");
        return;
    }

    ArduinoOTA.setPassword(otaPass.c_str());

    ArduinoOTA.onStart([]() {
        LOG_INFO("OTA start");
    });
    ArduinoOTA.onEnd([]() {
        LOG_INFO("OTA end");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        // intentionally silent to avoid Serial flooding
    });
    ArduinoOTA.onError([](ota_error_t error) {
        LOG_INFO("OTA error");
    });

    ArduinoOTA.begin();
    enabled_ = true;
    LOG_INFO("OTA ready");
}

void OtaManager::handle() {
    if (enabled_) ArduinoOTA.handle();
}
