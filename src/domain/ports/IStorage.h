#pragma once
#include "../entities/EnergyStatistics.h"

class IStorage {
public:
    virtual void save(const EnergyStatistics& stats) = 0;
    virtual EnergyStatistics load() = 0;
    virtual ~IStorage() = default;
};
