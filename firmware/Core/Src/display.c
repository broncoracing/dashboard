#include "display.h"
#include "main.h"
#include <string.h>

union color_t display_frame_buffer[NUM_STRIPS][STRIP_LENGTH];

uint8_t brightness = DEFAULT_BRIGHTNESS;

const uint16_t group_1_pins[4] = {LED_0_Pin, LED_1_Pin, LED_2_Pin, LED_3_Pin};
const uint16_t group_2_pins[4] = {LED_4_Pin, LED_5_Pin, LED_6_Pin, LED_7_Pin};

#define DIGIT_XY(dx, dy) \
    {dx,dy},{dx-7,dy+11},{dx-11,dy+11},{dx-16,dy+19},{dx-16,dy+14},{dx-16,dy+8},{dx-16,dy+3},{dx-11,dy+1},{dx-7,dy+1},{dx-4,dy+3},{dx-4,dy+8},{dx-4,dy+14},{dx-4,dy+19},{dx-7,dy+22},{dx-11,dy+22},{0,0}

const struct xy_t pixel_pos[NUM_STRIPS][STRIP_LENGTH] = {
    {{0,0}, {0,13}, {0,26}, {0,38}, {13,50}, {23,50}, {33,50}, {43,50}, {53,50}, {63,50}, {73,50}, {83,50}, {93,50}, {103,50}, {113,50}, {123,50}},
    {{24,32}, {32,32}, {40,32}, {48,32}, {56,32}, {64,32}, {72,32}, {80,32}, {88,32}, {96,32}, {104,32}, {112,32}, {136,38}, {136,26}, {136,13}, {136,0}},
    {DIGIT_XY(31, 9)},
    {DIGIT_XY(49, 9)},
    {DIGIT_XY(67, 9)},
    {DIGIT_XY(85, 9)},
    {DIGIT_XY(103, 9)},
    {DIGIT_XY(121, 9)},
};

void update_display(void) {
    write_strip_4x(display_frame_buffer, LED_0_GPIO_Port, group_1_pins);
    write_strip_4x(display_frame_buffer[4], LED_4_GPIO_Port, group_2_pins);
}

// Set display to all zeros
void wipe_display(void) {
    memset(display_frame_buffer, 0, sizeof(display_frame_buffer));
}


void shade_display(display_shader_t shader) {
    for(uint8_t strip = 0; strip < NUM_STRIPS; ++strip){
        for(uint8_t pixel = 0; pixel < STRIP_LENGTH; ++pixel){
            display_frame_buffer[strip][pixel] = shader(pixel_pos[strip][pixel]);
        }
    }
}


union color_t flash(union color_t color, uint16_t period_ms, uint16_t length_ms){
  if(HAL_GetTick() % period_ms < length_ms) return color;
  else return COLOR_BLACK;
}

union color_t hsv(uint8_t h, uint8_t s, uint8_t v) {
    struct color_struct_t rgb;

    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        rgb.r = v;
        rgb.g = v;
        rgb.b = v;
        union color_t output_col = {.color=rgb};
        return output_col;
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            rgb.r = v;
            rgb.g = t;
            rgb.b = p;
            break;
        case 1:
            rgb.r = q;
            rgb.g = v;
            rgb.b = p;
            break;
        case 2:
            rgb.r = p;
            rgb.g = v;
            rgb.b = t;
            break;
        case 3:
            rgb.r = p;
            rgb.g = q;
            rgb.b = v;
            break;
        case 4:
            rgb.r = t;
            rgb.g = p;
            rgb.b = v;
            break;
        default:
            rgb.r = v;
            rgb.g = p;
            rgb.b = q;
            break;
    }

    union color_t output_col = {.color=rgb};
    return output_col;
}

// gamma = 2.00 steps = 256 range = 0-255
const uint8_t gamma_lut[256] = {
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
     1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   4,   4,
     4,   4,   5,   5,   5,   5,   6,   6,   6,   7,   7,   7,   8,   8,   8,   9,
     9,   9,  10,  10,  11,  11,  11,  12,  12,  13,  13,  14,  14,  15,  15,  16,
    16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  23,  23,  24,  24,
    25,  26,  26,  27,  28,  28,  29,  30,  30,  31,  32,  32,  33,  34,  35,  35,
    36,  37,  38,  38,  39,  40,  41,  42,  42,  43,  44,  45,  46,  47,  47,  48,
    49,  50,  51,  52,  53,  54,  55,  56,  56,  57,  58,  59,  60,  61,  62,  63,
    64,  65,  66,  67,  68,  69,  70,  71,  73,  74,  75,  76,  77,  78,  79,  80,
    81,  82,  84,  85,  86,  87,  88,  89,  91,  92,  93,  94,  95,  97,  98,  99,
   100, 102, 103, 104, 105, 107, 108, 109, 111, 112, 113, 115, 116, 117, 119, 120,
   121, 123, 124, 126, 127, 128, 130, 131, 133, 134, 136, 137, 139, 140, 142, 143,
   145, 146, 148, 149, 151, 152, 154, 155, 157, 158, 160, 162, 163, 165, 166, 168,
   170, 171, 173, 175, 176, 178, 180, 181, 183, 185, 186, 188, 190, 192, 193, 195,
   197, 199, 200, 202, 204, 206, 207, 209, 211, 213, 215, 217, 218, 220, 222, 224,
   226, 228, 230, 232, 233, 235, 237, 239, 241, 243, 245, 247, 249, 251, 253, 255,
  };

union color_t gamma(union color_t color){
    union color_t output = {.color={
        .r=gamma_lut[color.color.r],
        .g=gamma_lut[color.color.g],
        .b=gamma_lut[color.color.b],
    }};
    return output;
}

uint16_t gamma_16(uint16_t val){
  uint8_t idx = val >> 8;
  uint8_t v2 = gamma_lut[idx];
  uint8_t v1;
  if(idx == 0){
    v1=0;
  } else {
    v1 = gamma_lut[idx - 1];
  }

  uint8_t fpart = val & 0xFF;
  return (v1 << 8) + fpart * (v2 - v1);
}

const uint8_t digit_lookup_table[16] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111, // 9
  0b01110111, // A
  0b01111100, // b
  0b00111001, // c
  0b01011110, // d
  0b01111001, // E
  0b01110001, // f
};

const uint8_t segment_idx_table[8][2]={
    {14, 13},
    {12, 11},
    {10, 9},
    {8, 7},
    {6, 5},
    {4, 3},
    {2, 1},
    {0, 0}
};

const uint8_t digit_dp = 0b10000000;
void write_digit(uint8_t value, enum DigitPosition digit, uint8_t dp, union color_t color) {
  uint8_t segments;
  if(value < sizeof(digit_lookup_table)/sizeof(digit_lookup_table[0])) {
    // Digit is valid
    // Read lookup table and set
    segments = digit_lookup_table[value];
  } else {
    // Invalid digit, turn off
    segments = 0x00; 
  }
  if(dp) {
    segments |= digit_dp;
  }

  for(uint8_t segment = 0; segment < 8; ++segment){
    if((segments >> segment) & 1){
        display_frame_buffer[digit][segment_idx_table[segment][0]] = color;
        display_frame_buffer[digit][segment_idx_table[segment][1]] = color;
    }
  }
}


void write_char(uint8_t character, enum DigitPosition digit, uint8_t dp, union color_t color) {
  if(dp) {
    character |= digit_dp;
  }
  for(uint8_t segment = 0; segment < 8; ++segment){
    if((character >> segment) & 1){
        display_frame_buffer[digit][segment_idx_table[segment][0]] = color;
        display_frame_buffer[digit][segment_idx_table[segment][1]] = color;
    }
  }
}

void write_int(uint32_t value, enum DigitPosition startDigit, uint8_t length, union color_t color) {
  if(startDigit + length > MAX_DIGIT + 1) {
    // Invalid size to write
    Error_Handler();
    return;
  }
  for(int8_t i = length - 1; i >= 0; --i) {
    uint8_t digit_val = value % 10;
    write_digit(digit_val, startDigit + i, 0, color);
    value = value / 10;
    if(value == 0) break;
  }
}

void write_fixedpoint(uint32_t value, enum DigitPosition startDigit, uint8_t length, uint8_t decimals, union color_t color){
  if(startDigit + length > MAX_DIGIT + 1) {
    // Invalid size to write
    Error_Handler();
    return;
  }
  for(int8_t i = length - 1; i >= 0; --i) {
    uint8_t digit_val = value % 10;
    write_digit(digit_val, startDigit + i, 0, color);
    value = value / 10;
    if(value == 0 && i < length - decimals) break;
  }
  // write dp
  write_char(0b00000000, startDigit + length - decimals - 1, 1, color);
}

void startup_animation(void) {
}


void write_shift_lights(uint8_t start, uint8_t length, union color_t color) {
  if(start >= 12) Error_Handler();
  if(start + length > 12) Error_Handler();
  
  for(uint8_t i = 0; i < length; ++i) {
    display_frame_buffer[0][i+start+4] = color;
  }
}

void write_tach(uint8_t start, uint8_t length, union color_t color) {
  if(start >= 12) Error_Handler();
  if(start + length > 12) Error_Handler();

  for(uint8_t i = 0; i < length; ++i) {
    display_frame_buffer[1][i+start] = color;
  }
}

void write_status(uint8_t idx, union color_t color) {
  if(idx >= 8){
    Error_Handler();
  }

  if(idx < 4){
    display_frame_buffer[0][idx] = color;
  } else {
    display_frame_buffer[1][idx + 8] = color;
  }
}
