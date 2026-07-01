# Wind Energy Monitor — Design Document
_Date: 2026-06-30_

## Overview

ESP32-S3-N16R8 based professional monitor for a 12V wind turbine. Measures voltage, current, power, and energy (Wh/kWh). Displays data on Nokia 5110 LCD, exposes a web dashboard with 24h chart, publishes via MQTT, and supports OTA updates. Firmware written in C++17 with PlatformIO using hexagonal architecture.

---

## Decisions Log

| Topic | Decision | Rationale |
|-------|----------|-----------|
| WiFi/MQTT provisioning | WiFiManager captive portal | No recompile to change network; stores creds in NVS |
| Energy persistence | NVS for config, LittleFS for Wh + history | Config writes rare; energy/history needs wear-leveling |
| Hall pulses/rev | Configurable in NVS via dashboard | Firmware reusable across generators |
| Dashboard | HTML + Chart.js CDN, 24h history | No build step; 16MB flash has headroom |
| LCD navigation | Physical button (GPIO TBD) | More natural than auto-cycle for installed monitor |
| Sampling rates | INA219 1s, RPM 3s window, persist 10min, history 1min | Stable readings, low flash wear |
| Tests | Native (host) only, domain+services | No hardware needed in CI |
| INA219 shunt | Configurable in NVS via dashboard | Works with any GY-219 module without recompile |
| Concurrency | Hybrid: ISR for Hall, 1s loop on Core 1, AsyncWebServer on Core 0 | Simple, robust, no unnecessary RTOS tasks |

---

## Architecture

### Layer Diagram

```
┌─────────────────────────────────────────────┐
│              Presentation Layer             │
│         LcdPresenter  │  WebApiHandler      │
└──────────────┬─────────────────┬────────────┘
               │                 │
┌──────────────▼─────────────────▼────────────┐
│              Application Layer              │
│           MonitorApplication                │
│   (orchestrates use cases, 1s tick loop)    │
└──────────────┬──────────────────────────────┘
               │
┌──────────────▼──────────────────────────────┐
│               Domain Layer                  │
│  Entities: Measurement, BatteryState,       │
│            EnergyStatistics, Rpm            │
│  Services: EnergyCalculator,                │
│            BatteryEstimator, RpmCalculator  │
│  Ports:    IEnergySensor, IDisplay,         │
│            IStorage, IMqttClient, IRpmSensor│
└──────────────┬──────────────────────────────┘
               │
┌──────────────▼──────────────────────────────┐
│           Infrastructure Layer              │
│  Ina219Sensor  HallSensor  Nokia5110Display │
│  PreferencesRepository  LittleFsRepository  │
│  WifiManager  MqttClient  OtaManager        │
│  AsyncWebServer                             │
└─────────────────────────────────────────────┘
```

### Principles

- SOLID + Hexagonal: domain has zero infrastructure dependencies
- Constructor dependency injection throughout
- `MonitorApplication` is the single stateful object; instantiated in `main()`, never global
- `loop()` calls `MonitorApplication::tick()` with 1s guard

---

## Domain Layer

### Entities

```cpp
struct Measurement      { float voltage; float current; float power; };
struct Rpm              { uint16_t value; };
struct BatteryState     { uint8_t percent; bool charging; };
struct EnergyStatistics { float whToday; float whTotal; };

struct AppState {
    Measurement     measurement;
    Rpm             rpm;
    BatteryState    battery;
    EnergyStatistics energy;
};
```

### Ports

```cpp
class IEnergySensor { public: virtual Measurement read() = 0; virtual ~IEnergySensor() = default; };
class IRpmSensor    { public: virtual Rpm read() = 0;         virtual ~IRpmSensor() = default; };
enum class Page : uint8_t { PowerMetrics = 0, Energy = 1, RpmBattery = 2 };
class IDisplay      { public: virtual void show(Page, const AppState&) = 0; virtual ~IDisplay() = default; };
class IStorage      { public: virtual void save(const EnergyStatistics&) = 0;
                               virtual EnergyStatistics load() = 0;
                               virtual ~IStorage() = default; };
class IMqttClient   { public: virtual void publish(const AppState&) = 0; virtual ~IMqttClient() = default; };
```

### Services

| Service | Responsibility |
|---------|---------------|
| `EnergyCalculator` | Integrates power×time → Wh; accumulates `whToday` and `whTotal`; resets `whToday` at midnight |
| `BatteryEstimator` | Maps voltage to percent via LiFePO4 12V lookup table |
| `RpmCalculator` | Reads pulse count over 3s window, applies configurable pulses/rev divisor → RPM |

All services are pure functions of their inputs — no hardware dependencies, fully testable natively.

---

## Infrastructure Layer

### Sensors

| Class | Port | Library | Notes |
|-------|------|---------|-------|
| `Ina219Sensor` | `IEnergySensor` | `adafruit/Adafruit INA219` | Shunt value from NVS at init |
| `HallSensor` | `IRpmSensor` | — | ISR `IRAM_ATTR`, `volatile uint32_t pulseCount`, mutex-protected read |
| `Ina226Sensor` | `IEnergySensor` | future | Same port, drop-in swap |

### Display

`Nokia5110Display` implements `IDisplay`. Uses `adafruit/Adafruit PCD8544`. Three pages:

| Page | Content |
|------|---------|
| 1 | Voltage / Current / Power |
| 2 | Wh today / Wh total |
| 3 | RPM / Battery % |

Button on GPIO TBD: `digitalRead()` inside `tick()`, 50ms software debounce, cycles pages.

### Storage

| Class | Backend | Data |
|-------|---------|------|
| `PreferencesRepository` | NVS `Preferences.h`, namespace `wind` | Config keys (see below) |
| `LittleFsRepository` | LittleFS | `energy.json`, `history.json`, dashboard assets |

**NVS keys (namespace `wind`):**

| Key | Type | Default |
|-----|------|---------|
| `mqtt_host` | string | `""` |
| `mqtt_port` | uint16 | `1883` |
| `pulses_rev` | uint8 | `1` |
| `shunt_ohm` | float | `0.1` |
| `ota_pass` | string | `"windota"` |

**LittleFS layout:**

```
/data/
  index.html      ← dashboard (pio run -t uploadfs)
  history.json    ← circular buffer, 1440 entries (24h @ 1/min), ~60KB
  energy.json     ← { whToday, whTotal, lastSave }
```

`energy.json` is loaded on boot; `whToday` resets if `lastSave` date differs from current date. Written every 10 minutes. `history.json` written every 1 minute.

### Connectivity

| Class | Library | Notes |
|-------|---------|-------|
| `WifiManager` | `tzapu/WiFiManager` | Captive portal on first boot; creds in own NVS namespace |
| `NtpSync` | `configTime()` (Arduino/ESP-IDF built-in) | Syncs SNTP on WiFi connect; required for midnight `whToday` reset and `lastSave` date in `energy.json` |
| `MqttClient` | `knolleary/PubSubClient` | Auto-reconnect; publishes 7 topics on each `AppState` update |
| `OtaManager` | `ArduinoOTA` | Password from NVS `ota_pass` |

**MQTT topics:** `wind/voltage`, `wind/current`, `wind/power`, `wind/rpm`, `wind/energy/today`, `wind/energy/total`, `wind/battery`

### Web Server

`ESPAsyncWebServer` (runs on Core 0 via AsyncTCP):

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Serves `index.html` from LittleFS |
| `/api/status` | GET | JSON snapshot of current `AppState` |
| `/api/history` | GET | JSON array of last 1440 history entries |
| `/api/config` | POST | Writes any subset of NVS config keys; returns 200 |

---

## Concurrency Model

```
Core 0 (protocol stack)     Core 1 (application)
┌──────────────────┐        ┌─────────────────────────┐
│  AsyncWebServer  │        │  MonitorApplication      │
│  (ESPAsyncTCP)   │        │  tick() every 1s         │
│  WiFiManager     │        │  ├─ INA219 read (~2ms)   │
│  PubSubClient    │        │  ├─ Hall read (mutex)    │
└────────┬─────────┘        │  ├─ EnergyCalculator     │
         │  reads AppState  │  ├─ BatteryEstimator     │
         │  (mutex)         │  ├─ update AppState (mx) │
         └──────────────────│  ├─ Nokia5110 (~5ms)     │
                            │  ├─ MQTT publish (~1ms)  │
ISR (any core)              │  └─ persist if due       │
┌──────────────────┐        └─────────────────────────┘
│  HallSensor ISR  │──mutex──► pulseCount (volatile)
└──────────────────┘
```

**Single mutex** protects:
1. `AppState` — written by Core 1 tick, read by Core 0 web handler
2. `pulseCount` — written by ISR, read by `RpmCalculator` in tick

**Tick budget:** INA219 I²C ~2ms + display SPI ~5ms + MQTT ~1ms + calculations <1ms = ~10ms of 1000ms available.

Button read via `digitalRead()` inside tick — no ISR needed.

---

## Web Dashboard

Single file `index.html` (~15KB) stored in LittleFS. No build step.

**Stack:** HTML5 + inline CSS + Chart.js from CDN.

**Layout:**
```
┌─────────────────────────────────────┐
│  Wind Energy Monitor          🟢    │
├──────────┬──────────┬───────────────┤
│ 13.8 V   │ 1.25 A   │  17.2 W       │
├──────────┴──────────┴───────────────┤
│  [Power chart — last 24h]           │
├─────────────────────────────────────┤
│ Hoy: 124.2 Wh  │ Total: 5.83 kWh   │
│ RPM: 412       │ Batería: 87%      │
├─────────────────────────────────────┤
│ ⚙ Config: MQTT broker / Shunt / PPR │
└─────────────────────────────────────┘
```

**Behavior:**
- `setInterval` 2s → `GET /api/status` → update live values
- On load → `GET /api/history` → render Chart.js line chart
- Config form → `POST /api/config` → show "Saved ✓"

---

## Testing

**Native tests** (run on host, no hardware required):

```
test/
  native/
    test_energy_calculator.cpp   ← power integration, Wh accumulation, midnight reset
    test_battery_estimator.cpp   ← LiFePO4 voltage→percent table, edge cases
    test_rpm_calculator.cpp      ← pulse count / window / divisor → RPM
```

**PlatformIO envs:**

```ini
[env:esp32s3]   ; full firmware, targets real hardware
[env:native]    ; domain services tests, runs on PC
```

---

## CI/CD (GitHub Actions)

Two jobs:

```yaml
build:
  runs-on: ubuntu-latest
  steps:
    - pio run -e esp32s3          # firmware compiles

test:
  runs-on: ubuntu-latest
  steps:
    - pio test -e native          # domain tests pass

lint:
  runs-on: ubuntu-latest
  steps:
    - clang-format --dry-run --Werror src/**  # formatting enforced
```

Doxygen docs generated as artifact (non-blocking).

---

## Roadmap (unchanged from spec)

| Version | Scope |
|---------|-------|
| V0.1 | PlatformIO project, Logger, Config, Architecture, compiles |
| V0.2 | Nokia 5110 display |
| V0.3 | INA219 sensor |
| V0.4 | Energy calculation + persistence |
| V0.5 | Hall RPM |
| V0.6 | Web dashboard |
| V0.7 | OTA |
| V0.8 | MQTT |
| V1.0 | Complete |

---

## Open Items

| Item | Status |
|------|--------|
| Button GPIO | TBD — assign when hardware decided |
| INA219 shunt value | TBD — measure with multimeter; configurable at runtime |
