#ifndef WS2812_H
#define WS2812_H

#include "main.h"

#define DELAY_0H 2
#define DELAY_1H 6
#define DELAY_L  10

struct __packed color_struct_t {
    uint8_t g;
    uint8_t r;
    uint8_t b;
    uint8_t w;
};

union color_t {
    uint32_t u32;
    struct color_struct_t color;
};

void write_strip(union color_t *buffer, uint8_t length, GPIO_TypeDef *port, uint16_t pin_mask);

#endif