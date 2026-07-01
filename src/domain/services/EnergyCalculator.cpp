#include "EnergyCalculator.h"

void EnergyCalculator::update(float powerW, uint32_t nowMs) {
    if (!firstUpdate_) {
        float deltaHours = static_cast<float>(nowMs - lastMs_) / 3600000.0f;
        float wh = powerW * deltaHours;
        whToday_ += wh;
        whTotal_ += wh;
    }
    firstUpdate_ = false;
    lastMs_ = nowMs;
}

void EnergyCalculator::resetToday() {
    whToday_ = 0.0f;
    firstUpdate_ = true;
}

EnergyStatistics EnergyCalculator::statistics() const {
    EnergyStatistics s;
    s.whToday = whToday_;
    s.whTotal = whTotal_;
    return s;
}
