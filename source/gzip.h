#pragma once
#include <stdint.h>

uint8_t* gzOpen(const char* path, int* size = nullptr);

void gzClose(uint8_t* data);
