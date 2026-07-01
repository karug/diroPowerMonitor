#include "RpmCalculator.h"

RpmCalculator::RpmCalculator(uint8_t pulsesPerRev) : pulsesPerRev_(pulsesPerRev) {}

void RpmCalculator::setPulsesPerRev(uint8_t ppr) {
    pulsesPerRev_ = (ppr == 0) ? 1 : ppr;
}

Rpm RpmCalculator::calculate(uint32_t pulseCount, uint32_t windowMs) const {
    if (windowMs == 0 || pulseCount == 0) { Rpm r; r.value = 0; return r; }
    float revolutions = static_cast<float>(pulseCount) / pulsesPerRev_;
    Rpm r;
    r.value = static_cast<uint16_t>(revolutions / static_cast<float>(windowMs) * 60000.0f);
    return r;
}
