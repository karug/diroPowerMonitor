#pragma once
#include <cstdint>
#include "../../domain/ports/IDisplay.h"

class Adafruit_PCD8544;

class Nokia5110Display : public IDisplay {
public:
    Nokia5110Display(uint8_t clk, uint8_t din, uint8_t dc, uint8_t ce, uint8_t rst,
                     uint8_t buttonAdcPin);
    ~Nokia5110Display() override;

    void begin();
    void show(Page page, const AppState& state) override;
    bool buttonPressed();

private:
    void showPowerMetrics(const AppState& state);
    void showEnergy(const AppState& state);
    void showRpmBattery(const AppState& state);

    Adafruit_PCD8544* lcd_;
    uint8_t buttonAdcPin_;
    uint32_t lastDebounceMs_{0};
    bool lastPressedState_{false};
};
