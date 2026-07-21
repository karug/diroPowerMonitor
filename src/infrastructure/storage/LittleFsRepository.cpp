#include "LittleFsRepository.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../../domain/entities/EnergyStatistics.h"

static constexpr char kEnergyPath[] = "/energy.json";

bool LittleFsRepository::begin() {
    if (!LittleFS.begin(true)) {  // true = format if mount fails
        return false;
    }
    mounted_ = true;
    return true;
}

void LittleFsRepository::save(const EnergyStatistics& stats) {
    if (!mounted_) return;
    JsonDocument doc;
    doc["whToday"] = stats.whToday;
    doc["whTotal"] = stats.whTotal;
    File f = LittleFS.open(kEnergyPath, "w");
    if (!f) return;
    serializeJson(doc, f);
    f.close();
}

EnergyStatistics LittleFsRepository::load() {
    EnergyStatistics stats;
    if (!mounted_) return stats;
    File f = LittleFS.open(kEnergyPath, "r");
    if (!f) return stats;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return stats;
    stats.whToday = doc["whToday"] | 0.0f;
    stats.whTotal = doc["whTotal"] | 0.0f;
    return stats;
}
