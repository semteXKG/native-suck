#pragma once

#include <string.h>
#include <constants.h>
#include <stdbool.h>

typedef struct find_result_ {
    char* found_at;
    bool partial_match;
} find_result;

find_result find_match(char* buffer, size_t len, char* search_string);
void printBuffer(char* buffer, size_t start, size_t count);
