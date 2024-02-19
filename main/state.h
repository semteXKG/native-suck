#pragma once

#include "constants.h"

void setOpMode(enum OpMode opMode);
void setStatus(enum MachineStatus status);
const char* getOpModeString();
const char* getStatusString();
void advanceOpMode();
void advanceStatus();

measurements_t getMeasurements();
void setMeasurements(measurements_t measurements);
const char* get_ip_address();
void set_ip_address(char* ip_address);
