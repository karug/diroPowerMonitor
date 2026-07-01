#pragma once
#include <cstdint>
#include "../entities/Rpm.h"

class RpmCalculator {
public:
    explicit RpmCalculator(uint8_t pulsesPerRev = 1);
    void setPulsesPerRev(uint8_t ppr);
    Rpm calculate(uint32_t pulseCount, uint32_t windowMs) const;

private:
    uint8_t pulsesPerRev_;
};
