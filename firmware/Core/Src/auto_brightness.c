#include "auto_brightness.h"

// Filtered brightness value. Changes much slower than actual brightness.
uint32_t brightness_filtered;
// Set to 1 once brightness has been updated once.
uint8_t initialized = 0;

void update_brightness(uint16_t adc_light_val){
    // If this is the first call, bypass the filter. This keeps the filtered value from starting at 0 (Full brightness)
    if(initialized){
        brightness_filtered = (brightness_filtered >> 16) * BRIGHTNESS_FILTER_FACTOR + adc_light_val * (65536 - BRIGHTNESS_FILTER_FACTOR);
    } else { // Not initialized yet
        brightness_filtered = adc_light_val * 65536;
        initialized = 1;
    }
    
    // Calculate brightness factor based on min/max brightness values.
    // This will be 0-65535, where 0 is dimmest and 65535 is brightest.
    int32_t brightness_fac = 65536 - ((brightness_filtered >> 16) - MAX_BRIGHTNESS_VALUE) * 65536 / MIN_BRIGHTNESS_VALUE;
    if(brightness_fac < 0) brightness = MIN_BRIGHTNESS;
    else if(brightness_fac > 65535) brightness = MAX_BRIGHTNESS;
    else {
        // Gamma correction for better dimming
        // Apply twice: Once cancels out the curve of the photodiode, the second time corrects for apparent brightness
        brightness_fac = gamma_16(gamma_16(brightness_fac));

        // Scale by max brightness
        brightness = (brightness_fac * MAX_BRIGHTNESS) >> 16;
        // Clamp to min brightness
        if(brightness < MIN_BRIGHTNESS) brightness = MIN_BRIGHTNESS;
    }
}