#include "HistoryRepository.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

static constexpr char kHistoryPath[] = "/history.json";
static constexpr uint16_t kMaxEntries = 1440;

bool HistoryRepository::begin() {
    if (!LittleFS.begin(false)) return false;
    mounted_ = true;
    return true;
}

void HistoryRepository::append(const HistoryEntry& entry) {
    if (!mounted_) return;
    JsonDocument doc;
    JsonArray arr;

    File f = LittleFS.open(kHistoryPath, "r");
    if (f) {
        DeserializationError err = deserializeJson(doc, f);
        f.close();
        if (!err && doc.is<JsonArray>()) {
            arr = doc.as<JsonArray>();
        }
    }

    if (!doc.is<JsonArray>()) {
        doc.to<JsonArray>();
        arr = doc.as<JsonArray>();
    }

    while (arr.size() >= kMaxEntries) {
        arr.remove(0);
    }

    JsonObject obj = arr.add<JsonObject>();
    obj["t"] = entry.timestampMs;
    obj["p"] = entry.powerW;

    File fw = LittleFS.open(kHistoryPath, "w");
    if (!fw) return;
    serializeJson(doc, fw);
    fw.close();
}
