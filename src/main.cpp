#include <Arduino.h>
#include "Logger.h"
#include "infrastructure/display/Nokia5110Display.h"
#include "infrastructure/sensors/Ina219Sensor.h"
#include "infrastructure/sensors/HallSensor.h"
#include "infrastructure/storage/PreferencesRepository.h"
#include "infrastructure/storage/LittleFsRepository.h"
#include "infrastructure/storage/HistoryRepository.h"
#include "infrastructure/network/WifiNtpManager.h"
#include "infrastructure/network/WebApiHandler.h"
#include "application/MonitorApplication.h"
#include "domain/services/EnergyCalculator.h"
#include "domain/services/BatteryEstimator.h"
#include "domain/ports/IRpmSensor.h"
#include "domain/ports/IStorage.h"
#include "domain/ports/IMqttClient.h"
#include "domain/entities/EnergyStatistics.h"

static constexpr uint8_t PIN_LCD_CLK = 12;
static constexpr uint8_t PIN_LCD_DIN = 11;
static constexpr uint8_t PIN_LCD_DC  =  9;
static constexpr uint8_t PIN_LCD_CE  = 10;
static constexpr uint8_t PIN_LCD_RST = 14;
static constexpr uint8_t PIN_BTN_ADC =  7;
static constexpr uint8_t PIN_HALL    =  6;

struct NullMqtt : public IMqttClient {
    void publish(const AppState&) override {}
};

static PreferencesRepository config;
static Ina219Sensor* inaSensor = nullptr;
static HallSensor* hallSensor = nullptr;
static Nokia5110Display display(PIN_LCD_CLK, PIN_LCD_DIN, PIN_LCD_DC,
                                PIN_LCD_CE, PIN_LCD_RST, PIN_BTN_ADC);
static LittleFsRepository lfsStorage;
static HistoryRepository historyRepo;
static NullMqtt nullMqtt;
static EnergyCalculator energyCalc;
static BatteryEstimator batteryEst;

static WifiNtpManager wifiMgr;
static WebApiHandler* webApi = nullptr;
static MonitorApplication* app = nullptr;
static uint32_t lastTickMs = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.3");

    config.begin();
    WindConfig cfg = config.loadConfig();

    inaSensor = new Ina219Sensor(cfg.shuntOhm);
    if (!inaSensor->begin()) {
        LOG_INFO("INA219 not found, using shunt=0.1");
    }

    hallSensor = new HallSensor(PIN_HALL, cfg.pulsesPerRev);
    hallSensor->begin();

    display.begin();
    lfsStorage.begin();
    historyRepo.begin();

    // WiFi provisioning via captive portal (credentials stored in NVS by WiFiManager)
    wifiMgr.begin([&]() { LOG_INFO("WiFi connected"); });

    // Web API — only start server if WiFi is up
    webApi = new WebApiHandler(
        [&]() { return app ? app->getState() : AppState{}; },
        historyRepo, config);
    if (wifiMgr.isConnected()) webApi->begin();

    app = new MonitorApplication(*inaSensor, *hallSensor, display, lfsStorage, nullMqtt,
                                 energyCalc, batteryEst,
                                 [&]() { return display.buttonPressed(); });
    app->begin();
}

void loop() {
    wifiMgr.process();
    uint32_t now = millis();
    if (now - lastTickMs >= 1000) {
        lastTickMs = now;
        app->tick();
    }
}
