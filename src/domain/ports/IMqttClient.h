#pragma once
#include "../entities/AppState.h"

class IMqttClient {
public:
    virtual void publish(const AppState& state) = 0;
    virtual ~IMqttClient() = default;
};
