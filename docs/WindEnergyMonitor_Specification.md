# Wind Energy Monitor - Especificación del Proyecto

## Objetivo

Desarrollar un monitor profesional para un aerogenerador de 12 V basado
en ESP32-S3.

Características:

-   Medición de tensión, corriente, potencia y energía (Wh/kWh).
-   Pantalla Nokia 5110 (PCD8544).
-   Sensor INA219 (arquitectura preparada para INA226).
-   Sensor Hall para RPM.
-   WiFi.
-   Dashboard web.
-   MQTT.
-   OTA.
-   Arquitectura Hexagonal.
-   PlatformIO.
-   C++17.

------------------------------------------------------------------------

# Hardware

## MCU

-   ESP32-S3-N16R8 (48 pines)

## Pantalla

-   Nokia 5110 (PCD8544)

## Sensor de energía

-   INA219 (interfaz mediante abstracción para permitir INA226)

## Sensor RPM

-   Hall A3144 + imán

## Alimentación

-   Batería LiFePO4 12 V
-   Convertidor DC/DC 12→5 V

------------------------------------------------------------------------

# Pinout recomendado

  Elemento   GPIO
  ---------- ------
  LCD CLK    12
  LCD DIN    11
  LCD DC     9
  LCD CE     10
  LCD RST    14
  LCD BL     3V3
  INA SDA    4
  INA SCL    5
  Hall       6

------------------------------------------------------------------------

# Arquitectura

    src/
        application/
        domain/
            entities/
            services/
            ports/
        infrastructure/
            display/
            sensors/
            storage/
            wifi/
            mqtt/
            ota/
            web/
        presentation/
            lcd/
            api/

Principios:

-   SOLID
-   Hexagonal
-   Dependency Injection por constructor
-   Interfaces para hardware
-   Sin lógica en main()

------------------------------------------------------------------------

# Estructura del repositorio

    wind-energy-monitor/
        platformio.ini
        README.md
        docs/
        include/
        lib/
        src/
        test/

------------------------------------------------------------------------

# Módulos

## Domain

-   Measurement
-   BatteryState
-   EnergyStatistics
-   Rpm

## Services

-   EnergyCalculator
-   BatteryEstimator
-   RpmCalculator

## Infrastructure

### Sensors

-   Ina219Sensor
-   Ina226Sensor
-   HallSensor

### Display

-   Nokia5110Display

### Storage

-   PreferencesRepository

### Connectivity

-   WifiManager
-   MqttClient
-   OtaManager

### Web

-   Async Web Server
-   REST API

------------------------------------------------------------------------

# Casos de uso

-   Leer sensores
-   Calcular energía
-   Mostrar pantalla
-   Publicar MQTT
-   Guardar histórico
-   Actualizar dashboard

------------------------------------------------------------------------

# Pantallas

## Página 1

-   Voltaje
-   Corriente
-   Potencia

## Página 2

-   Wh hoy
-   Wh total

## Página 3

-   RPM
-   Estado batería

------------------------------------------------------------------------

# API REST

GET /api/status

Respuesta:

``` json
{
  "voltage":13.8,
  "current":1.25,
  "power":17.2,
  "energyToday":124.2,
  "energyTotal":5832,
  "rpm":412,
  "battery":87
}
```

------------------------------------------------------------------------

# MQTT

Topics:

-   wind/voltage
-   wind/current
-   wind/power
-   wind/rpm
-   wind/energy/today
-   wind/energy/total
-   wind/battery

------------------------------------------------------------------------

# Roadmap

## V0.1

-   Proyecto PlatformIO
-   Logger
-   Configuración
-   Arquitectura
-   Compila

## V0.2

-   Nokia 5110

## V0.3

-   INA219

## V0.4

-   Energía

## V0.5

-   Hall RPM

## V0.6

-   Dashboard Web

## V0.7

-   OTA

## V0.8

-   MQTT

## V1.0

-   Proyecto terminado

------------------------------------------------------------------------

# Requisitos de calidad

-   C++17
-   PlatformIO
-   ESP32-S3
-   clang-format
-   Doxygen
-   GitHub Actions
-   Tests para lógica
-   Sin variables globales
-   Código desacoplado

------------------------------------------------------------------------

# Definición de éxito

El sistema deberá:

-   Arrancar automáticamente.
-   Medir continuamente.
-   Mostrar datos en pantalla.
-   Acumular Wh.
-   Publicar por WiFi.
-   Ser ampliable sin modificar la lógica de negocio.
