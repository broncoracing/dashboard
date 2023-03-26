#ifndef AUTO_BRIGHTNESS_H
#define AUTO_BRIGHTNESS_H

#include "display.h"

// Control the speed of the brightness filter. Higher = slower. 0=instant update, 65535=slowest update
#define BRIGHTNESS_FILTER_FACTOR 65000

// Min/max ADC values from photodiode
#define MIN_BRIGHTNESS_VALUE 3300
#define MAX_BRIGHTNESS_VALUE 0

// Call with brightness value to update brightness
void update_brightness(uint16_t adc_light_val);

#endif