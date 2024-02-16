#include "state.h"
#include "esp_log.h"
#include "relay_controller.h"

static const char* LOG = "STATE";

enum MachineStatus status;
enum OpMode opMode;
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
    handleStateChange(newStatus);
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

void update_state_for_pm_25(float air_quality) {
    if (air_quality > 40) {
        setStatus(ON_HIGH);
    } else if(air_quality >  20) {
        setStatus(ON_LOW);
    } else 
    setStatus(OFF);
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