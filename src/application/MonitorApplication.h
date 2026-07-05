#pragma once
#include <functional>
#include "../domain/ports/IEnergySensor.h"
#include "../domain/ports/IRpmSensor.h"
#include "../domain/ports/IDisplay.h"
#include "../domain/ports/IStorage.h"
#include "../domain/ports/IMqttClient.h"
#include "../domain/services/EnergyCalculator.h"
#include "../domain/services/BatteryEstimator.h"
#include "../domain/services/RpmCalculator.h"
#include "../domain/entities/AppState.h"

class MonitorApplication {
public:
    MonitorApplication(IEnergySensor& energySensor, IRpmSensor& rpmSensor,
                       IDisplay& display, IStorage& storage, IMqttClient& mqtt,
                       EnergyCalculator& energyCalc, BatteryEstimator& batteryEst,
                       RpmCalculator& rpmCalc,
                       std::function<bool()> pageButtonFn = nullptr,
                       std::function<uint8_t()> currentDayFn = nullptr);

    void begin();
    void tick();
    const AppState& state() const { return state_; }

private:
    IEnergySensor& energySensor_;
    IRpmSensor& rpmSensor_;
    IDisplay& display_;
    IStorage& storage_;
    IMqttClient& mqtt_;
    EnergyCalculator& energyCalc_;
    BatteryEstimator& batteryEst_;
    RpmCalculator& rpmCalc_;
    std::function<bool()> pageButtonFn_;
    std::function<uint8_t()> currentDayFn_;

    AppState state_;
    Page currentPage_{Page::PowerMetrics};
    uint32_t lastPersistMs_{0};
    uint8_t lastDay_{0};
};
