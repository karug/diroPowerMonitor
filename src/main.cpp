#include <Arduino.h>
#include "Logger.h"
#include "application/MonitorApplication.h"
#include "domain/services/EnergyCalculator.h"
#include "domain/services/BatteryEstimator.h"
#include "domain/services/RpmCalculator.h"

// Null stubs — replaced by real implementations in Tasks 5-11
struct NullSensor : public IEnergySensor {
    Measurement read() override {
        Measurement m;
        m.voltage = 13.8f;
        m.current = 0.5f;
        m.power = 6.9f;
        return m;
    }
};
struct NullRpmSensor : public IRpmSensor {
    Rpm read() override { return Rpm{}; }
};
struct NullStorage : public IStorage {
    void save(const EnergyStatistics&) override {}
    EnergyStatistics load() override { return {}; }
};
struct NullMqtt : public IMqttClient {
    void publish(const AppState&) override {}
};
struct NullDisplay : public IDisplay {
    void show(Page, const AppState&) override {}
};

static NullSensor nullSensor;
static NullRpmSensor nullRpm;
static NullStorage nullStorage;
static NullMqtt nullMqtt;
static NullDisplay nullDisplay;
static EnergyCalculator energyCalc;
static BatteryEstimator batteryEst;
static RpmCalculator rpmCalc(1);

static MonitorApplication* app = nullptr;
static uint32_t lastTickMs = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.1");
    app = new MonitorApplication(nullSensor, nullRpm, nullDisplay, nullStorage, nullMqtt,
                                 energyCalc, batteryEst, rpmCalc);
    app->begin();
}

void loop() {
    uint32_t now = millis();
    if (now - lastTickMs >= 1000) {
        lastTickMs = now;
        app->tick();
    }
}
