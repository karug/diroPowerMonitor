#pragma once
#include <functional>

class WifiNtpManager {
public:
    using OnConnectCallback = std::function<void()>;

    void begin(OnConnectCallback onConnect = nullptr);
    bool isConnected() const;
    bool ntpSynced() const;

    // Called from loop to run WiFiManager background tasks
    void process();
};
