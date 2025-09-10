#pragma once

#if ARDUINO

#include <Arduino.h>
#define MB3_LOG(...) log_printf(__VA_ARGS__)

#else

#include "esp_log.h"
#define MB3_LOG(...) ESP_LOGD("MB3", __VA_ARGS__)
#define micros() esp_timer_get_time()
#define millis() (esp_time_get_time() / 1000)

#endif

#define MB3_LOG_NICE(format, ...) MB3_LOG("[%6u][D] " format "\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL) __VA_OPT__(,) __VA_ARGS__);

#ifndef MB3_CAN_TX
#define MB3_CAN_TX GPIO_NUM_NC
#endif

#ifndef MB3_CAN_RX
#define MB3_CAN_RX GPIO_NUM_NC
#endif

#ifndef MB3_CAN_TIMING
#define MB3_CAN_TIMING TWAI_TIMING_CONFIG_250KBITS()
#endif

// TWAI_MODE_NORMAL
// TWAI_MODE_LISTEN_ONLY
// TWAI_MODE_NO_ACK
#ifndef MB3_CAN_MODE
#define MB3_CAN_MODE TWAI_MODE_NORMAL
#endif