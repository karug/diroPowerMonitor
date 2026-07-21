#include "HallSensor.h"
#include <Arduino.h>

volatile uint32_t HallSensor::pulseCount_ = 0;

void IRAM_ATTR HallSensor::isr() {
    pulseCount_++;
}

HallSensor::HallSensor(uint8_t pin, uint8_t pulsesPerRev)
    : pin_(pin), pulsesPerRev_(pulsesPerRev == 0 ? 1 : pulsesPerRev) {}

void HallSensor::begin() {
    pinMode(pin_, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin_), isr, FALLING);
    windowStartMs_ = millis();
}

Rpm HallSensor::read() {
    uint32_t now = millis();
    uint32_t elapsed = now - windowStartMs_;
    if (elapsed < 3000) return lastRpm_;

    // Atomic read + reset
    noInterrupts();
    uint32_t count = pulseCount_;
    pulseCount_ = 0;
    interrupts();

    uint32_t revolutions = count / pulsesPerRev_;
    lastRpm_ = calc_.calculate(revolutions, elapsed);
    windowStartMs_ = now;
    return lastRpm_;
}
