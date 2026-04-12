#include <config.hpp>
#include <mb3/defaults.hpp>
#include <mb3/system_can.hpp>
#include <mb3/can.hpp>
#include <config.hpp>
#include MB3_CAN_LOG_INCLUDE

static CanLog message = {0};
static uint64_t timer;
static uint32_t last_recovery_time = 0;
static const uint32_t RECOVERY_DEBOUNCE_MS = 1000; // Minimum 1 second between recoveries to handle WiFi interference
static uint32_t last_hard_reset_time = 0;
static const uint32_t HARD_RESET_DEBOUNCE_MS = 10000; // Minimum 10 seconds between hard resets to avoid thrashing
static uint32_t recovery_attempt_count = 0;
static const uint32_t RECOVERY_ATTEMPTS_THRESHOLD = 5; // Trigger hard reset after 5 failed soft recoveries
static uint32_t consecutive_bus_errors = 0;
static const uint32_t BUS_ERROR_THRESHOLD = 10; // Trigger recovery after 10 consecutive BUS_ERROR alerts

// Startup grace period: suppress hard resets during boot while WiFi initializes
// WiFi RF interference during esp_wifi_init/start causes transient BUS_ERRORs that
// should be handled with soft recovery only. After 30s, normal hard reset logic applies.
static uint32_t startup_time_ms = 0;
static const uint32_t STARTUP_GRACE_MS = 30000;
static uint32_t last_startup_log_ms = 0;
static const uint32_t STARTUP_LOG_RATE_MS = 3000; // Rate-limit startup error logs

// Bus idle detection: when no other nodes are active (e.g. vehicle off),
// the controller enters error passive with no received frames.
// Recovery is pointless in this state — just wait quietly for the bus to come alive.
static uint32_t last_rx_time_ms = 0;
static bool bus_idle = false;
static const uint32_t BUS_IDLE_TIMEOUT_MS = 10000;  // 10s with no RX = idle
static uint32_t last_idle_log_ms = 0;
static const uint32_t IDLE_LOG_INTERVAL_MS = 60000; // Log idle status once per minute

bool CAN::perform_hard_reset() {
    MB3_LOG_NICE("[CAN] *** HARD RESET: Uninstalling and reinstalling TWAI driver ***");
    
    // Stop the driver first
    esp_err_t stop_res = twai_stop();
    if (stop_res != ESP_OK && stop_res != ESP_ERR_INVALID_STATE) {
        MB3_LOG_NICE("[CAN] Hard reset: twai_stop() returned %d", stop_res);
    }
    
    // Give it a moment to stop
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Uninstall the driver
    esp_err_t uninstall_res = twai_driver_uninstall();
    if (uninstall_res != ESP_OK) {
        MB3_LOG_NICE("[CAN] Hard reset: twai_driver_uninstall() failed: %d", uninstall_res);
        return false;
    }
    
    MB3_LOG_NICE("[CAN] Hard reset: Driver uninstalled, waiting 200ms before reinstall");
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Reinstall the driver with same config as in setup_impl
    twai_timing_config_t t_config = MB3_CAN_TIMING;
    twai_general_config_t g_config = {
        .mode = MB3_CAN_MODE,
        .tx_io = MB3_CAN_TX, 
        .rx_io = MB3_CAN_RX,       
        .clkout_io = TWAI_IO_UNUSED, 
        .bus_off_io = TWAI_IO_UNUSED,
        .tx_queue_len = MB3_CAN_TX_QUEUE_LEN, 
        .rx_queue_len = MB3_CAN_RX_QUEUE_LEN,
        .alerts_enabled = TWAI_ALERT_ALL & ~TWAI_ALERT_TX_IDLE & ~TWAI_ALERT_TX_SUCCESS & ~TWAI_ALERT_RX_DATA & ~TWAI_ALERT_RX_FIFO_OVERRUN,  
        .clkout_divider = 0,          
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    esp_err_t install_res = twai_driver_install(&g_config, &t_config, &f_config);
    if (install_res != ESP_OK) {
        MB3_LOG_NICE("[CAN] Hard reset: twai_driver_install() failed: %d", install_res);
        return false;
    }
    
    MB3_LOG_NICE("[CAN] Hard reset: Driver reinstalled, starting...");
    
    esp_err_t start_res = twai_start();
    if (start_res != ESP_OK) {
        MB3_LOG_NICE("[CAN] Hard reset: twai_start() failed: %d", start_res);
        return false;
    }
    
    MB3_LOG_NICE("[CAN] *** HARD RESET COMPLETE: Driver back online ***");
    recovery_attempt_count = 0;
    last_hard_reset_time = millis();
    last_recovery_time = millis();
    return true;
}

bool CAN::setup_impl() {

    twai_timing_config_t t_config = MB3_CAN_TIMING;

    twai_general_config_t g_config = {
        .mode = MB3_CAN_MODE,
        .tx_io = MB3_CAN_TX, 
        .rx_io = MB3_CAN_RX,       
        .clkout_io = TWAI_IO_UNUSED, 
        .bus_off_io = TWAI_IO_UNUSED,
        .tx_queue_len = MB3_CAN_TX_QUEUE_LEN, 
        .rx_queue_len = MB3_CAN_RX_QUEUE_LEN, // affects PSRAM
        // .alerts_enabled = TWAI_ALERT_ALL, 
        .alerts_enabled = TWAI_ALERT_ALL & ~TWAI_ALERT_TX_IDLE & ~TWAI_ALERT_TX_SUCCESS & ~TWAI_ALERT_RX_DATA & ~TWAI_ALERT_RX_FIFO_OVERRUN,  
        .clkout_divider = 0,          
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };

    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    auto res = twai_driver_install(&g_config, &t_config, &f_config);
    if (res == ESP_OK) {
        MB3_LOG_NICE("[CAN] Driver installed");
    } else {
        MB3_LOG_NICE("[CAN] Failed to install driver: %d", res);
        return false;
    }

    // gpio_hold_en(MB3_CAN_TX);
    // gpio_hold_en(MB3_CAN_RX);
    // gpio_deep_sleep_hold_en();

    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        MB3_LOG_NICE("[CAN] Driver started");
    } else {
        MB3_LOG_NICE("[CAN] Failed to start driver");
        return false;
    }

    timer = millis();
    startup_time_ms = millis();      // Grace period starts from CAN init
    last_hard_reset_time = millis(); // Treat boot as a recent hard reset so debounce starts now
    last_rx_time_ms = millis();      // Assume bus might be active at start
    // message.frame.identifier = 0x154;
    // message.frame.self = 1;
    // message.frame.data_length_code = 8;

    return true;

}

void CAN::task_impl() {
    esp_err_t res = ESP_OK;

    // Check for CAN alerts immediately on each task cycle for fast error recovery
    // But debounce recoveries to prevent constant cycling
    uint32_t current_time = millis();
    bool in_startup_grace = (current_time - startup_time_ms) < STARTUP_GRACE_MS;

    // Bus idle detection: if no frames received for a while, the bus has no active nodes
    if (!bus_idle && !in_startup_grace && (current_time - last_rx_time_ms) > BUS_IDLE_TIMEOUT_MS) {
        bus_idle = true;
        MB3_LOG_NICE("[CAN] Bus idle - no frames received for %ds, suppressing recovery", BUS_IDLE_TIMEOUT_MS / 1000);
        last_idle_log_ms = current_time;
        recovery_attempt_count = 0;
        consecutive_bus_errors = 0;
    }

    uint32_t alerts_triggered;
    if (twai_read_alerts(&alerts_triggered, 0) == ESP_OK) {
        // When bus is idle, skip all recovery logic — just consume and discard alerts
        if (bus_idle) {
            // Still check for bus recovery (car turned on)
            if (alerts_triggered & TWAI_ALERT_BUS_RECOVERED) {
                MB3_LOG_NICE("[CAN] Bus recovered from idle");
                bus_idle = false;
                recovery_attempt_count = 0;
                consecutive_bus_errors = 0;
            }
            goto read_frames; // Skip all recovery logic
        }

        // During startup grace, WiFi RF causes a flood of BUS_ERRORs.
        // Skip recovery logic — just consume alerts and read any frames that arrive.
        if (in_startup_grace) {
            goto read_frames;
        }

        bool should_recover = false;
        const char* recovery_reason = nullptr;
        
        // Check for error counters maxing out (indicates driver is stuck)
        twai_status_info_t status_check;
        if (twai_get_status_info(&status_check) == ESP_OK) {
            if ((status_check.tx_error_counter >= 255 || status_check.rx_error_counter >= 255) &&
                (current_time - last_hard_reset_time) > HARD_RESET_DEBOUNCE_MS) {
                if (!in_startup_grace) {
                    MB3_LOG_NICE("[CAN] ERROR COUNTERS MAXED (TX:%d RX:%d) - triggering hard reset", status_check.tx_error_counter, status_check.rx_error_counter);
                    perform_hard_reset();
                    return;
                } else {
                    if (current_time - last_startup_log_ms > STARTUP_LOG_RATE_MS) {
                        last_startup_log_ms = current_time;
                        MB3_LOG_NICE("[CAN] Startup: error counters high (TX:%d RX:%d) - soft recovery only during boot", status_check.tx_error_counter, status_check.rx_error_counter);
                    }
                }
            }
        }
        
        // Natural bus recovery resets the attempt counter
        if (alerts_triggered & TWAI_ALERT_BUS_RECOVERED) {
            MB3_LOG_NICE("[CAN] Bus recovered naturally - resetting recovery counters");
            recovery_attempt_count = 0;
            consecutive_bus_errors = 0;
        }

        // Track repeated BUS_ERROR alerts (WiFi interference pattern)
        if (alerts_triggered & TWAI_ALERT_BUS_ERROR) {
            consecutive_bus_errors++;
            if (consecutive_bus_errors >= BUS_ERROR_THRESHOLD && 
                (current_time - last_recovery_time) > RECOVERY_DEBOUNCE_MS) {
                if (in_startup_grace) {
                    // Rate-limit logs during startup to avoid spam from WiFi RF interference
                    if (current_time - last_startup_log_ms > STARTUP_LOG_RATE_MS) {
                        last_startup_log_ms = current_time;
                        MB3_LOG_NICE("[CAN] Startup BUS_ERROR noise (%d consecutive) - WiFi init interference, soft recovery only", consecutive_bus_errors);
                    }
                } else {
                    MB3_LOG_NICE("[CAN] BUS_ERROR escalating (%d consecutive) - initiating recovery", consecutive_bus_errors);
                }
                should_recover = true;
                recovery_reason = "ESCALATING_BUS_ERROR";
                consecutive_bus_errors = 0;
            }
        } else {
            consecutive_bus_errors = 0; // Reset on other alerts
        }
        
        // Only check for critical states that require immediate action
        if (alerts_triggered & TWAI_ALERT_BUS_OFF) {
            should_recover = true;
            recovery_reason = "BUS_OFF";
            consecutive_bus_errors = 0;
        } else if (alerts_triggered & TWAI_ALERT_ERR_PASS) {
            // Only recover if we haven't recovered recently (debouncing)
            if (current_time - last_recovery_time > RECOVERY_DEBOUNCE_MS) {
                should_recover = true;
                recovery_reason = "ERR_PASS";
            }
            consecutive_bus_errors = 0;
        }
        
        if (should_recover) {
            twai_status_info_t status;
            if (twai_get_status_info(&status) == ESP_OK) {
                // For escalating BUS_ERROR from WiFi, be more aggressive - go straight to hard reset
                if (strcmp(recovery_reason, "ESCALATING_BUS_ERROR") == 0) {
                    if (!in_startup_grace && recovery_attempt_count >= 2 && 
                        (current_time - last_hard_reset_time) > HARD_RESET_DEBOUNCE_MS) {
                        MB3_LOG_NICE("[CAN] Escalating BUS_ERROR after %d soft attempts - triggering hard reset", recovery_attempt_count);
                        perform_hard_reset();
                        return;
                    }
                }
                
                if (status.state == TWAI_STATE_BUS_OFF) {
                    MB3_LOG_NICE("[CAN] RECOVERY: %s - Initiating recovery from BUS_OFF (TX:%d RX:%d)", recovery_reason, status.tx_error_counter, status.rx_error_counter);
                    twai_initiate_recovery();
                    recovery_attempt_count++;
                    last_recovery_time = current_time;
                } else if (status.state == TWAI_STATE_STOPPED) {
                    MB3_LOG_NICE("[CAN] RECOVERY: %s - Restarting TWAI from STOPPED state", recovery_reason);
                    twai_start();
                    recovery_attempt_count++;
                    last_recovery_time = current_time;
                } else if (status.state == TWAI_STATE_RECOVERING) {
                    MB3_LOG_NICE("[CAN] RECOVERY: %s - Already in recovery, error counters (TX:%d RX:%d)", recovery_reason, status.tx_error_counter, status.rx_error_counter);
                    recovery_attempt_count++;
                    last_recovery_time = current_time;
                } else if (status.state == TWAI_STATE_RUNNING) {
                    // Bus seems to be running but BUS_ERROR keeps triggering - this is WiFi noise
                    // Just log it but try to recover anyway
                    MB3_LOG_NICE("[CAN] RECOVERY: %s - Bus running but errors detected (TX:%d RX:%d)", recovery_reason, status.tx_error_counter, status.rx_error_counter);
                    recovery_attempt_count++;
                    last_recovery_time = current_time;
                }
                
                // Check if recovery attempts are escalating - trigger hard reset if stuck
                if (!in_startup_grace && recovery_attempt_count >= RECOVERY_ATTEMPTS_THRESHOLD && 
                    (current_time - last_hard_reset_time) > HARD_RESET_DEBOUNCE_MS) {
                    MB3_LOG_NICE("[CAN] Recovery stuck after %d attempts, triggering hard reset", recovery_attempt_count);
                    perform_hard_reset();
                }
            }
        }
    }

read_frames:
    while (res = twai_receive(&message.frame, 0), res == ESP_OK) {
        hasRX = true;
        last_rx_time_ms = current_time;
        if (bus_idle) {
            bus_idle = false;
            MB3_LOG_NICE("[CAN] Bus active - resuming normal operation");
            recovery_attempt_count = 0;
            consecutive_bus_errors = 0;
        }
        o_status.update();
        message.timestamp = esp_timer_get_time();
        // SDCard::log_can_message(&message);
        MB3_CAN_LOG(&message);
        // MB3_LOG_NICE("Message received: 0x%02X", message.frame.identifier);

        // bool known = false;
        // for (auto & can_msg_type : CanFrameTypes::types) {
        auto type = CanFrameTypes::types.find(message.frame.identifier);
        if (type != CanFrameTypes::types.end()) {
            auto can_msg_type = type->second;
            // if (can_msg_type->id() == message.frame.identifier) {
                // known = true;
                // MB3_LOG_NICE("%s message received (%d)", can_msg_type->name().c_str(), message.frame.data_length_code);
                // memcpy(can_msg_type->data(), message.frame.data, message.frame.data_length_code);
                memcpy(can_msg_type->data(), message.frame.data, can_msg_type->size());
                can_msg_type->update();
                // MB3_LOG_NICE("%s (%u): %02X %02X %02X %02X %02X %02X %02X %02X", can_msg_type->name().c_str(), can_msg_type->size(),
                //     message.frame.data[0], 
                //     message.frame.data[1], 
                //     message.frame.data[2], 
                //     message.frame.data[3], 
                //     message.frame.data[4], 
                //     message.frame.data[5], 
                //     message.frame.data[6], 
                //     message.frame.data[7]
                // );
            // }
        }
        else {
        // if (!known) {
            MB3_LOG_NICE("[CAN] Unknown message: %08X (%u): %02X %02X %02X %02X %02X %02X %02X %02X", message.frame.identifier, message.frame.data_length_code,
                message.frame.data[0], 
                message.frame.data[1], 
                message.frame.data[2], 
                message.frame.data[3], 
                message.frame.data[4], 
                message.frame.data[5], 
                message.frame.data[6], 
                message.frame.data[7]
            );
        }
    }

    if (res == ESP_ERR_TIMEOUT) {
        hasRX = false;
    } else {
        MB3_LOG_NICE("[CAN] ESP twai_receive error: %d", res);
    }

    if ((millis() - timer) > 5000) {
        timer = millis();

        // When bus is idle, log minimally — once per minute
        if (bus_idle) {
            if ((current_time - last_idle_log_ms) >= IDLE_LOG_INTERVAL_MS) {
                last_idle_log_ms = current_time;
                twai_status_info_t status;
                if (twai_get_status_info(&status) == ESP_OK) {
                    MB3_LOG_NICE("[CAN] Bus idle (TX:%d RX:%d, errors:%d)", status.tx_error_counter, status.rx_error_counter, status.bus_error_count);
                }
            }
        } else if (!in_startup_grace) {
            // Note: alerts were already consumed by twai_read_alerts() at the top of this function.
            // Read status directly - no second alert read needed.
            twai_status_info_t status;
            if (twai_get_status_info(&status) == ESP_OK) {
                if (status.state == TWAI_STATE_STOPPED) {
                    MB3_LOG_NICE("[CAN] * TWAI_STATE_STOPPED (periodic check)");
                } else if (status.state == TWAI_STATE_BUS_OFF) {
                    MB3_LOG_NICE("[CAN] * TWAI_STATE_BUS_OFF (periodic check)");
                } else if (status.state == TWAI_STATE_RECOVERING) {
                    MB3_LOG_NICE("[CAN] * TWAI_STATE_RECOVERING (periodic check)");
                }
                if (status.tx_error_counter > 50 || status.rx_error_counter > 50) {
                    MB3_LOG_NICE("[CAN] Error counters: TX=%d RX=%d", status.tx_error_counter, status.rx_error_counter);
                }
                if (status.bus_error_count > 1000) {
                    MB3_LOG_NICE("[CAN] High bus error count: %d", status.bus_error_count);
                }
            }
        }

        // test code to send a frame that was received
        // message.frame.identifier = 0x154;
        // auto wres = twai_transmit(&message.frame, pdMS_TO_TICKS(1000));
        // if (wres == ESP_OK) {
        //     MB3_LOG_NICE("Can frame sent");
        // } else {
        //     if (wres == ESP_ERR_INVALID_ARG) MB3_LOG_NICE("ESP_ERR_INVALID_ARG");
        //     if (wres == ESP_ERR_TIMEOUT) MB3_LOG_NICE("ESP_ERR_TIMEOUT");
        //     if (wres == ESP_FAIL) MB3_LOG_NICE("ESP_FAIL");
        //     if (wres == ESP_ERR_INVALID_STATE) MB3_LOG_NICE("ESP_ERR_INVALID_STATE");
        //     if (wres == ESP_ERR_NOT_SUPPORTED) MB3_LOG_NICE("ESP_ERR_NOT_SUPPORTED");
        // }
    }

}