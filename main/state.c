#include "state.h"
#include "esp_log.h"
#include "relay_controller.h"
#include "nvs_store.h"
#include "esp_timer.h"

static const char* LOG = "STATE";

enum MachineStatus status;
enum OpMode opMode;
char ip_addr[16];
int64_t switched_on_timestamp;
int32_t preasure_diff;


measurements_t measurements = {
    .humidity = 0,
    .pm25Level = 0,
    .temperature = 0,
    .lastHumidityUpdate = 0,
    .lastPM25Update = 0,
    .lastTempUpdate = 0
};

void setOpMode(enum OpMode newOpMode) {
    if (newOpMode == opMode) {
        return;
    }
    ESP_LOGI(LOG, "Switching OpMode: %s -> %s", OpModeStr[opMode], OpModeStr[newOpMode]);
    opMode = newOpMode;
    store_save_last_op_mode(opMode);
}

void setStatus(enum MachineStatus newStatus) {
    if (status == newStatus) {
        return;
    }
    
    ESP_LOGI(LOG, "Switching Status: %s -> %s", MachineStatusStr[status], MachineStatusStr[newStatus]);
    
    if (newStatus == ON_HIGH || newStatus == ON_LOW) {
        switched_on_timestamp = esp_timer_get_time() / 1000;
    }
    handleStateChange(newStatus);
    
    status = newStatus;
}

const char* getOpModeString() {
    return OpModeStr[opMode];
}
const char* getStatusString() {
    return MachineStatusStr[status];
}


enum MachineStatus ac400_get_status() {
    return status;
}

const char* get_ip_address() {
    return ip_addr;
}

void set_ip_address(char* ip_address) {
    strcpy(ip_addr, ip_address);
}

measurements_t getMeasurements() {
    return measurements;
}

void update_state_for_pm_25(float air_quality) {
    int64_t currentTimestamp = esp_timer_get_time() / 1000;
    if (air_quality >= get_high_limit()) {
        setStatus(ON_HIGH);
    } else if(air_quality >= get_low_limit()) {
        setStatus(ON_LOW);
    } else if (currentTimestamp - get_afterrun_seconds()*1000 > switched_on_timestamp) {
        setStatus(OFF);
    }
}

void setMeasurements(measurements_t new_measurments) {
    measurements = new_measurments;
    if(opMode == AUTO) {
        update_state_for_pm_25(measurements.pm25Level);
    }
}

void advanceOpMode() {
    setOpMode((opMode+1) % 3);
}

void advanceStatus() {
    setStatus((status+1) % 3);
    handleStateChange(status);
}

int32_t ac400_get_preasure_diff() {
    return preasure_diff;
}

void ac400_set_preasure_diff(int32_t new_preasure_diff) {
    preasure_diff = new_preasure_diff;
}
