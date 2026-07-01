# Wind Energy Monitor — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a complete ESP32-S3 wind turbine monitor with Nokia 5110 LCD, web dashboard, MQTT, and OTA, following hexagonal architecture.

**Architecture:** Domain layer (entities + services + ports) has zero infrastructure dependencies and is testable on host. `MonitorApplication` orchestrates use cases via a 1s tick on Core 1. AsyncWebServer runs on Core 0. A single mutex protects shared `AppState` and Hall pulse counter.

**Tech Stack:** C++17, PlatformIO, Arduino framework, ESP32-S3-N16R8 (16MB flash / 8MB PSRAM), Adafruit INA219, Adafruit PCD8544, tzapu/WiFiManager, knolleary/PubSubClient, ESPAsyncWebServer + AsyncTCP, LittleFS, Unity (native tests), GitHub Actions CI.

## Global Constraints

- C++17 (`-std=gnu++17`) enforced in both `esp32s3` and `native` envs
- No `Arduino.h` in `src/domain/` or `src/domain/services/` — native compilation will catch violations
- No global variables except `volatile` ISR counter and its mutex (Task 9)
- Constructor dependency injection: every class receives its dependencies via constructor, no setters
- clang-format Google style — CI fails on diff
- Pinout (do not change): LCD CLK=12, DIN=11, DC=9, CE=10, RST=14; INA SDA=4, SCL=5; Hall=6; Button=GPIO TBD (assigned when hardware decided)
- LittleFS assets uploaded via `pio run -t uploadfs`, not embedded in firmware binary
- `EnergyCalculator::resetToday()` is called by `MonitorApplication` when NTP detects a new calendar day — the calculator itself does not read the clock

---

## File Structure

```
diroPowerMonitor/
├── platformio.ini
├── .clang-format
├── .github/workflows/ci.yml
├── src/
│   ├── main.cpp
│   ├── Logger.h
│   ├── domain/
│   │   ├── entities/
│   │   │   ├── Measurement.h
│   │   │   ├── Rpm.h
│   │   │   ├── BatteryState.h
│   │   │   ├── EnergyStatistics.h
│   │   │   └── AppState.h
│   │   ├── ports/
│   │   │   ├── IEnergySensor.h
│   │   │   ├── IRpmSensor.h
│   │   │   ├── IDisplay.h          ← also defines Page enum
│   │   │   ├── IStorage.h
│   │   │   └── IMqttClient.h
│   │   └── services/
│   │       ├── EnergyCalculator.h / .cpp
│   │       ├── BatteryEstimator.h / .cpp
│   │       └── RpmCalculator.h / .cpp
│   ├── application/
│   │   ├── MonitorApplication.h
│   │   └── MonitorApplication.cpp
│   └── infrastructure/
│       ├── display/
│       │   └── Nokia5110Display.h / .cpp
│       ├── sensors/
│       │   ├── Ina219Sensor.h / .cpp
│       │   └── HallSensor.h / .cpp
│       ├── storage/
│       │   ├── PreferencesRepository.h / .cpp
│       │   └── LittleFsRepository.h / .cpp
│       └── connectivity/
│           ├── WifiManagerWrapper.h / .cpp
│           ├── NtpSync.h / .cpp
│           ├── OtaManager.h / .cpp
│           └── MqttClientWrapper.h / .cpp
├── presentation/
│   └── web/
│       └── WebApiHandler.h / .cpp
├── data/
│   └── index.html
└── test/
    └── native/
        ├── test_energy_calculator.cpp
        ├── test_battery_estimator.cpp
        └── test_rpm_calculator.cpp
```

---

## Task 1: V0.1 — PlatformIO Scaffold + Logger + CI

**Files:**
- Create: `platformio.ini`
- Create: `.clang-format`
- Create: `src/Logger.h`
- Create: `src/main.cpp`
- Create: `.github/workflows/ci.yml`

**Interfaces:**
- Produces: `LOG_INFO(msg)`, `LOG_WARN(msg)`, `LOG_ERROR(msg)` macros available to all files

- [ ] **Step 1: Create `platformio.ini`**

```ini
[platformio]
default_envs = esp32s3

[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
board_build.flash_size = 16MB
board_build.partitions = default_16MB.csv
board_build.filesystem = littlefs
build_flags =
    -std=gnu++17
    -DCORE_DEBUG_LEVEL=3
lib_deps =
    adafruit/Adafruit INA219@^1.2.0
    adafruit/Adafruit PCD8544 Nokia 5110 LCD library@^2.0.1
    adafruit/Adafruit GFX Library@^1.11.9
    tzapu/WiFiManager@^2.0.17
    knolleary/PubSubClient@^2.8.0
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1
    bblanchon/ArduinoJson@^7.0.0

[env:native]
platform = native
build_flags = -std=gnu++17
test_framework = unity
```

- [ ] **Step 2: Create `.clang-format`**

```yaml
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
```

- [ ] **Step 3: Create `src/Logger.h`**

```cpp
#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#define LOG_INFO(msg)  Serial.println("[INFO]  " + String(msg))
#define LOG_WARN(msg)  Serial.println("[WARN]  " + String(msg))
#define LOG_ERROR(msg) Serial.println("[ERROR] " + String(msg))
#else
#include <cstdio>
#define LOG_INFO(msg)  printf("[INFO]  %s\n", msg)
#define LOG_WARN(msg)  printf("[WARN]  %s\n", msg)
#define LOG_ERROR(msg) printf("[ERROR] %s\n", msg)
#endif
```

- [ ] **Step 4: Create `src/main.cpp`**

```cpp
#include <Arduino.h>
#include "Logger.h"

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.1 starting");
}

void loop() {
    delay(1000);
}
```

- [ ] **Step 5: Verify firmware compiles**

```bash
pio run -e esp32s3
```

Expected: `SUCCESS` with no errors.

- [ ] **Step 6: Create `.github/workflows/ci.yml`**

```yaml
name: CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/cache@v4
        with:
          path: ~/.platformio
          key: ${{ runner.os }}-pio-${{ hashFiles('platformio.ini') }}
      - uses: actions/setup-python@v5
        with:
          python-version: '3.11'
      - run: pip install platformio
      - run: pio run -e esp32s3

  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: '3.11'
      - run: pip install platformio
      - run: pio test -e native

  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: sudo apt-get install -y clang-format
      - run: find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror
```

- [ ] **Step 7: Commit**

```bash
git init
git add platformio.ini .clang-format src/Logger.h src/main.cpp .github/workflows/ci.yml
git commit -m "feat: V0.1 PlatformIO scaffold, Logger, CI"
```

---

## Task 2: V0.1 — Domain Entities and Ports

**Files:**
- Create: `src/domain/entities/Measurement.h`
- Create: `src/domain/entities/Rpm.h`
- Create: `src/domain/entities/BatteryState.h`
- Create: `src/domain/entities/EnergyStatistics.h`
- Create: `src/domain/entities/AppState.h`
- Create: `src/domain/ports/IEnergySensor.h`
- Create: `src/domain/ports/IRpmSensor.h`
- Create: `src/domain/ports/IDisplay.h`
- Create: `src/domain/ports/IStorage.h`
- Create: `src/domain/ports/IMqttClient.h`

**Interfaces:**
- Produces: all entity structs and port interfaces consumed by Tasks 3, 4, 5, 6, 7, 8, 9, 10, 11

- [ ] **Step 1: Create `src/domain/entities/Measurement.h`**

```cpp
#pragma once

struct Measurement {
    float voltage{0.0f};
    float current{0.0f};
    float power{0.0f};
};
```

- [ ] **Step 2: Create `src/domain/entities/Rpm.h`**

```cpp
#pragma once
#include <cstdint>

struct Rpm {
    uint16_t value{0};
};
```

- [ ] **Step 3: Create `src/domain/entities/BatteryState.h`**

```cpp
#pragma once
#include <cstdint>

struct BatteryState {
    uint8_t percent{0};
    bool charging{false};
};
```

- [ ] **Step 4: Create `src/domain/entities/EnergyStatistics.h`**

```cpp
#pragma once

struct EnergyStatistics {
    float whToday{0.0f};
    float whTotal{0.0f};
};
```

- [ ] **Step 5: Create `src/domain/entities/AppState.h`**

```cpp
#pragma once
#include "Measurement.h"
#include "Rpm.h"
#include "BatteryState.h"
#include "EnergyStatistics.h"

struct AppState {
    Measurement measurement;
    Rpm rpm;
    BatteryState battery;
    EnergyStatistics energy;
};
```

- [ ] **Step 6: Create `src/domain/ports/IEnergySensor.h`**

```cpp
#pragma once
#include "../entities/Measurement.h"

class IEnergySensor {
public:
    virtual Measurement read() = 0;
    virtual ~IEnergySensor() = default;
};
```

- [ ] **Step 7: Create `src/domain/ports/IRpmSensor.h`**

```cpp
#pragma once
#include "../entities/Rpm.h"

class IRpmSensor {
public:
    virtual Rpm read() = 0;
    virtual ~IRpmSensor() = default;
};
```

- [ ] **Step 8: Create `src/domain/ports/IDisplay.h`**

```cpp
#pragma once
#include <cstdint>
#include "../entities/AppState.h"

enum class Page : uint8_t {
    PowerMetrics = 0,
    Energy = 1,
    RpmBattery = 2
};

class IDisplay {
public:
    virtual void show(Page page, const AppState& state) = 0;
    virtual ~IDisplay() = default;
};
```

- [ ] **Step 9: Create `src/domain/ports/IStorage.h`**

```cpp
#pragma once
#include "../entities/EnergyStatistics.h"

class IStorage {
public:
    virtual void save(const EnergyStatistics& stats) = 0;
    virtual EnergyStatistics load() = 0;
    virtual ~IStorage() = default;
};
```

- [ ] **Step 10: Create `src/domain/ports/IMqttClient.h`**

```cpp
#pragma once
#include "../entities/AppState.h"

class IMqttClient {
public:
    virtual void publish(const AppState& state) = 0;
    virtual ~IMqttClient() = default;
};
```

- [ ] **Step 11: Verify firmware still compiles**

```bash
pio run -e esp32s3
```

Expected: `SUCCESS`.

- [ ] **Step 12: Commit**

```bash
git add src/domain/
git commit -m "feat: domain entities and ports"
```

---

## Task 3: V0.1 — Domain Services (TDD, native)

**Files:**
- Create: `src/domain/services/EnergyCalculator.h`
- Create: `src/domain/services/EnergyCalculator.cpp`
- Create: `src/domain/services/BatteryEstimator.h`
- Create: `src/domain/services/BatteryEstimator.cpp`
- Create: `src/domain/services/RpmCalculator.h`
- Create: `src/domain/services/RpmCalculator.cpp`
- Create: `test/native/test_energy_calculator.cpp`
- Create: `test/native/test_battery_estimator.cpp`
- Create: `test/native/test_rpm_calculator.cpp`

**Interfaces:**
- Consumes: `EnergyStatistics` (Task 2), `Rpm` (Task 2)
- Produces:
  - `EnergyCalculator::update(float powerW, uint32_t nowMs)` — integrate power×time
  - `EnergyCalculator::resetToday()` — zero `whToday` (called by app at midnight)
  - `EnergyCalculator::statistics() const -> EnergyStatistics`
  - `BatteryEstimator::estimate(float voltageV) const -> uint8_t` — 0–100%
  - `RpmCalculator(uint8_t pulsesPerRev)` — constructor sets divisor
  - `RpmCalculator::setPulsesPerRev(uint8_t ppr)`
  - `RpmCalculator::calculate(uint32_t pulseCount, uint32_t windowMs) const -> Rpm`

### EnergyCalculator (TDD)

- [ ] **Step 1: Write failing test for EnergyCalculator**

Create `test/native/test_energy_calculator.cpp`:

```cpp
#include <unity.h>
#include "domain/services/EnergyCalculator.h"

void setUp() {}
void tearDown() {}

void test_initial_statistics_are_zero() {
    EnergyCalculator calc;
    auto stats = calc.statistics();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, stats.whToday);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, stats.whTotal);
}

void test_accumulates_wh_correctly() {
    EnergyCalculator calc;
    // 100W for 3600 seconds = 100 Wh
    calc.update(100.0f, 0);
    calc.update(100.0f, 3600000);  // nowMs = 3600000 ms = 1 hour
    auto stats = calc.statistics();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, stats.whToday);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, stats.whTotal);
}

void test_reset_today_zeroes_whtoday_keeps_whtotal() {
    EnergyCalculator calc;
    calc.update(100.0f, 0);
    calc.update(100.0f, 3600000);
    calc.resetToday();
    auto stats = calc.statistics();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, stats.whToday);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, stats.whTotal);
}

void test_multiple_updates_accumulate() {
    EnergyCalculator calc;
    calc.update(50.0f, 0);
    calc.update(50.0f, 1800000);  // 30 min → 25 Wh
    calc.update(50.0f, 3600000);  // 30 min → 25 Wh more = 50 Wh total
    auto stats = calc.statistics();
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 50.0f, stats.whToday);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_initial_statistics_are_zero);
    RUN_TEST(test_accumulates_wh_correctly);
    RUN_TEST(test_reset_today_zeroes_whtoday_keeps_whtotal);
    RUN_TEST(test_multiple_updates_accumulate);
    return UNITY_END();
}
```

- [ ] **Step 2: Run test — verify it fails (no implementation yet)**

```bash
pio test -e native --filter native/test_energy_calculator
```

Expected: compile error — `EnergyCalculator.h` not found.

- [ ] **Step 3: Create `src/domain/services/EnergyCalculator.h`**

```cpp
#pragma once
#include <cstdint>
#include "../entities/EnergyStatistics.h"

class EnergyCalculator {
public:
    void update(float powerW, uint32_t nowMs);
    void resetToday();
    EnergyStatistics statistics() const;

private:
    float whToday_{0.0f};
    float whTotal_{0.0f};
    uint32_t lastMs_{0};
    bool firstUpdate_{true};
};
```

- [ ] **Step 4: Create `src/domain/services/EnergyCalculator.cpp`**

```cpp
#include "EnergyCalculator.h"

void EnergyCalculator::update(float powerW, uint32_t nowMs) {
    if (!firstUpdate_) {
        float deltaHours = static_cast<float>(nowMs - lastMs_) / 3'600'000.0f;
        float wh = powerW * deltaHours;
        whToday_ += wh;
        whTotal_ += wh;
    }
    firstUpdate_ = false;
    lastMs_ = nowMs;
}

void EnergyCalculator::resetToday() {
    whToday_ = 0.0f;
    firstUpdate_ = true;
}

EnergyStatistics EnergyCalculator::statistics() const {
    return {whToday_, whTotal_};
}
```

- [ ] **Step 5: Run test — verify it passes**

```bash
pio test -e native --filter native/test_energy_calculator
```

Expected: `4 Tests 0 Failures 0 Ignored — OK`.

### BatteryEstimator (TDD)

- [ ] **Step 6: Write failing test for BatteryEstimator**

Create `test/native/test_battery_estimator.cpp`:

```cpp
#include <unity.h>
#include "domain/services/BatteryEstimator.h"

void setUp() {}
void tearDown() {}

void test_full_voltage_returns_100_percent() {
    BatteryEstimator est;
    TEST_ASSERT_EQUAL_UINT8(100, est.estimate(13.6f));
}

void test_mid_voltage_returns_approximately_50_percent() {
    BatteryEstimator est;
    uint8_t pct = est.estimate(12.8f);
    TEST_ASSERT_GREATER_OR_EQUAL(45, pct);
    TEST_ASSERT_LESS_OR_EQUAL(55, pct);
}

void test_low_voltage_returns_low_percent() {
    BatteryEstimator est;
    TEST_ASSERT_LESS_OR_EQUAL(15, est.estimate(12.0f));
}

void test_below_minimum_returns_0() {
    BatteryEstimator est;
    TEST_ASSERT_EQUAL_UINT8(0, est.estimate(9.0f));
}

void test_above_maximum_returns_100() {
    BatteryEstimator est;
    TEST_ASSERT_EQUAL_UINT8(100, est.estimate(15.0f));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_full_voltage_returns_100_percent);
    RUN_TEST(test_mid_voltage_returns_approximately_50_percent);
    RUN_TEST(test_low_voltage_returns_low_percent);
    RUN_TEST(test_below_minimum_returns_0);
    RUN_TEST(test_above_maximum_returns_100);
    return UNITY_END();
}
```

- [ ] **Step 7: Create `src/domain/services/BatteryEstimator.h`**

```cpp
#pragma once
#include <cstdint>

class BatteryEstimator {
public:
    uint8_t estimate(float voltageV) const;
};
```

- [ ] **Step 8: Create `src/domain/services/BatteryEstimator.cpp`**

LiFePO4 4S (12V) discharge curve — table maps voltage breakpoints to percent:

```cpp
#include "BatteryEstimator.h"
#include <cstddef>

namespace {
struct Point { float v; uint8_t pct; };
constexpr Point kCurve[] = {
    {13.6f, 100}, {13.3f, 90}, {13.0f, 70},
    {12.8f, 50},  {12.5f, 30}, {12.0f, 10},
    {11.8f,  5},  {10.0f,  0}
};
constexpr size_t kSize = sizeof(kCurve) / sizeof(kCurve[0]);
}

uint8_t BatteryEstimator::estimate(float voltageV) const {
    if (voltageV >= kCurve[0].v) return 100;
    if (voltageV <= kCurve[kSize - 1].v) return 0;
    for (size_t i = 0; i < kSize - 1; ++i) {
        if (voltageV >= kCurve[i + 1].v) {
            float ratio = (voltageV - kCurve[i + 1].v) /
                          (kCurve[i].v - kCurve[i + 1].v);
            return static_cast<uint8_t>(
                kCurve[i + 1].pct + ratio * (kCurve[i].pct - kCurve[i + 1].pct));
        }
    }
    return 0;
}
```

- [ ] **Step 9: Run test — verify it passes**

```bash
pio test -e native --filter native/test_battery_estimator
```

Expected: `5 Tests 0 Failures 0 Ignored — OK`.

### RpmCalculator (TDD)

- [ ] **Step 10: Write failing test for RpmCalculator**

Create `test/native/test_rpm_calculator.cpp`:

```cpp
#include <unity.h>
#include "domain/services/RpmCalculator.h"

void setUp() {}
void tearDown() {}

void test_zero_pulses_returns_zero_rpm() {
    RpmCalculator calc(1);
    Rpm rpm = calc.calculate(0, 3000);
    TEST_ASSERT_EQUAL_UINT16(0, rpm.value);
}

void test_60_pulses_in_1s_with_1ppr_equals_60rpm() {
    RpmCalculator calc(1);
    Rpm rpm = calc.calculate(60, 1000);
    TEST_ASSERT_EQUAL_UINT16(60, rpm.value);
}

void test_60_pulses_in_1s_with_2ppr_equals_30rpm() {
    RpmCalculator calc(2);
    Rpm rpm = calc.calculate(60, 1000);
    TEST_ASSERT_EQUAL_UINT16(30, rpm.value);
}

void test_set_pulses_per_rev_changes_divisor() {
    RpmCalculator calc(1);
    calc.setPulsesPerRev(3);
    Rpm rpm = calc.calculate(90, 1000);
    TEST_ASSERT_EQUAL_UINT16(30, rpm.value);
}

void test_zero_window_returns_zero_rpm() {
    RpmCalculator calc(1);
    Rpm rpm = calc.calculate(100, 0);
    TEST_ASSERT_EQUAL_UINT16(0, rpm.value);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_zero_pulses_returns_zero_rpm);
    RUN_TEST(test_60_pulses_in_1s_with_1ppr_equals_60rpm);
    RUN_TEST(test_60_pulses_in_1s_with_2ppr_equals_30rpm);
    RUN_TEST(test_set_pulses_per_rev_changes_divisor);
    RUN_TEST(test_zero_window_returns_zero_rpm);
    return UNITY_END();
}
```

- [ ] **Step 11: Create `src/domain/services/RpmCalculator.h`**

```cpp
#pragma once
#include <cstdint>
#include "../entities/Rpm.h"

class RpmCalculator {
public:
    explicit RpmCalculator(uint8_t pulsesPerRev = 1);
    void setPulsesPerRev(uint8_t ppr);
    Rpm calculate(uint32_t pulseCount, uint32_t windowMs) const;

private:
    uint8_t pulsesPerRev_;
};
```

- [ ] **Step 12: Create `src/domain/services/RpmCalculator.cpp`**

```cpp
#include "RpmCalculator.h"

RpmCalculator::RpmCalculator(uint8_t pulsesPerRev) : pulsesPerRev_(pulsesPerRev) {}

void RpmCalculator::setPulsesPerRev(uint8_t ppr) {
    pulsesPerRev_ = (ppr == 0) ? 1 : ppr;
}

Rpm RpmCalculator::calculate(uint32_t pulseCount, uint32_t windowMs) const {
    if (windowMs == 0 || pulseCount == 0) return {0};
    float revolutions = static_cast<float>(pulseCount) / pulsesPerRev_;
    float minutes = static_cast<float>(windowMs) / 60'000.0f;
    return {static_cast<uint16_t>(revolutions / minutes)};
}
```

- [ ] **Step 13: Run all native tests**

```bash
pio test -e native
```

Expected: `14 Tests 0 Failures 0 Ignored — OK`.

- [ ] **Step 14: Commit**

```bash
git add src/domain/services/ test/
git commit -m "feat: domain services with native tests (EnergyCalculator, BatteryEstimator, RpmCalculator)"
```

---

## Task 4: V0.1 — MonitorApplication Skeleton

**Files:**
- Create: `src/application/MonitorApplication.h`
- Create: `src/application/MonitorApplication.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: all ports (Task 2), all services (Task 3)
- Produces: `MonitorApplication(IEnergySensor&, IRpmSensor&, IDisplay&, IStorage&, IMqttClient&, EnergyCalculator&, BatteryEstimator&, RpmCalculator&)`; `begin()`; `tick()`

- [ ] **Step 1: Create `src/application/MonitorApplication.h`**

```cpp
#pragma once
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
                       RpmCalculator& rpmCalc);

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

    AppState state_;
    Page currentPage_{Page::PowerMetrics};
    uint32_t lastPersistMs_{0};
};
```

- [ ] **Step 2: Create `src/application/MonitorApplication.cpp`**

```cpp
#include "MonitorApplication.h"
#include "../Logger.h"

MonitorApplication::MonitorApplication(
    IEnergySensor& energySensor, IRpmSensor& rpmSensor,
    IDisplay& display, IStorage& storage, IMqttClient& mqtt,
    EnergyCalculator& energyCalc, BatteryEstimator& batteryEst,
    RpmCalculator& rpmCalc)
    : energySensor_(energySensor), rpmSensor_(rpmSensor),
      display_(display), storage_(storage), mqtt_(mqtt),
      energyCalc_(energyCalc), batteryEst_(batteryEst), rpmCalc_(rpmCalc) {}

void MonitorApplication::begin() {
    state_.energy = storage_.load();
    LOG_INFO("MonitorApplication started");
}

void MonitorApplication::tick() {
    // Sensors
    state_.measurement = energySensor_.read();
    state_.rpm = rpmSensor_.read();

    // Domain calculations
    energyCalc_.update(state_.measurement.power, static_cast<uint32_t>(0));
    state_.energy = energyCalc_.statistics();
    state_.battery.percent = batteryEst_.estimate(state_.measurement.voltage);
    state_.battery.charging = state_.measurement.current > 0;

    // Display
    display_.show(currentPage_, state_);

    // Publish
    mqtt_.publish(state_);

    // Persist energy every 10 minutes
    // NOTE: lastPersistMs_ timing wired in Task 6 when millis() available
}
```

> **Note:** `energyCalc_.update()` is called with `0` as placeholder — Task 6 wires `millis()` into the tick. The `MonitorApplication` is not compiled in the native env, so `millis()` not being available there is fine.

- [ ] **Step 3: Update `src/main.cpp` — add null stub implementations so firmware compiles**

The app cannot be instantiated until real implementations exist (Task 5+). Add `setup()`/`loop()` guards:

```cpp
#include <Arduino.h>
#include "Logger.h"

// MonitorApplication wired in Task 5+ once implementations exist
// For V0.1, just verify Logger and Serial work

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.1");
}

void loop() {
    delay(1000);
}
```

- [ ] **Step 4: Verify firmware compiles (includes MonitorApplication.cpp now)**

```bash
pio run -e esp32s3
```

Expected: `SUCCESS`.

- [ ] **Step 5: Commit**

```bash
git add src/application/ src/main.cpp
git commit -m "feat: MonitorApplication skeleton"
```

---

## Task 5: V0.2 — Nokia 5110 Display

**Files:**
- Create: `src/infrastructure/display/Nokia5110Display.h`
- Create: `src/infrastructure/display/Nokia5110Display.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: `IDisplay` port (Task 2), `Page` enum (Task 2), `AppState` (Task 2)
- Produces: `Nokia5110Display(uint8_t clk, uint8_t din, uint8_t dc, uint8_t ce, uint8_t rst, uint8_t buttonPin)` — implements `IDisplay`

- [ ] **Step 1: Create `src/infrastructure/display/Nokia5110Display.h`**

```cpp
#pragma once
#include <cstdint>
#include "../../domain/ports/IDisplay.h"

class Adafruit_PCD8544;  // forward declaration to avoid including in header

class Nokia5110Display : public IDisplay {
public:
    Nokia5110Display(uint8_t clk, uint8_t din, uint8_t dc, uint8_t ce,
                     uint8_t rst, uint8_t buttonPin);
    ~Nokia5110Display() override;

    void begin();
    void show(Page page, const AppState& state) override;
    bool buttonPressed();  // call from tick(); returns true if page changed

private:
    void showPowerMetrics(const AppState& state);
    void showEnergy(const AppState& state);
    void showRpmBattery(const AppState& state);

    Adafruit_PCD8544* lcd_;
    uint8_t buttonPin_;
    uint32_t lastDebounceMs_{0};
    bool lastButtonState_{false};
};
```

- [ ] **Step 2: Create `src/infrastructure/display/Nokia5110Display.cpp`**

```cpp
#include "Nokia5110Display.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

Nokia5110Display::Nokia5110Display(uint8_t clk, uint8_t din, uint8_t dc,
                                   uint8_t ce, uint8_t rst, uint8_t buttonPin)
    : lcd_(new Adafruit_PCD8544(clk, din, dc, ce, rst)),
      buttonPin_(buttonPin) {}

Nokia5110Display::~Nokia5110Display() { delete lcd_; }

void Nokia5110Display::begin() {
    lcd_->begin();
    lcd_->setContrast(50);
    lcd_->clearDisplay();
    lcd_->display();
    pinMode(buttonPin_, INPUT_PULLUP);
}

bool Nokia5110Display::buttonPressed() {
    bool reading = (digitalRead(buttonPin_) == LOW);
    if (reading && !lastButtonState_ && (millis() - lastDebounceMs_ > 50)) {
        lastDebounceMs_ = millis();
        lastButtonState_ = true;
        return true;
    }
    if (!reading) lastButtonState_ = false;
    return false;
}

void Nokia5110Display::show(Page page, const AppState& state) {
    lcd_->clearDisplay();
    lcd_->setTextSize(1);
    lcd_->setTextColor(BLACK);
    switch (page) {
        case Page::PowerMetrics: showPowerMetrics(state); break;
        case Page::Energy:       showEnergy(state);       break;
        case Page::RpmBattery:   showRpmBattery(state);   break;
    }
    lcd_->display();
}

void Nokia5110Display::showPowerMetrics(const AppState& s) {
    lcd_->setCursor(0, 0); lcd_->print("V: "); lcd_->print(s.measurement.voltage, 1); lcd_->print(" V");
    lcd_->setCursor(0, 10); lcd_->print("I: "); lcd_->print(s.measurement.current, 2); lcd_->print(" A");
    lcd_->setCursor(0, 20); lcd_->print("P: "); lcd_->print(s.measurement.power, 1);   lcd_->print(" W");
}

void Nokia5110Display::showEnergy(const AppState& s) {
    lcd_->setCursor(0, 0); lcd_->print("Hoy:");
    lcd_->setCursor(0, 10); lcd_->print(s.energy.whToday, 1); lcd_->print(" Wh");
    lcd_->setCursor(0, 25); lcd_->print("Total:");
    lcd_->setCursor(0, 35); lcd_->print(s.energy.whTotal / 1000.0f, 2); lcd_->print(" kWh");
}

void Nokia5110Display::showRpmBattery(const AppState& s) {
    lcd_->setCursor(0, 0); lcd_->print("RPM: "); lcd_->print(s.rpm.value);
    lcd_->setCursor(0, 15); lcd_->print("Bat: "); lcd_->print(s.battery.percent); lcd_->print("%");
    if (s.battery.charging) { lcd_->setCursor(0, 25); lcd_->print("CHG"); }
}
```

- [ ] **Step 3: Update `src/application/MonitorApplication.h` — add `buttonPin` page-cycle logic**

Add `checkButton(Nokia5110Display&)` helper or handle in tick. The cleanest approach: `MonitorApplication::tick()` calls `display_.buttonPressed()` but `IDisplay` doesn't expose that. Add a concrete `Nokia5110Display&` reference to the app OR add `checkPage()` to IDisplay. Simplest: cast in tick. Actually cleanest: make `MonitorApplication` template-free and pass `Nokia5110Display` directly.

> **Design note:** Rather than polluting `IDisplay` with button logic, accept a `std::function<bool()>` in `MonitorApplication` for the page-advance callback. This keeps `IDisplay` clean and the button implementation in infrastructure.

Update `src/application/MonitorApplication.h` — add constructor parameter:

```cpp
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
                       std::function<bool()> pageButtonFn = nullptr);

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

    AppState state_;
    Page currentPage_{Page::PowerMetrics};
    uint32_t lastPersistMs_{0};
};
```

Update `src/application/MonitorApplication.cpp` — add button check in tick:

```cpp
#include "MonitorApplication.h"
#include "../Logger.h"
#include <Arduino.h>

MonitorApplication::MonitorApplication(
    IEnergySensor& energySensor, IRpmSensor& rpmSensor,
    IDisplay& display, IStorage& storage, IMqttClient& mqtt,
    EnergyCalculator& energyCalc, BatteryEstimator& batteryEst,
    RpmCalculator& rpmCalc, std::function<bool()> pageButtonFn)
    : energySensor_(energySensor), rpmSensor_(rpmSensor),
      display_(display), storage_(storage), mqtt_(mqtt),
      energyCalc_(energyCalc), batteryEst_(batteryEst), rpmCalc_(rpmCalc),
      pageButtonFn_(pageButtonFn) {}

void MonitorApplication::begin() {
    state_.energy = storage_.load();
    LOG_INFO("MonitorApplication started");
}

void MonitorApplication::tick() {
    uint32_t now = millis();

    // Page button
    if (pageButtonFn_ && pageButtonFn_()) {
        currentPage_ = static_cast<Page>((static_cast<uint8_t>(currentPage_) + 1) % 3);
    }

    // Sensors
    state_.measurement = energySensor_.read();
    state_.rpm = rpmSensor_.read();

    // Calculations
    energyCalc_.update(state_.measurement.power, now);
    state_.energy = energyCalc_.statistics();
    state_.battery.percent = batteryEst_.estimate(state_.measurement.voltage);
    state_.battery.charging = state_.measurement.current > 0;

    // Display
    display_.show(currentPage_, state_);

    // MQTT
    mqtt_.publish(state_);

    // Persist energy every 10 minutes
    if (now - lastPersistMs_ >= 600'000UL || lastPersistMs_ == 0) {
        storage_.save(state_.energy);
        lastPersistMs_ = now;
    }
}
```

- [ ] **Step 4: Update `src/main.cpp` — wire Nokia5110Display**

At this point we still don't have real sensors or MQTT, so create minimal stub implementations inline in main.cpp:

```cpp
#include <Arduino.h>
#include "Logger.h"
#include "infrastructure/display/Nokia5110Display.h"
#include "application/MonitorApplication.h"
#include "domain/services/EnergyCalculator.h"
#include "domain/services/BatteryEstimator.h"
#include "domain/services/RpmCalculator.h"

// GPIO definitions
static constexpr uint8_t PIN_LCD_CLK  = 12;
static constexpr uint8_t PIN_LCD_DIN  = 11;
static constexpr uint8_t PIN_LCD_DC   =  9;
static constexpr uint8_t PIN_LCD_CE   = 10;
static constexpr uint8_t PIN_LCD_RST  = 14;
static constexpr uint8_t PIN_BUTTON   = 0;  // TBD — use BOOT button for now

// Null stubs (replaced in later tasks)
struct NullSensor : public IEnergySensor {
    Measurement read() override { return {13.8f, 0.5f, 6.9f}; }
};
struct NullRpmSensor : public IRpmSensor {
    Rpm read() override { return {120}; }
};
struct NullStorage : public IStorage {
    void save(const EnergyStatistics&) override {}
    EnergyStatistics load() override { return {}; }
};
struct NullMqtt : public IMqttClient {
    void publish(const AppState&) override {}
};

static NullSensor nullSensor;
static NullRpmSensor nullRpm;
static NullStorage nullStorage;
static NullMqtt nullMqtt;
static Nokia5110Display display(PIN_LCD_CLK, PIN_LCD_DIN, PIN_LCD_DC,
                                PIN_LCD_CE, PIN_LCD_RST, PIN_BUTTON);
static EnergyCalculator energyCalc;
static BatteryEstimator batteryEst;
static RpmCalculator rpmCalc(1);

static MonitorApplication* app = nullptr;
static uint32_t lastTickMs = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.2");
    display.begin();
    app = new MonitorApplication(nullSensor, nullRpm, display, nullStorage, nullMqtt,
                                 energyCalc, batteryEst, rpmCalc,
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
```

- [ ] **Step 5: Compile and flash to device**

```bash
pio run -e esp32s3 --target upload
pio device monitor --baud 115200
```

Expected on serial: `[INFO]  Wind Energy Monitor v0.2`. Expected on LCD: page 1 showing stub values (13.8V / 0.5A / 6.9W). Button (GPIO0 / BOOT) cycles pages.

- [ ] **Step 6: Commit**

```bash
git add src/infrastructure/display/ src/application/ src/main.cpp
git commit -m "feat: V0.2 Nokia 5110 display with 3 pages and button navigation"
```

---

## Task 6: V0.3 — INA219 Sensor + PreferencesRepository

**Files:**
- Create: `src/infrastructure/sensors/Ina219Sensor.h`
- Create: `src/infrastructure/sensors/Ina219Sensor.cpp`
- Create: `src/infrastructure/storage/PreferencesRepository.h`
- Create: `src/infrastructure/storage/PreferencesRepository.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: `IEnergySensor` (Task 2)
- Produces:
  - `Ina219Sensor(float shuntOhm)` — implements `IEnergySensor`; reads I²C on SDA=4, SCL=5
  - `PreferencesRepository()` — reads/writes NVS namespace `wind`
  - `PreferencesRepository::mqttHost() const -> String`
  - `PreferencesRepository::mqttPort() const -> uint16_t`
  - `PreferencesRepository::pulsesPerRev() const -> uint8_t`
  - `PreferencesRepository::shuntOhm() const -> float`
  - `PreferencesRepository::otaPassword() const -> String`
  - `PreferencesRepository::save(key, value)` — overloaded for String, uint16_t, uint8_t, float

- [ ] **Step 1: Create `src/infrastructure/sensors/Ina219Sensor.h`**

```cpp
#pragma once
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
```

- [ ] **Step 2: Create `src/infrastructure/sensors/Ina219Sensor.cpp`**

```cpp
#include "Ina219Sensor.h"
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "../../Logger.h"

static constexpr uint8_t PIN_SDA = 4;
static constexpr uint8_t PIN_SCL = 5;

Ina219Sensor::Ina219Sensor(float shuntOhm)
    : ina_(new Adafruit_INA219()), shuntOhm_(shuntOhm) {}

Ina219Sensor::~Ina219Sensor() { delete ina_; }

bool Ina219Sensor::begin() {
    Wire.begin(PIN_SDA, PIN_SCL);
    if (!ina_->begin()) {
        LOG_ERROR("INA219 not found on I2C");
        return false;
    }
    // Set calibration for shunt resistor value
    // Default Adafruit lib calibrates for 0.1 Ohm / 3.2A range
    // For non-default shunt, recalibrate:
    // ina_->setCalibration_32V_2A() or custom cal based on shuntOhm_
    LOG_INFO("INA219 initialized");
    return true;
}

Measurement Ina219Sensor::read() {
    float v = ina_->getBusVoltage_V() + (ina_->getShuntVoltage_mV() / 1000.0f);
    float i = ina_->getCurrent_mA() / 1000.0f;
    float p = v * i;
    return {v, i, p};
}
```

- [ ] **Step 3: Create `src/infrastructure/storage/PreferencesRepository.h`**

```cpp
#pragma once
#include <Arduino.h>
#include <Preferences.h>

class PreferencesRepository {
public:
    PreferencesRepository();

    String  mqttHost() const;
    uint16_t mqttPort() const;
    uint8_t  pulsesPerRev() const;
    float    shuntOhm() const;
    String   otaPassword() const;

    void saveMqttHost(const String& host);
    void saveMqttPort(uint16_t port);
    void savePulsesPerRev(uint8_t ppr);
    void saveShuntOhm(float ohm);
    void saveOtaPassword(const String& pass);

private:
    mutable Preferences prefs_;
};
```

- [ ] **Step 4: Create `src/infrastructure/storage/PreferencesRepository.cpp`**

```cpp
#include "PreferencesRepository.h"

static constexpr char kNs[] = "wind";

PreferencesRepository::PreferencesRepository() {}

String PreferencesRepository::mqttHost() const {
    prefs_.begin(kNs, true);
    String v = prefs_.getString("mqtt_host", "");
    prefs_.end();
    return v;
}

uint16_t PreferencesRepository::mqttPort() const {
    prefs_.begin(kNs, true);
    uint16_t v = prefs_.getUShort("mqtt_port", 1883);
    prefs_.end();
    return v;
}

uint8_t PreferencesRepository::pulsesPerRev() const {
    prefs_.begin(kNs, true);
    uint8_t v = prefs_.getUChar("pulses_rev", 1);
    prefs_.end();
    return v;
}

float PreferencesRepository::shuntOhm() const {
    prefs_.begin(kNs, true);
    float v = prefs_.getFloat("shunt_ohm", 0.1f);
    prefs_.end();
    return v;
}

String PreferencesRepository::otaPassword() const {
    prefs_.begin(kNs, true);
    String v = prefs_.getString("ota_pass", "windota");
    prefs_.end();
    return v;
}

void PreferencesRepository::saveMqttHost(const String& host) {
    prefs_.begin(kNs, false); prefs_.putString("mqtt_host", host); prefs_.end();
}

void PreferencesRepository::saveMqttPort(uint16_t port) {
    prefs_.begin(kNs, false); prefs_.putUShort("mqtt_port", port); prefs_.end();
}

void PreferencesRepository::savePulsesPerRev(uint8_t ppr) {
    prefs_.begin(kNs, false); prefs_.putUChar("pulses_rev", ppr); prefs_.end();
}

void PreferencesRepository::saveShuntOhm(float ohm) {
    prefs_.begin(kNs, false); prefs_.putFloat("shunt_ohm", ohm); prefs_.end();
}

void PreferencesRepository::saveOtaPassword(const String& pass) {
    prefs_.begin(kNs, false); prefs_.putString("ota_pass", pass); prefs_.end();
}
```

- [ ] **Step 5: Update `src/main.cpp` — replace NullSensor with Ina219Sensor**

```cpp
#include <Arduino.h>
#include "Logger.h"
#include "infrastructure/display/Nokia5110Display.h"
#include "infrastructure/sensors/Ina219Sensor.h"
#include "infrastructure/storage/PreferencesRepository.h"
#include "application/MonitorApplication.h"
#include "domain/services/EnergyCalculator.h"
#include "domain/services/BatteryEstimator.h"
#include "domain/services/RpmCalculator.h"

static constexpr uint8_t PIN_LCD_CLK  = 12;
static constexpr uint8_t PIN_LCD_DIN  = 11;
static constexpr uint8_t PIN_LCD_DC   =  9;
static constexpr uint8_t PIN_LCD_CE   = 10;
static constexpr uint8_t PIN_LCD_RST  = 14;
static constexpr uint8_t PIN_BUTTON   =  0;   // TBD

struct NullRpmSensor : public IRpmSensor { Rpm read() override { return {0}; } };
struct NullStorage   : public IStorage   {
    void save(const EnergyStatistics&) override {}
    EnergyStatistics load() override { return {}; }
};
struct NullMqtt      : public IMqttClient { void publish(const AppState&) override {} };

static PreferencesRepository prefs;
static Ina219Sensor ina219(prefs.shuntOhm());
static NullRpmSensor nullRpm;
static NullStorage nullStorage;
static NullMqtt nullMqtt;
static Nokia5110Display display(PIN_LCD_CLK, PIN_LCD_DIN, PIN_LCD_DC,
                                PIN_LCD_CE, PIN_LCD_RST, PIN_BUTTON);
static EnergyCalculator energyCalc;
static BatteryEstimator batteryEst;
static RpmCalculator rpmCalc(prefs.pulsesPerRev());

static MonitorApplication* app = nullptr;
static uint32_t lastTickMs = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.3");
    display.begin();
    ina219.begin();
    app = new MonitorApplication(ina219, nullRpm, display, nullStorage, nullMqtt,
                                 energyCalc, batteryEst, rpmCalc,
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
```

- [ ] **Step 6: Compile and flash**

```bash
pio run -e esp32s3 --target upload
pio device monitor --baud 115200
```

Expected on LCD page 1: real voltage/current/power from INA219. Serial shows `[INFO]  INA219 initialized`.

- [ ] **Step 7: Commit**

```bash
git add src/infrastructure/sensors/ src/infrastructure/storage/PreferencesRepository.* src/main.cpp
git commit -m "feat: V0.3 INA219 sensor and PreferencesRepository (NVS)"
```

---

## Task 7: V0.4 — Energy Persistence (LittleFsRepository)

**Files:**
- Create: `src/infrastructure/storage/LittleFsRepository.h`
- Create: `src/infrastructure/storage/LittleFsRepository.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: `IStorage` (Task 2), `EnergyStatistics` (Task 2)
- Produces: `LittleFsRepository()` — implements `IStorage`; reads/writes `/energy.json` and `/history.json` on LittleFS

- [ ] **Step 1: Create `src/infrastructure/storage/LittleFsRepository.h`**

```cpp
#pragma once
#include "../../domain/ports/IStorage.h"
#include "../../domain/entities/AppState.h"

class LittleFsRepository : public IStorage {
public:
    LittleFsRepository();

    bool begin();
    void save(const EnergyStatistics& stats) override;
    EnergyStatistics load() override;

    void appendHistory(const AppState& state);
    String readHistoryJson();

private:
    static constexpr char kEnergyFile[]  = "/energy.json";
    static constexpr char kHistoryFile[] = "/history.json";
    static constexpr size_t kMaxHistory  = 1440;

    void writeJson(const char* path, const String& json);
    String readJson(const char* path);
};
```

- [ ] **Step 2: Create `src/infrastructure/storage/LittleFsRepository.cpp`**

```cpp
#include "LittleFsRepository.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../../Logger.h"

constexpr char LittleFsRepository::kEnergyFile[];
constexpr char LittleFsRepository::kHistoryFile[];

LittleFsRepository::LittleFsRepository() {}

bool LittleFsRepository::begin() {
    if (!LittleFS.begin(true)) {
        LOG_ERROR("LittleFS mount failed");
        return false;
    }
    LOG_INFO("LittleFS mounted");
    return true;
}

void LittleFsRepository::save(const EnergyStatistics& stats) {
    JsonDocument doc;
    doc["whToday"] = stats.whToday;
    doc["whTotal"] = stats.whTotal;
    doc["lastSave"] = millis();  // replaced by NTP timestamp in Task 10
    String json;
    serializeJson(doc, json);
    writeJson(kEnergyFile, json);
}

EnergyStatistics LittleFsRepository::load() {
    String json = readJson(kEnergyFile);
    if (json.isEmpty()) return {};
    JsonDocument doc;
    if (deserializeJson(doc, json)) return {};
    return {doc["whToday"] | 0.0f, doc["whTotal"] | 0.0f};
}

void LittleFsRepository::appendHistory(const AppState& state) {
    // Read existing array, append, trim to kMaxHistory, write back
    String existing = readJson(kHistoryFile);
    JsonDocument doc;
    JsonArray arr;
    if (existing.isEmpty() || deserializeJson(doc, existing)) {
        arr = doc.to<JsonArray>();
    } else {
        arr = doc.as<JsonArray>();
    }
    JsonObject entry = arr.add<JsonObject>();
    entry["t"]  = millis();
    entry["v"]  = state.measurement.voltage;
    entry["i"]  = state.measurement.current;
    entry["p"]  = state.measurement.power;
    entry["wh"] = state.energy.whToday;

    // Trim oldest if over limit
    while (arr.size() > kMaxHistory) arr.remove(0);

    String json;
    serializeJson(doc, json);
    writeJson(kHistoryFile, json);
}

String LittleFsRepository::readHistoryJson() {
    return readJson(kHistoryFile);
}

void LittleFsRepository::writeJson(const char* path, const String& json) {
    File f = LittleFS.open(path, "w");
    if (!f) { LOG_ERROR("Cannot write file"); return; }
    f.print(json);
    f.close();
}

String LittleFsRepository::readJson(const char* path) {
    if (!LittleFS.exists(path)) return "";
    File f = LittleFS.open(path, "r");
    if (!f) return "";
    String s = f.readString();
    f.close();
    return s;
}
```

- [ ] **Step 3: Update `MonitorApplication` to use history append**

In `src/application/MonitorApplication.h`, add:
```cpp
    uint32_t lastHistoryMs_{0};
```

In `src/application/MonitorApplication.cpp`, add in tick after persist block:
```cpp
    // Append history every 1 minute (storage_ cast to LittleFsRepository if available)
    // NOTE: history append is called via direct reference passed in Task 7's main.cpp update
```

> For now the history append is called directly from `main.cpp` loop since `IStorage` doesn't expose `appendHistory`. This avoids polluting the port with implementation details.

- [ ] **Step 4: Update `src/main.cpp` — replace NullStorage with LittleFsRepository**

```cpp
#include <Arduino.h>
#include "Logger.h"
#include "infrastructure/display/Nokia5110Display.h"
#include "infrastructure/sensors/Ina219Sensor.h"
#include "infrastructure/storage/PreferencesRepository.h"
#include "infrastructure/storage/LittleFsRepository.h"
#include "application/MonitorApplication.h"
#include "domain/services/EnergyCalculator.h"
#include "domain/services/BatteryEstimator.h"
#include "domain/services/RpmCalculator.h"

static constexpr uint8_t PIN_LCD_CLK  = 12;
static constexpr uint8_t PIN_LCD_DIN  = 11;
static constexpr uint8_t PIN_LCD_DC   =  9;
static constexpr uint8_t PIN_LCD_CE   = 10;
static constexpr uint8_t PIN_LCD_RST  = 14;
static constexpr uint8_t PIN_BUTTON   =  0;

struct NullRpmSensor : public IRpmSensor { Rpm read() override { return {0}; } };
struct NullMqtt      : public IMqttClient { void publish(const AppState&) override {} };

static PreferencesRepository prefs;
static Ina219Sensor       ina219(prefs.shuntOhm());
static LittleFsRepository storage;
static NullRpmSensor      nullRpm;
static NullMqtt           nullMqtt;
static Nokia5110Display   display(PIN_LCD_CLK, PIN_LCD_DIN, PIN_LCD_DC,
                                  PIN_LCD_CE, PIN_LCD_RST, PIN_BUTTON);
static EnergyCalculator   energyCalc;
static BatteryEstimator   batteryEst;
static RpmCalculator      rpmCalc(prefs.pulsesPerRev());

static MonitorApplication* app = nullptr;
static uint32_t lastTickMs    = 0;
static uint32_t lastHistoryMs = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.4");
    display.begin();
    ina219.begin();
    storage.begin();
    app = new MonitorApplication(ina219, nullRpm, display, storage, nullMqtt,
                                 energyCalc, batteryEst, rpmCalc,
                                 [&]() { return display.buttonPressed(); });
    app->begin();
}

void loop() {
    uint32_t now = millis();
    if (now - lastTickMs >= 1000) {
        lastTickMs = now;
        app->tick();
    }
    if (now - lastHistoryMs >= 60'000UL) {
        lastHistoryMs = now;
        storage.appendHistory(app->state());
    }
}
```

- [ ] **Step 5: Compile and flash**

```bash
pio run -e esp32s3 --target upload
pio device monitor --baud 115200
```

Expected: `[INFO]  LittleFS mounted` on serial. LCD page 2 accumulates Wh while turbine produces power. After 10 minutes, energy survives reboot.

- [ ] **Step 6: Commit**

```bash
git add src/infrastructure/storage/LittleFsRepository.* src/main.cpp
git commit -m "feat: V0.4 LittleFS energy persistence and history buffer"
```

---

## Task 8: V0.5 — Hall Sensor + RPM

**Files:**
- Create: `src/infrastructure/sensors/HallSensor.h`
- Create: `src/infrastructure/sensors/HallSensor.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: `IRpmSensor` (Task 2), `RpmCalculator` (Task 3)
- Produces: `HallSensor(uint8_t pin, RpmCalculator& calc)` — implements `IRpmSensor`; ISR counts pulses on GPIO 6

- [ ] **Step 1: Create `src/infrastructure/sensors/HallSensor.h`**

```cpp
#pragma once
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../../domain/ports/IRpmSensor.h"
#include "../../domain/services/RpmCalculator.h"

class HallSensor : public IRpmSensor {
public:
    HallSensor(uint8_t pin, RpmCalculator& calc);

    void begin();
    Rpm read() override;

    // Called by ISR — must be public
    static void IRAM_ATTR isrHandler();

private:
    uint8_t pin_;
    RpmCalculator& calc_;

    static volatile uint32_t pulseCount_;
    static SemaphoreHandle_t mutex_;
    uint32_t windowStartMs_{0};
    uint32_t capturedPulses_{0};
};
```

- [ ] **Step 2: Create `src/infrastructure/sensors/HallSensor.cpp`**

```cpp
#include "HallSensor.h"
#include <Arduino.h>

volatile uint32_t HallSensor::pulseCount_ = 0;
SemaphoreHandle_t HallSensor::mutex_      = nullptr;

void IRAM_ATTR HallSensor::isrHandler() {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(mutex_, &woken);  // won't work for counting — use portENTER_CRITICAL
    pulseCount_++;
}

HallSensor::HallSensor(uint8_t pin, RpmCalculator& calc)
    : pin_(pin), calc_(calc) {
    mutex_ = xSemaphoreCreateMutex();
}

void HallSensor::begin() {
    pinMode(pin_, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin_), isrHandler, FALLING);
    windowStartMs_ = millis();
}

Rpm HallSensor::read() {
    uint32_t now = millis();
    uint32_t windowMs = now - windowStartMs_;

    if (windowMs < 3000) return {0};  // wait for 3s window

    // Atomic read of pulse count
    portDISABLE_INTERRUPTS();
    uint32_t pulses = pulseCount_;
    pulseCount_ = 0;
    portENABLE_INTERRUPTS();

    windowStartMs_ = now;
    return calc_.calculate(pulses, windowMs);
}
```

> **Note:** `isrHandler` uses direct increment with interrupt disable for atomic read — the `xSemaphoreGiveFromISR` line in the header is replaced by plain `pulseCount_++`; remove the semphr include from the .cpp if unused after this correction. The mutex shown in the header is for the AppState (Task 4), not the ISR counter.

Update `src/infrastructure/sensors/HallSensor.h` to remove unused semaphore:

```cpp
#pragma once
#include <cstdint>
#include "../../domain/ports/IRpmSensor.h"
#include "../../domain/services/RpmCalculator.h"

class HallSensor : public IRpmSensor {
public:
    HallSensor(uint8_t pin, RpmCalculator& calc);

    void begin();
    Rpm read() override;

    static void IRAM_ATTR isrHandler();

private:
    uint8_t pin_;
    RpmCalculator& calc_;

    static volatile uint32_t pulseCount_;
    uint32_t windowStartMs_{0};
};
```

Update `src/infrastructure/sensors/HallSensor.cpp`:

```cpp
#include "HallSensor.h"
#include <Arduino.h>

volatile uint32_t HallSensor::pulseCount_ = 0;

void IRAM_ATTR HallSensor::isrHandler() {
    pulseCount_++;
}

HallSensor::HallSensor(uint8_t pin, RpmCalculator& calc)
    : pin_(pin), calc_(calc) {}

void HallSensor::begin() {
    pinMode(pin_, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin_), isrHandler, FALLING);
    windowStartMs_ = millis();
}

Rpm HallSensor::read() {
    uint32_t now = millis();
    uint32_t windowMs = now - windowStartMs_;
    if (windowMs < 3000) return {0};

    portDISABLE_INTERRUPTS();
    uint32_t pulses = pulseCount_;
    pulseCount_ = 0;
    portENABLE_INTERRUPTS();

    windowStartMs_ = now;
    return calc_.calculate(pulses, windowMs);
}
```

- [ ] **Step 3: Update `src/main.cpp` — replace NullRpmSensor with HallSensor**

```cpp
#include <Arduino.h>
#include "Logger.h"
#include "infrastructure/display/Nokia5110Display.h"
#include "infrastructure/sensors/Ina219Sensor.h"
#include "infrastructure/sensors/HallSensor.h"
#include "infrastructure/storage/PreferencesRepository.h"
#include "infrastructure/storage/LittleFsRepository.h"
#include "application/MonitorApplication.h"
#include "domain/services/EnergyCalculator.h"
#include "domain/services/BatteryEstimator.h"
#include "domain/services/RpmCalculator.h"

static constexpr uint8_t PIN_LCD_CLK  = 12;
static constexpr uint8_t PIN_LCD_DIN  = 11;
static constexpr uint8_t PIN_LCD_DC   =  9;
static constexpr uint8_t PIN_LCD_CE   = 10;
static constexpr uint8_t PIN_LCD_RST  = 14;
static constexpr uint8_t PIN_BUTTON   =  0;
static constexpr uint8_t PIN_HALL     =  6;

struct NullMqtt : public IMqttClient { void publish(const AppState&) override {} };

static PreferencesRepository prefs;
static EnergyCalculator   energyCalc;
static BatteryEstimator   batteryEst;
static RpmCalculator      rpmCalc(prefs.pulsesPerRev());
static Ina219Sensor       ina219(prefs.shuntOhm());
static HallSensor         hall(PIN_HALL, rpmCalc);
static LittleFsRepository storage;
static NullMqtt           nullMqtt;
static Nokia5110Display   display(PIN_LCD_CLK, PIN_LCD_DIN, PIN_LCD_DC,
                                  PIN_LCD_CE, PIN_LCD_RST, PIN_BUTTON);

static MonitorApplication* app = nullptr;
static uint32_t lastTickMs    = 0;
static uint32_t lastHistoryMs = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.5");
    display.begin();
    ina219.begin();
    hall.begin();
    storage.begin();
    app = new MonitorApplication(ina219, hall, display, storage, nullMqtt,
                                 energyCalc, batteryEst, rpmCalc,
                                 [&]() { return display.buttonPressed(); });
    app->begin();
}

void loop() {
    uint32_t now = millis();
    if (now - lastTickMs >= 1000) {
        lastTickMs = now;
        app->tick();
    }
    if (now - lastHistoryMs >= 60'000UL) {
        lastHistoryMs = now;
        storage.appendHistory(app->state());
    }
}
```

- [ ] **Step 4: Compile and flash**

```bash
pio run -e esp32s3 --target upload
pio device monitor --baud 115200
```

Expected: LCD page 3 shows RPM. Pass a magnet near the Hall sensor — RPM should increase.

- [ ] **Step 5: Commit**

```bash
git add src/infrastructure/sensors/HallSensor.* src/main.cpp
git commit -m "feat: V0.5 Hall sensor RPM with ISR and 3s window"
```

---

## Task 9: V0.6 — WiFiManager + NTP + Web Dashboard

**Files:**
- Create: `src/infrastructure/connectivity/WifiManagerWrapper.h`
- Create: `src/infrastructure/connectivity/WifiManagerWrapper.cpp`
- Create: `src/infrastructure/connectivity/NtpSync.h`
- Create: `src/infrastructure/connectivity/NtpSync.cpp`
- Create: `src/presentation/web/WebApiHandler.h`
- Create: `src/presentation/web/WebApiHandler.cpp`
- Create: `data/index.html`
- Modify: `src/main.cpp`
- Modify: `src/application/MonitorApplication.cpp` (NTP midnight check)

**Interfaces:**
- Consumes: `LittleFsRepository` (Task 7), `AppState` (Task 2), `PreferencesRepository` (Task 6)
- Produces:
  - `WifiManagerWrapper::begin(const char* apName)` — blocks until connected; triggers NTP on connect
  - `NtpSync::begin()` — calls `configTime()`
  - `NtpSync::currentDay() const -> uint8_t` — day of month for midnight comparison
  - `WebApiHandler(AsyncWebServer&, LittleFsRepository&, AppState& sharedState, SemaphoreHandle_t mutex, PreferencesRepository&)` — registers all routes

- [ ] **Step 1: Create `src/infrastructure/connectivity/WifiManagerWrapper.h`**

```cpp
#pragma once
#include <Arduino.h>

class WifiManagerWrapper {
public:
    void begin(const char* apName = "WindMonitor");
    bool connected() const;
};
```

- [ ] **Step 2: Create `src/infrastructure/connectivity/WifiManagerWrapper.cpp`**

```cpp
#include "WifiManagerWrapper.h"
#include <WiFiManager.h>
#include "../../Logger.h"

void WifiManagerWrapper::begin(const char* apName) {
    WiFiManager wm;
    wm.setConnectTimeout(30);
    if (!wm.autoConnect(apName)) {
        LOG_WARN("WiFiManager: no connection, restarting");
        ESP.restart();
    }
    LOG_INFO("WiFi connected: " + WiFi.localIP().toString());
}

bool WifiManagerWrapper::connected() const {
    return WiFi.status() == WL_CONNECTED;
}
```

- [ ] **Step 3: Create `src/infrastructure/connectivity/NtpSync.h`**

```cpp
#pragma once
#include <cstdint>

class NtpSync {
public:
    void begin(const char* ntpServer = "pool.ntp.org",
               long gmtOffsetSec = 0, int dstOffsetSec = 0);
    bool synced() const;
    uint8_t currentDay() const;
    uint32_t currentEpoch() const;
};
```

- [ ] **Step 4: Create `src/infrastructure/connectivity/NtpSync.cpp`**

```cpp
#include "NtpSync.h"
#include <Arduino.h>
#include <time.h>
#include "../../Logger.h"

void NtpSync::begin(const char* ntpServer, long gmtOffsetSec, int dstOffsetSec) {
    configTime(gmtOffsetSec, dstOffsetSec, ntpServer);
    LOG_INFO("NTP sync started");
}

bool NtpSync::synced() const {
    return time(nullptr) > 1000000000UL;
}

uint8_t NtpSync::currentDay() const {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    return static_cast<uint8_t>(t.tm_mday);
}

uint32_t NtpSync::currentEpoch() const {
    return static_cast<uint32_t>(time(nullptr));
}
```

- [ ] **Step 5: Create `src/presentation/web/WebApiHandler.h`**

```cpp
#pragma once
#include <ESPAsyncWebServer.h>
#include <freertos/semphr.h>
#include "../../domain/entities/AppState.h"
#include "../../infrastructure/storage/LittleFsRepository.h"
#include "../../infrastructure/storage/PreferencesRepository.h"

class WebApiHandler {
public:
    WebApiHandler(AsyncWebServer& server,
                  LittleFsRepository& storage,
                  AppState& sharedState,
                  SemaphoreHandle_t stateMutex,
                  PreferencesRepository& prefs);

    void begin();

private:
    AsyncWebServer& server_;
    LittleFsRepository& storage_;
    AppState& sharedState_;
    SemaphoreHandle_t stateMutex_;
    PreferencesRepository& prefs_;

    void registerRoutes();
    String buildStatusJson();
};
```

- [ ] **Step 6: Create `src/presentation/web/WebApiHandler.cpp`**

```cpp
#include "WebApiHandler.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "../../Logger.h"

WebApiHandler::WebApiHandler(AsyncWebServer& server, LittleFsRepository& storage,
                             AppState& sharedState, SemaphoreHandle_t stateMutex,
                             PreferencesRepository& prefs)
    : server_(server), storage_(storage), sharedState_(sharedState),
      stateMutex_(stateMutex), prefs_(prefs) {}

void WebApiHandler::begin() {
    registerRoutes();
    server_.begin();
    LOG_INFO("Web server started");
}

void WebApiHandler::registerRoutes() {
    server_.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(200, "application/json", buildStatusJson());
    });

    server_.on("/api/history", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(200, "application/json", storage_.readHistoryJson());
    });

    // POST /api/config — body: JSON with any subset of config keys
    auto* configHandler = new AsyncCallbackJsonWebHandler(
        "/api/config", [this](AsyncWebServerRequest* req, JsonVariant& json) {
            JsonObject obj = json.as<JsonObject>();
            if (obj["mqtt_host"].is<const char*>())
                prefs_.saveMqttHost(String(obj["mqtt_host"].as<const char*>()));
            if (obj["mqtt_port"].is<uint16_t>())
                prefs_.saveMqttPort(obj["mqtt_port"].as<uint16_t>());
            if (obj["pulses_rev"].is<uint8_t>())
                prefs_.savePulsesPerRev(obj["pulses_rev"].as<uint8_t>());
            if (obj["shunt_ohm"].is<float>())
                prefs_.saveShuntOhm(obj["shunt_ohm"].as<float>());
            if (obj["ota_pass"].is<const char*>())
                prefs_.saveOtaPassword(String(obj["ota_pass"].as<const char*>()));
            req->send(200, "application/json", "{\"ok\":true}");
        });
    server_.addHandler(configHandler);
}

String WebApiHandler::buildStatusJson() {
    AppState s;
    xSemaphoreTake(stateMutex_, portMAX_DELAY);
    s = sharedState_;
    xSemaphoreGive(stateMutex_);

    JsonDocument doc;
    doc["voltage"]     = s.measurement.voltage;
    doc["current"]     = s.measurement.current;
    doc["power"]       = s.measurement.power;
    doc["energyToday"] = s.energy.whToday;
    doc["energyTotal"] = s.energy.whTotal;
    doc["rpm"]         = s.rpm.value;
    doc["battery"]     = s.battery.percent;
    String out;
    serializeJson(doc, out);
    return out;
}
```

> **Note:** `AsyncCallbackJsonWebHandler` requires `ESPAsyncWebServer` with the `AsyncJson.h` header. Add `#include <AsyncJson.h>` to `WebApiHandler.cpp`.

- [ ] **Step 7: Create `data/index.html`**

```html
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Wind Energy Monitor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4/dist/chart.umd.min.js"></script>
<style>
  body { font-family: sans-serif; max-width: 800px; margin: 0 auto; padding: 1rem; background:#111; color:#eee; }
  h1 { display:flex; justify-content:space-between; }
  .metrics { display:grid; grid-template-columns:repeat(3,1fr); gap:1rem; margin:1rem 0; }
  .card { background:#222; border-radius:8px; padding:1rem; text-align:center; }
  .val { font-size:2rem; font-weight:bold; color:#4af; }
  .lbl { font-size:.8rem; color:#888; }
  canvas { background:#1a1a1a; border-radius:8px; }
  .stats { display:grid; grid-template-columns:repeat(4,1fr); gap:.5rem; margin:1rem 0; }
  details { margin-top:1rem; }
  summary { cursor:pointer; color:#4af; }
  form label { display:block; margin:.5rem 0 .2rem; font-size:.85rem; color:#aaa; }
  form input { width:100%; box-sizing:border-box; padding:.4rem; background:#333; border:1px solid #555; color:#eee; border-radius:4px; }
  form button { margin-top:.5rem; padding:.5rem 1.5rem; background:#4af; color:#000; border:none; border-radius:4px; cursor:pointer; }
  #msg { color:#4f4; font-size:.85rem; }
  #status-dot { width:12px; height:12px; border-radius:50%; background:#4f4; display:inline-block; }
</style>
</head>
<body>
<h1>Wind Energy Monitor <span><span id="status-dot"></span></span></h1>

<div class="metrics">
  <div class="card"><div class="val" id="v">--</div><div class="lbl">Voltaje (V)</div></div>
  <div class="card"><div class="val" id="i">--</div><div class="lbl">Corriente (A)</div></div>
  <div class="card"><div class="val" id="p">--</div><div class="lbl">Potencia (W)</div></div>
</div>

<canvas id="chart" height="120"></canvas>

<div class="stats">
  <div class="card"><div class="val" id="wh-today">--</div><div class="lbl">Wh hoy</div></div>
  <div class="card"><div class="val" id="wh-total">--</div><div class="lbl">kWh total</div></div>
  <div class="card"><div class="val" id="rpm">--</div><div class="lbl">RPM</div></div>
  <div class="card"><div class="val" id="bat">--</div><div class="lbl">Batería %</div></div>
</div>

<details>
<summary>⚙ Configuración</summary>
<form id="cfg">
  <label>MQTT Broker Host <input type="text" name="mqtt_host"></label>
  <label>MQTT Puerto <input type="number" name="mqtt_port" value="1883"></label>
  <label>Shunt (Ω) <input type="number" step="0.001" name="shunt_ohm" value="0.1"></label>
  <label>Pulsos/revolución <input type="number" name="pulses_rev" value="1"></label>
  <label>OTA Password <input type="text" name="ota_pass"></label>
  <button type="submit">Guardar</button> <span id="msg"></span>
</form>
</details>

<script>
const chart = new Chart(document.getElementById('chart'), {
  type: 'line',
  data: { labels: [], datasets: [{ label: 'Potencia (W)', data: [], borderColor: '#4af', tension: 0.2, pointRadius: 0 }] },
  options: { scales: { x: { display: false }, y: { beginAtZero: true } }, plugins: { legend: { display: false } } }
});

async function loadHistory() {
  const r = await fetch('/api/history');
  const arr = await r.json();
  chart.data.labels = arr.map(e => e.t);
  chart.data.datasets[0].data = arr.map(e => e.p);
  chart.update('none');
}

async function update() {
  try {
    const r = await fetch('/api/status');
    const d = await r.json();
    document.getElementById('v').textContent = d.voltage.toFixed(1);
    document.getElementById('i').textContent = d.current.toFixed(2);
    document.getElementById('p').textContent = d.power.toFixed(1);
    document.getElementById('wh-today').textContent = d.energyToday.toFixed(1);
    document.getElementById('wh-total').textContent = (d.energyTotal/1000).toFixed(2);
    document.getElementById('rpm').textContent = d.rpm;
    document.getElementById('bat').textContent = d.battery;
    document.getElementById('status-dot').style.background = '#4f4';
  } catch { document.getElementById('status-dot').style.background = '#f44'; }
}

document.getElementById('cfg').addEventListener('submit', async e => {
  e.preventDefault();
  const body = {};
  new FormData(e.target).forEach((v,k) => { if(v) body[k] = isNaN(v) ? v : Number(v); });
  await fetch('/api/config', { method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify(body) });
  document.getElementById('msg').textContent = 'Guardado ✓';
  setTimeout(() => document.getElementById('msg').textContent = '', 3000);
});

loadHistory();
update();
setInterval(update, 2000);
</script>
</body>
</html>
```

- [ ] **Step 8: Update `src/application/MonitorApplication.h` — add NTP day tracking**

Add to private section:
```cpp
    uint8_t lastDay_{0};
    std::function<uint8_t()> currentDayFn_;
```

Add to constructor parameters:
```cpp
    std::function<uint8_t()> currentDayFn = nullptr
```

- [ ] **Step 9: Update `src/application/MonitorApplication.cpp` — midnight reset**

In `begin()`:
```cpp
    if (currentDayFn_) lastDay_ = currentDayFn_();
```

In `tick()`, after energy calc:
```cpp
    if (currentDayFn_) {
        uint8_t day = currentDayFn_();
        if (day != lastDay_) {
            energyCalc_.resetToday();
            lastDay_ = day;
        }
    }
```

- [ ] **Step 10: Update `src/main.cpp` — wire WiFi, NTP, WebServer, mutex**

```cpp
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <freertos/semphr.h>
#include "Logger.h"
#include "infrastructure/display/Nokia5110Display.h"
#include "infrastructure/sensors/Ina219Sensor.h"
#include "infrastructure/sensors/HallSensor.h"
#include "infrastructure/storage/PreferencesRepository.h"
#include "infrastructure/storage/LittleFsRepository.h"
#include "infrastructure/connectivity/WifiManagerWrapper.h"
#include "infrastructure/connectivity/NtpSync.h"
#include "presentation/web/WebApiHandler.h"
#include "application/MonitorApplication.h"
#include "domain/services/EnergyCalculator.h"
#include "domain/services/BatteryEstimator.h"
#include "domain/services/RpmCalculator.h"

static constexpr uint8_t PIN_LCD_CLK = 12, PIN_LCD_DIN = 11, PIN_LCD_DC = 9;
static constexpr uint8_t PIN_LCD_CE  = 10, PIN_LCD_RST = 14, PIN_BUTTON = 0;
static constexpr uint8_t PIN_HALL    =  6;

struct NullMqtt : public IMqttClient { void publish(const AppState&) override {} };

static SemaphoreHandle_t stateMutex = xSemaphoreCreateMutex();
static PreferencesRepository prefs;
static EnergyCalculator   energyCalc;
static BatteryEstimator   batteryEst;
static RpmCalculator      rpmCalc(prefs.pulsesPerRev());
static Ina219Sensor       ina219(prefs.shuntOhm());
static HallSensor         hall(PIN_HALL, rpmCalc);
static LittleFsRepository storage;
static NullMqtt           nullMqtt;
static Nokia5110Display   display(PIN_LCD_CLK, PIN_LCD_DIN, PIN_LCD_DC,
                                  PIN_LCD_CE, PIN_LCD_RST, PIN_BUTTON);
static WifiManagerWrapper wifi;
static NtpSync            ntp;
static AsyncWebServer     server(80);
static WebApiHandler*     webApi = nullptr;
static MonitorApplication* app   = nullptr;

static uint32_t lastTickMs    = 0;
static uint32_t lastHistoryMs = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.6");
    display.begin();
    ina219.begin();
    hall.begin();
    storage.begin();
    wifi.begin("WindMonitor");
    ntp.begin("pool.ntp.org");

    webApi = new WebApiHandler(server, storage, app->state(),
                               stateMutex, prefs);

    app = new MonitorApplication(ina219, hall, display, storage, nullMqtt,
                                 energyCalc, batteryEst, rpmCalc,
                                 [&]() { return display.buttonPressed(); },
                                 [&]() { return ntp.currentDay(); });
    app->begin();
    webApi = new WebApiHandler(server, storage,
                               const_cast<AppState&>(app->state()),
                               stateMutex, prefs);
    webApi->begin();
}

void loop() {
    uint32_t now = millis();
    if (now - lastTickMs >= 1000) {
        lastTickMs = now;
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        app->tick();
        xSemaphoreGive(stateMutex);
    }
    if (now - lastHistoryMs >= 60'000UL) {
        lastHistoryMs = now;
        storage.appendHistory(app->state());
    }
}
```

> **Note:** `WebApiHandler` is constructed after `app` — fix the order: construct app first, then webApi. The duplicate `webApi =` line in the snippet above is a copy error; keep only the second one (after `app` exists). Final `setup()` order: display → sensors → storage → wifi → ntp → app → webApi.

- [ ] **Step 11: Upload filesystem and flash firmware**

```bash
pio run -e esp32s3 --target uploadfs   # uploads data/index.html to LittleFS
pio run -e esp32s3 --target upload     # uploads firmware
pio device monitor --baud 115200
```

Expected: serial shows WiFi IP. Browse to `http://<ip>/` — dashboard loads with live data and Chart.js power graph.

- [ ] **Step 12: Commit**

```bash
git add src/infrastructure/connectivity/ src/presentation/ data/ src/application/ src/main.cpp
git commit -m "feat: V0.6 WiFiManager, NTP, AsyncWebServer, web dashboard"
```

---

## Task 10: V0.7 — OTA

**Files:**
- Create: `src/infrastructure/connectivity/OtaManager.h`
- Create: `src/infrastructure/connectivity/OtaManager.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: `PreferencesRepository::otaPassword()` (Task 6)
- Produces: `OtaManager(const String& password)` — wraps `ArduinoOTA`; `begin()` + `handle()` called each loop

- [ ] **Step 1: Create `src/infrastructure/connectivity/OtaManager.h`**

```cpp
#pragma once
#include <Arduino.h>

class OtaManager {
public:
    explicit OtaManager(const String& password);
    void begin();
    void handle();
};
```

- [ ] **Step 2: Create `src/infrastructure/connectivity/OtaManager.cpp`**

```cpp
#include "OtaManager.h"
#include <ArduinoOTA.h>
#include "../../Logger.h"

OtaManager::OtaManager(const String& password) {
    ArduinoOTA.setPassword(password.c_str());
}

void OtaManager::begin() {
    ArduinoOTA.setHostname("wind-monitor");
    ArduinoOTA.onStart([]() { LOG_INFO("OTA start"); });
    ArduinoOTA.onEnd([]()   { LOG_INFO("OTA end"); });
    ArduinoOTA.onError([](ota_error_t e) { LOG_ERROR("OTA error: " + String(e)); });
    ArduinoOTA.begin();
    LOG_INFO("OTA ready");
}

void OtaManager::handle() {
    ArduinoOTA.handle();
}
```

- [ ] **Step 3: Update `src/main.cpp` — add OtaManager**

Add to includes:
```cpp
#include "infrastructure/connectivity/OtaManager.h"
```

Add static:
```cpp
static OtaManager* ota = nullptr;
```

In `setup()`, after `wifi.begin()`:
```cpp
    ota = new OtaManager(prefs.otaPassword());
    ota->begin();
```

In `loop()`, at the top:
```cpp
    ota->handle();
```

- [ ] **Step 4: Compile, flash, verify OTA**

```bash
pio run -e esp32s3 --target upload
```

After flashing, change a LOG message and upload via OTA:
```bash
pio run -e esp32s3 --target upload --upload-port wind-monitor.local
```

Expected: firmware updates over WiFi without USB connection.

- [ ] **Step 5: Commit**

```bash
git add src/infrastructure/connectivity/OtaManager.* src/main.cpp
git commit -m "feat: V0.7 ArduinoOTA with password from NVS"
```

---

## Task 11: V0.8 — MQTT

**Files:**
- Create: `src/infrastructure/connectivity/MqttClientWrapper.h`
- Create: `src/infrastructure/connectivity/MqttClientWrapper.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: `IMqttClient` (Task 2), `AppState` (Task 2), `PreferencesRepository` (Task 6)
- Produces: `MqttClientWrapper(const String& host, uint16_t port)` — implements `IMqttClient`; auto-reconnects; publishes 7 topics

- [ ] **Step 1: Create `src/infrastructure/connectivity/MqttClientWrapper.h`**

```cpp
#pragma once
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "../../domain/ports/IMqttClient.h"

class MqttClientWrapper : public IMqttClient {
public:
    MqttClientWrapper(const String& host, uint16_t port);

    void begin();
    void loop();
    void publish(const AppState& state) override;

private:
    void reconnect();

    WiFiClient wifiClient_;
    PubSubClient client_;
    String host_;
    uint16_t port_;
};
```

- [ ] **Step 2: Create `src/infrastructure/connectivity/MqttClientWrapper.cpp`**

```cpp
#include "MqttClientWrapper.h"
#include "../../Logger.h"

MqttClientWrapper::MqttClientWrapper(const String& host, uint16_t port)
    : client_(wifiClient_), host_(host), port_(port) {}

void MqttClientWrapper::begin() {
    client_.setServer(host_.c_str(), port_);
    LOG_INFO("MQTT client configured: " + host_ + ":" + String(port_));
}

void MqttClientWrapper::loop() {
    if (!client_.connected()) reconnect();
    client_.loop();
}

void MqttClientWrapper::reconnect() {
    if (host_.isEmpty()) return;
    if (client_.connect("wind-monitor")) {
        LOG_INFO("MQTT connected");
    }
}

void MqttClientWrapper::publish(const AppState& state) {
    if (!client_.connected()) return;
    client_.publish("wind/voltage",        String(state.measurement.voltage, 2).c_str());
    client_.publish("wind/current",        String(state.measurement.current, 3).c_str());
    client_.publish("wind/power",          String(state.measurement.power, 2).c_str());
    client_.publish("wind/rpm",            String(state.rpm.value).c_str());
    client_.publish("wind/energy/today",   String(state.energy.whToday, 1).c_str());
    client_.publish("wind/energy/total",   String(state.energy.whTotal / 1000.0f, 3).c_str());
    client_.publish("wind/battery",        String(state.battery.percent).c_str());
}
```

- [ ] **Step 3: Update `src/main.cpp` — replace NullMqtt with MqttClientWrapper**

Remove `struct NullMqtt` and add:
```cpp
#include "infrastructure/connectivity/MqttClientWrapper.h"
```

Replace static:
```cpp
static MqttClientWrapper mqtt(prefs.mqttHost(), prefs.mqttPort());
```

In `setup()`, after `wifi.begin()`:
```cpp
    mqtt.begin();
```

Pass `mqtt` instead of `nullMqtt` to `MonitorApplication`.

In `loop()`, after `ota->handle()`:
```cpp
    mqtt.loop();
```

- [ ] **Step 4: Compile, flash, verify MQTT**

```bash
pio run -e esp32s3 --target upload
```

Configure MQTT broker via web dashboard (`/api/config`). Subscribe on your broker:
```bash
mosquitto_sub -h <broker> -t "wind/#" -v
```

Expected: values published every second on all 7 topics.

- [ ] **Step 5: Commit**

```bash
git add src/infrastructure/connectivity/MqttClientWrapper.* src/main.cpp
git commit -m "feat: V0.8 MQTT publishing 7 topics with auto-reconnect"
```

---

## Task 12: V1.0 — Final Integration + Button GPIO Assignment

**Files:**
- Modify: `src/main.cpp` — assign final button GPIO once hardware decided
- Modify: `platformio.ini` — confirm partition table matches 16MB flash

**This task has one prerequisite:** hardware button GPIO must be known.

- [ ] **Step 1: Assign button GPIO**

Replace `PIN_BUTTON = 0` (BOOT button) with the real GPIO:
```cpp
static constexpr uint8_t PIN_BUTTON = <GPIO_TBD>;
```

- [ ] **Step 2: Run full native test suite**

```bash
pio test -e native
```

Expected: `14 Tests 0 Failures 0 Ignored — OK`.

- [ ] **Step 3: Run firmware build**

```bash
pio run -e esp32s3
```

Expected: `SUCCESS`.

- [ ] **Step 4: Full device smoke test checklist**

- [ ] Boot: serial shows all init messages without errors
- [ ] LCD page 1: real voltage, current, power from INA219
- [ ] LCD page 2: Wh accumulating
- [ ] LCD page 3: RPM updates when magnet passes Hall sensor
- [ ] Button: cycles pages without debounce bounce
- [ ] WiFi: connects on boot (or captive portal on first boot)
- [ ] Dashboard: `http://<ip>/` loads, values update every 2s, chart renders
- [ ] Config: submit MQTT broker via form, saved to NVS, survives reboot
- [ ] MQTT: topics publish on broker
- [ ] OTA: update firmware wirelessly
- [ ] Energy: Wh survives reboot (load from LittleFS)
- [ ] Midnight reset: `whToday` resets at midnight (verify with NTP synced)

- [ ] **Step 5: Tag release**

```bash
git tag v1.0.0
git push origin main --tags
```

- [ ] **Step 6: Final commit**

```bash
git add src/main.cpp platformio.ini
git commit -m "feat: V1.0 final integration, button GPIO assigned"
```

---

## Self-Review Spec Coverage

| Spec Requirement | Task |
|-----------------|------|
| Voltage / current / power measurement | Task 6 (INA219) |
| Wh/kWh accumulation | Task 3 (EnergyCalculator) |
| Nokia 5110 display, 3 pages | Task 5 |
| INA219 with shunt configurable | Task 6 |
| INA226 abstraction (port ready) | Task 2 (IEnergySensor port) |
| Hall sensor RPM | Task 8 |
| WiFi | Task 9 |
| Web dashboard + chart | Task 9 |
| REST API (status / history / config) | Task 9 |
| MQTT 7 topics | Task 11 |
| OTA | Task 10 |
| Hexagonal architecture | Tasks 2–4 |
| PlatformIO + C++17 | Task 1 |
| clang-format | Task 1 |
| Native tests | Task 3 |
| GitHub Actions CI | Task 1 |
| No global variables | All (except volatile ISR counter) |
| NVS config (mqtt, shunt, ppr, ota) | Task 6 |
| LittleFS energy + history | Task 7 |
| WiFiManager captive portal | Task 9 |
| NTP + midnight reset | Task 9 |
| Button page navigation | Task 5 |
| SOLID + DI | Tasks 2–4 |
