#ifndef DISPLAY_H
#define DISPLAY_H

#include "main.h"
#include "ws2812.h"

#define MIN_BRIGHTNESS 5
#define DEFAULT_BRIGHTNESS 25
#define MAX_BRIGHTNESS 128


// Stores data that will be written to LED segments
extern union color_t display_frame_buffer[NUM_STRIPS][STRIP_LENGTH];

// pixel brighness scale factor
extern uint8_t brightness;


// Pixel positions
struct xy_t {
    uint8_t x;
    uint8_t y;
};

extern const struct xy_t pixel_pos[NUM_STRIPS][STRIP_LENGTH];

// Color type
typedef union color_t(*display_shader_t)(struct xy_t);

// Pre-defined colors
#define COLOR_BLACK ((union color_t) {})
#define COLOR_RED ((union color_t) {.color={.r=brightness}})
#define COLOR_VERY_RED ((union color_t) {.color={.r=255}}) // obnoxiously bright red, regardless of brightness
#define COLOR_ORANGE (hsv(13, 255, brightness))
#define COLOR_GOLD (hsv(20, 250, brightness))
#define COLOR_YELLOW ((union color_t) {.color={.r=brightness, .g=brightness}})
#define COLOR_GREEN ((union color_t) {.color={.g=brightness}})
#define COLOR_CYAN ((union color_t) {.color={.g=brightness, .b=brightness}})
#define COLOR_BLUE ((union color_t) {.color={.b=brightness}})
#define COLOR_MAGENTA ((union color_t) {.color={.b=brightness, .r=brightness}})
#define COLOR_WHITE ((union color_t) {.color={.g=brightness, .b=brightness, .r=brightness,}})
#define COLOR_PAINFUL ((union color_t) {.color={.r=255,.g=255,.b=255}}) // ouch!

union color_t flash(union color_t color, uint16_t period_ms, uint16_t length_ms);

// Create color from hue, saturation, and value (brighness)
union color_t hsv(uint8_t h, uint8_t s, uint8_t v);

// Gamma correction function
union color_t gamma_color(union color_t color);

uint16_t gamma_16(uint16_t val);

// Fill every pixel with a color function of position
void shade_display(display_shader_t shader);

// 7 segment characters

enum Char7Seg {
  a_7SEG=   0b0111111111100111,
  A_7SEG=   0b0111111001111110,
  b_7SEG=   0b0000011111111110,
  c_7SEG=   0b0000000111100110,
  C_7SEG=   0b0110000111111000,
  d_7SEG=   0b0001111111100110,
  e_7SEG=   0b0111100111111110,
  E_7SEG=   0b0110000111111110,
  f_7SEG=   0b0110000001111100,
  F_7SEG=   0b0110000001111110,
  g_7SEG=   0b0111111111011110,
  G_7SEG=   0b0111011111111010,
  h_7SEG=   0b0000011001111110,
  H_7SEG=   0b0001111001111110,
  i_7SEG=   0b0000111000000000,
  j_7SEG=   0b0001111111100000,
  k_7SEG=   0b0000111001111110,
  l_7SEG=   0b0000000011111000,
  L_7SEG=   0b0000000111111000,
  n_7SEG=   0b0000011001100110,
  o_7SEG=   0b0000011111100110,
  P_7SEG=   0b0111100001111110,
  q_7SEG=   0b0111111000011110,
  r_7SEG=   0b0000000001100110,
  S_7SEG=   0b0110011110011110,
  t_7SEG=   0b0000000011111100,
  u_7SEG=   0b0000011111100000,
  U_7SEG=   0b0001111111111000,
  y_7SEG=   0b0001111000011110,
  Z_7SEG=   0b0111100111100110,

  dash_7SEG=0b0000000000000110,
  underscore_7SEG = 
            0b0000000110000000,
  bang_7SEG=0b0001101000000000,
  bangbang_7SEG=0b0001101001011000,
};

extern const uint8_t digit_dp;

enum DigitPosition {
  DIGIT_0=2,
  DIGIT_1=3,
  DIGIT_2=4,
  DIGIT_3=5,
  DIGIT_4=6,
  DIGIT_5=7,
  MAX_DIGIT=7
};

void write_digit(uint8_t value, enum DigitPosition digit, uint8_t dp, union color_t color);

void write_char(uint16_t character, enum DigitPosition digit, uint8_t dp, union color_t color);

void write_shift_lights(uint8_t start, uint8_t length, union color_t color);

void write_tach(uint8_t start, uint8_t length, union color_t color);

void write_status(uint8_t idx, union color_t color);

void write_int(uint32_t value, enum DigitPosition startDigit, uint8_t length, union color_t color);
void write_fixedpoint(uint32_t value, enum DigitPosition startDigit, uint8_t length, uint8_t decimals, union color_t color);

void wipe_display(void);

void update_display(void);

#endif