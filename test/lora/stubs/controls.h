#pragma once
#include <stdint.h>
#define MAX_INS 64
#define IN 0
enum { N0_2_32counts, R10K_40_120DegC, Humidty, V0_5, CO2_PPM, TVOC_PPB, LUX, DB };
typedef struct {
    int8_t description[21], label[9];
    int32_t value;
    uint8_t digital_analog, range;
} Str_in_point;
typedef struct { Str_in_point *pin; } Str_points_ptr;
Str_points_ptr put_io_buf(int type, uint8_t index);
