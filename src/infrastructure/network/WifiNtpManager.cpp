#include "WifiNtpManager.h"
#include <WiFiManager.h>
#include <Arduino.h>
#include <time.h>

static constexpr char kApName[] = "WindMonitor-Setup";
// AP password intentionally empty — open portal for provisioning only
// No secrets hardcoded per project security policy

static WiFiManager wm;
static WifiNtpManager::OnConnectCallback sOnConnect;

void WifiNtpManager::begin(OnConnectCallback onConnect) {
    sOnConnect = onConnect;

    wm.setConfigPortalTimeout(180);  // 3 min portal timeout
    wm.setSaveConfigCallback([]() {
        if (sOnConnect) sOnConnect();
    });

    if (!wm.autoConnect(kApName)) {
        // Portal timed out — continue without WiFi
        return;
    }

    // WiFi connected — sync NTP
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    if (sOnConnect) sOnConnect();
}

bool WifiNtpManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool WifiNtpManager::ntpSynced() const {
    time_t now = time(nullptr);
    return now > 1000000000UL;  // 2001-09-09 UTC — sane timestamp
}

void WifiNtpManager::process() {
    // WiFiManager handles reconnect internally
}
