#pragma once
#include <cstdint>

struct HistoryEntry {
    uint32_t timestampMs{0};
    float    powerW{0.0f};
};

class HistoryRepository {
public:
    bool begin();
    void append(const HistoryEntry& entry);

private:
    bool mounted_{false};
};
