#include <config.hpp>
#include <mb3/defaults.hpp>
#include <mb3/system_can.hpp>
#include <mb3/can.hpp>
#include <config.hpp>
#include MB3_CAN_LOG_INCLUDE

bool CAN::hasRX = false;

static CanLog message = {0};
static uint64_t timer;

bool CAN::setup_impl() {

    twai_timing_config_t t_config = MB3_CAN_TIMING;

    twai_general_config_t g_config = {
        .mode = MB3_CAN_MODE,
        .tx_io = MB3_CAN_TX, 
        .rx_io = MB3_CAN_RX,       
        .clkout_io = TWAI_IO_UNUSED, 
        .bus_off_io = TWAI_IO_UNUSED,
        .tx_queue_len = 500, 
        .rx_queue_len = 500, // affects PSRAM
        // .alerts_enabled = TWAI_ALERT_ALL, 
        .alerts_enabled = TWAI_ALERT_ALL & ~TWAI_ALERT_TX_IDLE & ~TWAI_ALERT_TX_SUCCESS & ~TWAI_ALERT_RX_DATA,  
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

    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        MB3_LOG_NICE("[CAN] Driver started");
    } else {
        MB3_LOG_NICE("[CAN] Failed to start driver");
        return false;
    }

    timer = millis();
    // message.frame.identifier = 0x154;
    // message.frame.self = 1;
    // message.frame.data_length_code = 8;

    return true;

}

void CAN::task_impl() {
    esp_err_t res = ESP_OK;
    while (res = twai_receive(&message.frame, 0), res == ESP_OK) {
        hasRX = true;
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

    if (res != ESP_ERR_TIMEOUT) {
        MB3_LOG_NICE("[CAN] ESP twai_receive error: %d", res);
    }

    if ((millis() - timer) > 5000) {
        timer = millis();

        uint32_t alerts_triggered;
        if (esp_err_t alert_status = twai_read_alerts(&alerts_triggered, 0); alert_status == ESP_OK) {
            if (alerts_triggered & 0x00000001) MB3_LOG_NICE("[CAN] TWAI_ALERT_TX_IDLE");
            if (alerts_triggered & 0x00000002) MB3_LOG_NICE("[CAN] TWAI_ALERT_TX_SUCCESS");
            if (alerts_triggered & 0x00000004) MB3_LOG_NICE("[CAN] TWAI_ALERT_RX_DATA");
            if (alerts_triggered & 0x00000008) MB3_LOG_NICE("[CAN] TWAI_ALERT_BELOW_ERR_WARN");
            if (alerts_triggered & 0x00000010) MB3_LOG_NICE("[CAN] TWAI_ALERT_ERR_ACTIVE");
            if (alerts_triggered & 0x00000020) MB3_LOG_NICE("[CAN] TWAI_ALERT_RECOVERY_IN_PROGRESS");
            if (alerts_triggered & 0x00000040) MB3_LOG_NICE("[CAN] TWAI_ALERT_BUS_RECOVERED");
            if (alerts_triggered & 0x00000080) MB3_LOG_NICE("[CAN] TWAI_ALERT_ARB_LOST");
            if (alerts_triggered & 0x00000100) MB3_LOG_NICE("[CAN] TWAI_ALERT_ABOVE_ERR_WARN");
            if (alerts_triggered & 0x00000200) MB3_LOG_NICE("[CAN] TWAI_ALERT_BUS_ERROR");
            if (alerts_triggered & 0x00000400) MB3_LOG_NICE("[CAN] TWAI_ALERT_TX_FAILED");
            if (alerts_triggered & 0x00000800) MB3_LOG_NICE("[CAN] TWAI_ALERT_RX_QUEUE_FULL");
            if (alerts_triggered & 0x00001000) MB3_LOG_NICE("[CAN] TWAI_ALERT_ERR_PASS");
            if (alerts_triggered & 0x00002000) MB3_LOG_NICE("[CAN] TWAI_ALERT_BUS_OFF");
            if (alerts_triggered & 0x00004000) MB3_LOG_NICE("[CAN] TWAI_ALERT_RX_FIFO_OVERRUN");
            if (alerts_triggered & 0x00008000) MB3_LOG_NICE("[CAN] TWAI_ALERT_TX_RETRIED");
            if (alerts_triggered & 0x00010000) MB3_LOG_NICE("[CAN] TWAI_ALERT_PERIPH_RESET");
        } else {
            // if (alert_status == ESP_OK) MB3_LOG_NICE("[CAN] Alerts read");
            // if (alert_status == ESP_ERR_TIMEOUT) MB3_LOG_NICE("[CAN] Timed out waiting for alerts");
            if (alert_status == ESP_ERR_INVALID_ARG) MB3_LOG_NICE("[CAN] Arguments are invalid");
            if (alert_status == ESP_ERR_INVALID_STATE) MB3_LOG_NICE("[CAN] TWAI driver is not installed");
        }
        if (alerts_triggered) {
            twai_status_info_t status;
            if (esp_err_t r = twai_get_status_info(&status); r == ESP_OK) {
                if (status.state == TWAI_STATE_STOPPED) MB3_LOG_NICE("[CAN] * TWAI_STATE_STOPPED");
                // if (status.state == TWAI_STATE_RUNNING) MB3_LOG_NICE("[CAN] * TWAI_STATE_RUNNING");
                if (status.state == TWAI_STATE_BUS_OFF) {
                    MB3_LOG_NICE("[CAN] * TWAI_STATE_BUS_OFF");
                    twai_initiate_recovery();
                }
                if (status.state == TWAI_STATE_RECOVERING) MB3_LOG_NICE("[CAN] * TWAI_STATE_RECOVERING");
                if (status.msgs_to_tx) MB3_LOG_NICE("[CAN] * msgs_to_tx: %d", status.msgs_to_tx);
                if (status.msgs_to_rx) MB3_LOG_NICE("[CAN] * msgs_to_rx: %d", status.msgs_to_rx);
                if (status.tx_error_counter) MB3_LOG_NICE("[CAN] * tx_error_counter: %d", status.tx_error_counter);
                if (status.rx_error_counter) MB3_LOG_NICE("[CAN] * rx_error_counter: %d", status.rx_error_counter);
                if (status.tx_failed_count) MB3_LOG_NICE("[CAN] * tx_failed_count: %d", status.tx_failed_count);
                if (status.rx_missed_count) MB3_LOG_NICE("[CAN] * rx_missed_count: %d", status.rx_missed_count);
                if (status.rx_overrun_count) MB3_LOG_NICE("[CAN] * rx_overrun_count: %d", status.rx_overrun_count);
                if (status.arb_lost_count) MB3_LOG_NICE("[CAN] * arb_lost_count: %d", status.arb_lost_count);
                if (status.bus_error_count) MB3_LOG_NICE("[CAN] * bus_error_count: %d", status.bus_error_count);
            } else {
                if (r == ESP_ERR_INVALID_ARG) MB3_LOG_NICE("[CAN] Arguments are invalid");
                if (r == ESP_ERR_INVALID_STATE) MB3_LOG_NICE("[CAN] TWAI driver is not installed");
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