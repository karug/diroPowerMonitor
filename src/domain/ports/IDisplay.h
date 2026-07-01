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
