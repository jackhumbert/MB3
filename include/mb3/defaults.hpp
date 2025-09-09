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

#ifndef MB3_CAN_TX
#define MB3_CAN_TX -1
#endif

#ifndef MB3_CAN_RX
#define MB3_CAN_RX -1
#endif