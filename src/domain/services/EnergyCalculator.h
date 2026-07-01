#pragma once
#include <cstdint>
#include "../entities/EnergyStatistics.h"

class EnergyCalculator {
public:
    void update(float powerW, uint32_t nowMs);
    void resetToday();
    EnergyStatistics statistics() const;

private:
    float whToday_{0.0f};
    float whTotal_{0.0f};
    uint32_t lastMs_{0};
    bool firstUpdate_{true};
};
