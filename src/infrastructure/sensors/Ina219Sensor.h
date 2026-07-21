#pragma once
#include <cstdint>
#include "../../domain/ports/IEnergySensor.h"

class Adafruit_INA219;

class Ina219Sensor : public IEnergySensor {
public:
    explicit Ina219Sensor(float shuntOhm = 0.1f);
    ~Ina219Sensor() override;

    bool begin();
    Measurement read() override;

private:
    Adafruit_INA219* ina_;
    float shuntOhm_;
};
