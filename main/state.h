#pragma once

#include "constants.h"

void setOpMode(enum OpMode opMode);
void setStatus(enum MachineStatus status);
const char* getOpModeString();
const char* getStatusString();
enum MachineStatus ac400_get_status();
void advanceOpMode();
void advanceStatus();

measurements_t getMeasurements();
void setMeasurements(measurements_t measurements);
const char* get_ip_address();
void set_ip_address(char* ip_address);

int32_t ac400_get_preasure_diff();
void ac400_set_preasure_diff(int32_t presaure_diff);
