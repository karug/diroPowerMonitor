#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#define LOG_INFO(msg)  Serial.println("[INFO]  " + String(msg))
#define LOG_WARN(msg)  Serial.println("[WARN]  " + String(msg))
#define LOG_ERROR(msg) Serial.println("[ERROR] " + String(msg))
#else
#include <cstdio>
#define LOG_INFO(msg)  printf("[INFO]  %s\n", msg)
#define LOG_WARN(msg)  printf("[WARN]  %s\n", msg)
#define LOG_ERROR(msg) printf("[ERROR] %s\n", msg)
#endif
