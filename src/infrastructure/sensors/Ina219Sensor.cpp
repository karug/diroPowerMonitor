#include "Ina219Sensor.h"
#include <Adafruit_INA219.h>

Ina219Sensor::Ina219Sensor(float shuntOhm)
    : ina_(new Adafruit_INA219()), shuntOhm_(shuntOhm) {}

Ina219Sensor::~Ina219Sensor() { delete ina_; }

bool Ina219Sensor::begin() {
    if (!ina_->begin()) return false;
    // Calibrate for shunt value: default INA219 calibration assumes 0.1Ω
    // Custom shunt is applied via scaling the current reading
    return true;
}

Measurement Ina219Sensor::read() {
    Measurement m;
    m.voltage = ina_->getBusVoltage_V();
    // Scale current for configurable shunt: INA219 calibrated for 0.1Ω
    // If shunt differs, current scales inversely
    m.current = ina_->getCurrent_mA() * (0.1f / shuntOhm_) / 1000.0f;
    m.power = m.voltage * m.current;
    return m;
}
