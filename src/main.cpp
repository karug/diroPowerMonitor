#include <Arduino.h>
#include "Logger.h"

void setup() {
    Serial.begin(115200);
    delay(500);
    LOG_INFO("Wind Energy Monitor v0.1 starting");
}

void loop() {
    delay(1000);
}
