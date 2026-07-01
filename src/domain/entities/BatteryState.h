#pragma once
#include <cstdint>

struct BatteryState {
    uint8_t percent{0};
    bool charging{false};
};
