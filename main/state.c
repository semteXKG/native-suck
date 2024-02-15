#include "state.h"
#include "esp_log.h"

static const char* LOG = "STATE";

enum MachineStatus status = OFF;
enum OpMode opMode = LOCAL;
measurements_t measurements = {
    .humidity = 0,
    .pm25Level = 0,
    .temperature = 0,
    .lastHumidityUpdate = 0,
    .lastPM25Update = 0,
    .lastTempUpdate = 0
};

void setOpMode(enum OpMode newOpMode) {
    if(newOpMode == opMode) {
        return;
    }
    ESP_LOGI(LOG, "Switching OpMode: %s -> %s", OpModeStr[opMode], OpModeStr[newOpMode]);
    opMode = newOpMode;
}

void setStatus(enum MachineStatus newStatus) {
    if(status == newStatus) {
        return;
    }
    
    ESP_LOGI(LOG, "Switching Status: %s -> %s", MachineStatusStr[status], MachineStatusStr[newStatus]);
    status = newStatus;
}

const char* getOpModeString() {
    return OpModeStr[opMode];
}
const char* getStatusString() {
    return MachineStatusStr[status];
}

measurements_t getMeasurements() {
    return measurements;
}

void setMeasurements(measurements_t new_measurments) {
    measurements = new_measurments;
}
