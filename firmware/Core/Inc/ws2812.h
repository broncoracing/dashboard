#ifndef WS2812_H
#define WS2812_H

#include "main.h"

#define DELAY_0H 2
#define DELAY_1H 6
#define DELAY_L  8


#define NUM_STRIPS 8
#define STRIPS_PER_GROUP 4
#define STRIP_LENGTH 16

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

void write_strip_4x(union color_t buffer[STRIPS_PER_GROUP][STRIP_LENGTH], GPIO_TypeDef *port, const uint16_t pin_mask[STRIPS_PER_GROUP]);

#endif