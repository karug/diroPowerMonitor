#pragma once
#include "../../domain/ports/IStorage.h"

class LittleFsRepository : public IStorage {
public:
    bool begin();
    void save(const EnergyStatistics& stats) override;
    EnergyStatistics load() override;

private:
    bool mounted_{false};
};
