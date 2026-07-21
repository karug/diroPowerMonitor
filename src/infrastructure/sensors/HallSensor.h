#pragma once
#include <cstdint>
#include "../../domain/ports/IRpmSensor.h"
#include "../../domain/services/RpmCalculator.h"

class HallSensor : public IRpmSensor {
public:
    HallSensor(uint8_t pin, uint8_t pulsesPerRev = 1);

    void begin();
    Rpm read() override;

    static void isr();

private:
    uint8_t pin_;
    uint8_t pulsesPerRev_;
    RpmCalculator calc_;
    Rpm lastRpm_;
    uint32_t windowStartMs_{0};

    static volatile uint32_t pulseCount_;
};
