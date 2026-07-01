#pragma once
#include "../entities/Measurement.h"

class IEnergySensor {
public:
    virtual Measurement read() = 0;
    virtual ~IEnergySensor() = default;
};
