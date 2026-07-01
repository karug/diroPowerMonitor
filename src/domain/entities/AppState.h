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
