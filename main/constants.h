#pragma once

#include <inttypes.h>

static const char *OpModeStr[] = { "LOCAL", "SENSOR", "API" };
static const char *MachineStatusStr[] = { "OFF", "ON_LOW", "ON_HIGH" };

typedef struct measurments_ {
    short temperature;
    short humidity;
    float pm25Level;

    int64_t lastTempUpdate;
    int64_t lastHumidityUpdate;
    int64_t lastPM25Update;
} measurements_t;


enum OpMode {
    LOCAL,
    SENSOR,
    API
};

enum MachineStatus {
    OFF,
    ON_LOW,
    ON_HIGH
};