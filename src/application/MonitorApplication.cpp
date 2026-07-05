#include "MonitorApplication.h"
#include "../Logger.h"
#include <Arduino.h>

MonitorApplication::MonitorApplication(
    IEnergySensor& energySensor, IRpmSensor& rpmSensor,
    IDisplay& display, IStorage& storage, IMqttClient& mqtt,
    EnergyCalculator& energyCalc, BatteryEstimator& batteryEst,
    RpmCalculator& rpmCalc, std::function<bool()> pageButtonFn,
    std::function<uint8_t()> currentDayFn)
    : energySensor_(energySensor), rpmSensor_(rpmSensor),
      display_(display), storage_(storage), mqtt_(mqtt),
      energyCalc_(energyCalc), batteryEst_(batteryEst), rpmCalc_(rpmCalc),
      pageButtonFn_(pageButtonFn), currentDayFn_(currentDayFn) {}

void MonitorApplication::begin() {
    state_.energy = storage_.load();
    if (currentDayFn_) lastDay_ = currentDayFn_();
    LOG_INFO("MonitorApplication started");
}

void MonitorApplication::tick() {
    uint32_t now = millis();

    // Page button
    if (pageButtonFn_ && pageButtonFn_()) {
        currentPage_ = static_cast<Page>((static_cast<uint8_t>(currentPage_) + 1) % 3);
    }

    // Midnight reset
    if (currentDayFn_) {
        uint8_t day = currentDayFn_();
        if (day != lastDay_) {
            energyCalc_.resetToday();
            lastDay_ = day;
        }
    }

    // Sensors
    state_.measurement = energySensor_.read();
    state_.rpm = rpmSensor_.read();

    // Domain calculations
    energyCalc_.update(state_.measurement.power, now);
    state_.energy = energyCalc_.statistics();
    state_.battery.percent = batteryEst_.estimate(state_.measurement.voltage);
    state_.battery.charging = state_.measurement.current > 0;

    // Display
    display_.show(currentPage_, state_);

    // MQTT
    mqtt_.publish(state_);

    // Persist every 10 minutes
    if (lastPersistMs_ == 0 || now - lastPersistMs_ >= 600000UL) {
        storage_.save(state_.energy);
        lastPersistMs_ = now;
    }
}
