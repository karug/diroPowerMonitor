#pragma once
#include "../entities/Rpm.h"

class IRpmSensor {
public:
    virtual Rpm read() = 0;
    virtual ~IRpmSensor() = default;
};
