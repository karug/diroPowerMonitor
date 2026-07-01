#pragma once
#include <cstdint>

class BatteryEstimator {
public:
    uint8_t estimate(float voltageV) const;
};
