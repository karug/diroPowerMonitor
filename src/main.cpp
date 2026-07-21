#include <Arduino.h>
#include "Logger.h"
#include "infrastructure/display/Nokia5110Display.h"
#include "application/MonitorApplication.h"
#include "domain/services/EnergyCalculator.h"
#include "domain/services/BatteryEstimator.h"

static constexpr uint8_t PIN_LCD_CLK = 12;
static constexpr uint8_t PIN_LCD_DIN = 11;
static constexpr uint8_t PIN_LCD_DC = 9;
static constexpr uint8_t PIN_LCD_CE = 10;
static constexpr uint8_t PIN_LCD_RST = 14;
static constexpr uint8_t PIN_BTN_ADC = 7;  // Keyes AD Key OUT pin

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
    EnergyStatistics load() override { return EnergyStatistics{}; }
};
struct NullMqtt : public IMqttClient {
    void publish(const AppState&) override {}
};

static NullSensor nullSensor;
static NullRpmSensor nullRpm;
static NullStorage nullStorage;
static NullMqtt nullMqtt;
static Nokia5110Display display(PIN_LCD_CLK, PIN_LCD_DIN, PIN_LCD_DC, PIN_LCD_CE,
                                PIN_LCD_RST, PIN_BTN_ADC);
static EnergyCalculator energyCalc;
static BatteryEstimator batteryEst;

static MonitorApplication* app = nullptr;
static uint32_t lastTickMs = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.2");
    display.begin();
    app = new MonitorApplication(nullSensor, nullRpm, display, nullStorage, nullMqtt,
                                 energyCalc, batteryEst,
                                 [&]() { return display.buttonPressed(); });
    app->begin();
}

void loop() {
    uint32_t now = millis();
    if (now - lastTickMs >= 1000) {
        lastTickMs = now;
        app->tick();
    }
}
