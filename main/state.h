#pragma once

#include "constants.h"



void setOpMode(enum OpMode opMode);
void setStatus(enum MachineStatus status);
const char* getOpModeString();
const char* getStatusString();

measurements_t getMeasurements();
void setMeasurements(measurements_t measurements);
