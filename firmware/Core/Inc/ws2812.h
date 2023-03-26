#ifndef WS2812_H
#define WS2812_H

#include "main.h"

#define NUM_STRIPS 8
#define STRIPS_PER_GROUP 4
#define STRIP_LENGTH 16

#define NUM_GROUPS 2

struct __packed color_struct_t {
    uint8_t b;
    uint8_t r;
    uint8_t g;
    uint8_t _; // unused byte for packing
};

union color_t {
    uint32_t u32;
    struct color_struct_t color;
};

void write_strip_8x(union color_t buffer[STRIPS_PER_GROUP * NUM_GROUPS][STRIP_LENGTH]);

#endif