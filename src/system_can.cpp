#include <mb3/defaults.hpp>
#include <mb3/system_can.hpp>
#include <mb3/can.hpp>
#include <config.hpp>

#ifndef MB3_CAN_TX
#define MB3_CAN_TX GPIO_NUM_NC
#endif

#ifndef MB3_CAN_RX
#define MB3_CAN_RX GPIO_NUM_NC
#endif

bool CAN::hasRX = false;

static CanLog message = {0};
static uint64_t timer;

bool CAN::setup_impl() {

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();

    twai_general_config_t g_config = {
        // .mode = TWAI_MODE_LISTEN_ONLY, 
        .mode = TWAI_MODE_NORMAL,
        // .mode = TWAI_MODE_NO_ACK,
        .tx_io = MB3_CAN_TX, 
        .rx_io = MB3_CAN_RX,       
        .clkout_io = TWAI_IO_UNUSED, 
        .bus_off_io = TWAI_IO_UNUSED,
        .tx_queue_len = 500, 
        .rx_queue_len = 500, // affects PSRAM
        .alerts_enabled = TWAI_ALERT_ALL,  
        .clkout_divider = 0,          
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };

    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    auto res = twai_driver_install(&g_config, &t_config, &f_config);
    if (res == ESP_OK) {
        log_d("Driver installed");
    } else {
        log_d("Failed to install driver: %d", res);
        return false;
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        log_d("Driver started");
    } else {
        log_d("Failed to start driver");
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
        // log_d("Message received: 0x%02X", message.frame.identifier);

        bool known = false;
        for (auto & can_msg_type : CanFrameTypes::types) {
            if (can_msg_type->id() == message.frame.identifier) {
                known = true;
                // log_d("%s message received (%d)", can_msg_type->name().c_str(), message.frame.data_length_code);
                // memcpy(can_msg_type->data(), message.frame.data, message.frame.data_length_code);
                memcpy(can_msg_type->data(), message.frame.data, can_msg_type->size());
                can_msg_type->update();
                // log_d("%s (%u): %02X %02X %02X %02X %02X %02X %02X %02X", can_msg_type->name().c_str(), can_msg_type->size(),
                //     message.frame.data[0], 
                //     message.frame.data[1], 
                //     message.frame.data[2], 
                //     message.frame.data[3], 
                //     message.frame.data[4], 
                //     message.frame.data[5], 
                //     message.frame.data[6], 
                //     message.frame.data[7]
                // );
            }
        }
        if (!known) {
            log_d("Unknown CAN message: %08X (%u): %02X %02X %02X %02X %02X %02X %02X %02X", message.frame.identifier, message.frame.data_length_code,
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
        MB3_LOG("ESP twai_receive error: %d", res);
    }

    if ((millis() - timer) > 1000) {
        timer = millis();

        uint32_t alerts_triggered;
        if (twai_read_alerts(&alerts_triggered, 0) == ESP_OK) {
            // if (alerts_triggered & 0x00000001) MB3_LOG("TWAI_ALERT_TX_IDLE");
            // if (alerts_triggered & 0x00000002) MB3_LOG("TWAI_ALERT_TX_SUCCESS");
            // if (alerts_triggered & 0x00000004) MB3_LOG("TWAI_ALERT_RX_DATA");
            if (alerts_triggered & 0x00000008) MB3_LOG("TWAI_ALERT_BELOW_ERR_WARN");
            if (alerts_triggered & 0x00000010) MB3_LOG("TWAI_ALERT_ERR_ACTIVE");
            if (alerts_triggered & 0x00000020) MB3_LOG("TWAI_ALERT_RECOVERY_IN_PROGRESS");
            if (alerts_triggered & 0x00000040) MB3_LOG("TWAI_ALERT_BUS_RECOVERED");
            if (alerts_triggered & 0x00000080) MB3_LOG("TWAI_ALERT_ARB_LOST");
            if (alerts_triggered & 0x00000100) MB3_LOG("TWAI_ALERT_ABOVE_ERR_WARN");
            if (alerts_triggered & 0x00000200) MB3_LOG("TWAI_ALERT_BUS_ERROR");
            if (alerts_triggered & 0x00000400) MB3_LOG("TWAI_ALERT_TX_FAILED");
            if (alerts_triggered & 0x00000800) MB3_LOG("TWAI_ALERT_RX_QUEUE_FULL");
            if (alerts_triggered & 0x00001000) MB3_LOG("TWAI_ALERT_ERR_PASS");
            if (alerts_triggered & 0x00002000) MB3_LOG("TWAI_ALERT_BUS_OFF");
            if (alerts_triggered & 0x00004000) MB3_LOG("TWAI_ALERT_RX_FIFO_OVERRUN");
            if (alerts_triggered & 0x00008000) MB3_LOG("TWAI_ALERT_TX_RETRIED");
            if (alerts_triggered & 0x00010000) MB3_LOG("TWAI_ALERT_PERIPH_RESET");
        }
        twai_status_info_t status;
        if (twai_get_status_info(&status) == ESP_OK) {
            if (status.state == TWAI_STATE_STOPPED) log_d("* TWAI_STATE_STOPPED");
            // if (status.state == TWAI_STATE_RUNNING) log_d("* TWAI_STATE_RUNNING");
            if (status.state == TWAI_STATE_BUS_OFF) log_d("* TWAI_STATE_BUS_OFF");
            if (status.state == TWAI_STATE_RECOVERING) log_d("* TWAI_STATE_RECOVERING");
            if (status.msgs_to_tx) log_d("* msgs_to_tx: %d", status.msgs_to_tx);
            if (status.msgs_to_rx) log_d("* msgs_to_rx: %d", status.msgs_to_rx);
            if (status.tx_error_counter) log_d("* tx_error_counter: %d", status.tx_error_counter);
            if (status.rx_error_counter) log_d("* rx_error_counter: %d", status.rx_error_counter);
            if (status.tx_failed_count) log_d("* tx_failed_count: %d", status.tx_failed_count);
            if (status.rx_missed_count) log_d("* rx_missed_count: %d", status.rx_missed_count);
            if (status.rx_overrun_count) log_d("* rx_overrun_count: %d", status.rx_overrun_count);
            if (status.arb_lost_count) log_d("* arb_lost_count: %d", status.arb_lost_count);
            if (status.bus_error_count) log_d("* bus_error_count: %d", status.bus_error_count);
        }

        // test code to send a frame that was received
        // message.frame.identifier = 0x154;
        // auto wres = twai_transmit(&message.frame, pdMS_TO_TICKS(1000));
        // if (wres == ESP_OK) {
        //     log_d("Can frame sent");
        // } else {
        //     if (wres == ESP_ERR_INVALID_ARG) MB3_LOG("ESP_ERR_INVALID_ARG");
        //     if (wres == ESP_ERR_TIMEOUT) MB3_LOG("ESP_ERR_TIMEOUT");
        //     if (wres == ESP_FAIL) MB3_LOG("ESP_FAIL");
        //     if (wres == ESP_ERR_INVALID_STATE) MB3_LOG("ESP_ERR_INVALID_STATE");
        //     if (wres == ESP_ERR_NOT_SUPPORTED) MB3_LOG("ESP_ERR_NOT_SUPPORTED");
        // }
    }

}