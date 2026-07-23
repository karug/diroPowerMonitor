# diro Power Monitor

Monitor de energía DC basado en ESP32-S3, compatible con aerogenerador, panel solar o cualquier fuente de 12 V. Mide voltaje, corriente, potencia y energía (Wh/kWh), muestra los datos en pantalla LCD Nokia 5110, expone un dashboard web con gráfico de 24 h, publica por MQTT y soporta actualizaciones OTA.

---

## Características

- **Medición** — Voltaje, corriente y potencia en tiempo real vía INA219 (I²C)
- **Fuentes** — Compatible con aerogenerador, panel solar o cualquier fuente DC 12 V
- **RPM** — Sensor Hall A3144 con ISR, ventana de 3 s, pulsos/rev configurable (aerogenerador)
- **Pantalla** — Nokia 5110 (PCD8544), 3 páginas navegables con botonera analógica
- **Web dashboard** — Página HTML embebida en LittleFS, gráfico de potencia, configuración live
- **WiFi** — Provisioning por portal cautivo (WiFiManager), sin recompilación
- **NTP** — Sincronización horaria para reset de Wh·día a medianoche
- **MQTT** — 7 topics en tiempo real (`pm/voltage`, `pm/current`, …)
- **OTA** — Actualizaciones inalámbricas vía ArduinoOTA, contraseña en NVS
- **Persistencia** — Configuración en NVS; energía e historial en LittleFS (flash 16 MB)

---

## Hardware necesario

| Componente | Descripción | Enlace |
|-----------|-------------|--------|
| ESP32-S3-N16R8 DevKit | Microcontrolador principal (16 MB flash, 8 MB PSRAM) | [Ver en tienda][link-esp32s3] |
| Módulo INA219 (GY-219) | Sensor de voltaje/corriente I²C, hasta 26 V / 3.2 A | [Ver en tienda][link-ina219] |
| Pantalla Nokia 5110 | LCD 84×48 px, controlador PCD8544, SPI | [Ver en tienda][link-nokia] |
| Sensor Hall A3144 | Detecta pulsos magnéticos del aerogenerador | [Ver en tienda][link-hall] |
| Botonera Keyes AD Key | 5 botones, salida analógica única, 3.3 V compatible | [Ver en tienda][link-adkey] |
| Imán neodimio N52 | Para montar en eje del aerogenerador (1 por revolución) | [Ver en tienda][link-iman] |
| Resistencia shunt | Valor según corriente máxima (ver sección de configuración) | — |

[link-esp32s3]: https://PLACEHOLDER
[link-ina219]: https://PLACEHOLDER
[link-nokia]: https://PLACEHOLDER
[link-hall]: https://PLACEHOLDER
[link-adkey]: https://PLACEHOLDER
[link-iman]: https://PLACEHOLDER

> Sustituye cada `https://PLACEHOLDER` por tu enlace de referidos.

---

## Esquema de pines (GPIO)

### Nokia 5110 (SPI software)

| Pin LCD | GPIO ESP32-S3 |
|---------|--------------|
| CLK     | 12           |
| DIN     | 11           |
| DC      | 9            |
| CE      | 10           |
| RST     | 14           |
| VCC     | 3.3 V        |
| GND     | GND          |

### INA219 (I²C)

| Pin INA219 | GPIO ESP32-S3 |
|-----------|--------------|
| SDA       | 4            |
| SCL       | 5            |
| VCC       | 3.3 V        |
| GND       | GND          |

Conecta `IN+` e `IN−` del INA219 en serie con la línea positiva del aerogenerador (entre generador y batería). El shunt se configura en NVS — ver sección de primeros pasos.

### Sensor Hall A3144

| Pin Hall | GPIO ESP32-S3 |
|---------|--------------|
| OUT     | 6            |
| VCC     | 3.3 V        |
| GND     | GND          |

Monta el sensor frente al eje del generador con un imán de neodimio. Un imán = 1 pulso/revolución (valor por defecto). Si montas más imanes, configura `pulses_rev` en el dashboard.

### Botonera Keyes AD Key

| Pin AD Key | GPIO ESP32-S3 |
|-----------|--------------|
| OUT       | 7 (ADC1)     |
| VCC       | 3.3 V        |
| GND       | GND          |

Cualquier botón de la botonera cicla entre las 3 páginas del LCD. La detección es analógica: ADC < 3600 → botón pulsado.

---

## Arquitectura de software

```
Presentation   Nokia5110Display · WebApiHandler
               ↕
Application    MonitorApplication  (tick 1 s, Core 1)
               ↕
Domain         Entities · Services (EnergyCalculator, BatteryEstimator, RpmCalculator)
               ↕
Infrastructure INA219 · HallSensor · LittleFS · NVS · WiFiManager · MQTT · OTA
```

Arquitectura hexagonal: el dominio no tiene dependencias de hardware. Tests nativos corren en PC sin ningún dispositivo físico.

---

## Primeros pasos

### 1. Clonar y compilar

```bash
git clone https://github.com/karug/diroPowerMonitor.git
cd diroPowerMonitor
pio run -e esp32s3
```

### 2. Flashear firmware y filesystem

```bash
pio run -e esp32s3 -t upload          # firmware
pio run -e esp32s3 -t uploadfs        # dashboard HTML en LittleFS
```

### 3. Provisioning WiFi

Al primer arranque el ESP32-S3 levanta un punto de acceso **`diro-PowerMonitor`**. Conéctate con el móvil y el portal cautivo te pedirá tu red WiFi. Las credenciales se guardan en NVS — no se recompila nunca.

### 4. Configurar parámetros

Abre el dashboard en `http://<IP-del-dispositivo>` y ve a **⚙ Configuración**:

| Parámetro | Descripción | Defecto |
|-----------|-------------|---------|
| `mqtt_host` | IP o hostname del broker MQTT | vacío (MQTT deshabilitado) |
| `mqtt_port` | Puerto del broker | 1883 |
| `pulses_rev` | Pulsos Hall por revolución (nº de imanes) | 1 |
| `shunt_ohm` | Valor de la resistencia shunt en ohmios | 0.1 |

Para OTA: escribe la contraseña en NVS con el endpoint:
```bash
curl -X POST http://<IP>/api/config \
  -H "Content-Type: application/json" \
  -d '{"ota_pass":"tu_contraseña"}'
```
Si `ota_pass` está vacío, OTA queda deshabilitado.

### 5. OTA (actualizaciones inalámbricas)

```bash
pio run -e esp32s3 -t upload --upload-port <IP-del-dispositivo>
```

---

## MQTT topics

| Topic | Contenido | Ejemplo |
|-------|-----------|---------|
| `pm/voltage` | Voltaje bus (V) | `13.80` |
| `pm/current` | Corriente (A) | `1.250` |
| `pm/power` | Potencia (W) | `17.3` |
| `pm/rpm` | RPM aerogenerador | `412` |
| `pm/energy/today` | Energía hoy (Wh) | `124.1` |
| `pm/energy/total` | Energía total (kWh) | `5.832` |
| `pm/battery` | Estado batería (%) | `87` |

---

## Tests

```bash
pio test -e native   # 14 tests de dominio, corre en PC
```

---

## Estructura del proyecto

```
src/
  domain/
    entities/          Measurement, Rpm, BatteryState, EnergyStatistics, AppState
    ports/             IEnergySensor, IRpmSensor, IDisplay, IStorage, IMqttClient
    services/          EnergyCalculator, BatteryEstimator, RpmCalculator
  application/
    MonitorApplication.h/.cpp
  infrastructure/
    display/           Nokia5110Display
    sensors/           Ina219Sensor, HallSensor
    storage/           PreferencesRepository (NVS), LittleFsRepository, HistoryRepository
    network/           WifiNtpManager, WebApiHandler, MqttClient, OtaManager
data/
  index.html           Dashboard web (se sube con uploadfs)
test/
  native/              Tests unitarios de dominio
```

---

## Seguridad

Repositorio público — **ninguna credencial está en el código**. WiFi, MQTT y OTA se configuran en runtime vía NVS. El archivo `secrets.h` está en `.gitignore` aunque no existe ni hace falta.

---

## Licencia

MIT
