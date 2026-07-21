#include "Nokia5110Display.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

static constexpr int kButtonThreshold = 3600;  // ADC < this = button pressed
static constexpr uint32_t kDebounceMs = 200;

Nokia5110Display::Nokia5110Display(uint8_t clk, uint8_t din, uint8_t dc, uint8_t ce,
                                   uint8_t rst, uint8_t buttonAdcPin)
    : lcd_(new Adafruit_PCD8544(clk, din, dc, ce, rst)), buttonAdcPin_(buttonAdcPin) {}

Nokia5110Display::~Nokia5110Display() { delete lcd_; }

void Nokia5110Display::begin() {
    lcd_->begin();
    lcd_->setContrast(50);
    lcd_->clearDisplay();
    lcd_->display();
    pinMode(buttonAdcPin_, INPUT);
}

bool Nokia5110Display::buttonPressed() {
    int adc = analogRead(buttonAdcPin_);
    bool pressed = (adc < kButtonThreshold);
    uint32_t now = millis();
    if (pressed && !lastPressedState_ && (now - lastDebounceMs_ > kDebounceMs)) {
        lastDebounceMs_ = now;
        lastPressedState_ = true;
        return true;
    }
    if (!pressed) lastPressedState_ = false;
    return false;
}

void Nokia5110Display::show(Page page, const AppState& state) {
    lcd_->clearDisplay();
    lcd_->setTextSize(1);
    lcd_->setTextColor(BLACK);
    switch (page) {
        case Page::PowerMetrics:
            showPowerMetrics(state);
            break;
        case Page::Energy:
            showEnergy(state);
            break;
        case Page::RpmBattery:
            showRpmBattery(state);
            break;
    }
    lcd_->display();
}

void Nokia5110Display::showPowerMetrics(const AppState& s) {
    lcd_->setCursor(0, 0);
    lcd_->print("V:");
    lcd_->print(s.measurement.voltage, 1);
    lcd_->print(" V");
    lcd_->setCursor(0, 10);
    lcd_->print("I:");
    lcd_->print(s.measurement.current, 2);
    lcd_->print(" A");
    lcd_->setCursor(0, 20);
    lcd_->print("P:");
    lcd_->print(s.measurement.power, 1);
    lcd_->print(" W");
}

void Nokia5110Display::showEnergy(const AppState& s) {
    lcd_->setCursor(0, 0);
    lcd_->print("Hoy:");
    lcd_->setCursor(0, 10);
    lcd_->print(s.energy.whToday, 1);
    lcd_->print(" Wh");
    lcd_->setCursor(0, 25);
    lcd_->print("Total:");
    lcd_->setCursor(0, 35);
    lcd_->print(s.energy.whTotal / 1000.0f, 2);
    lcd_->print(" kWh");
}

void Nokia5110Display::showRpmBattery(const AppState& s) {
    lcd_->setCursor(0, 0);
    lcd_->print("RPM:");
    lcd_->print(s.rpm.value);
    lcd_->setCursor(0, 15);
    lcd_->print("Bat:");
    lcd_->print(s.battery.percent);
    lcd_->print("%");
    if (s.battery.charging) {
        lcd_->setCursor(0, 30);
        lcd_->print("CHG");
    }
}
