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
