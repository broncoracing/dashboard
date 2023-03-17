#ifndef ADC_H
#define ADC_H

#include "main.h"

// Start an ADC conversion if the ADC is ready
void update_adc(void);

// Read an ADC value
uint16_t read_adc(uint8_t channel);

// Read the mcu temperature value
uint16_t read_temperature(void);

#endif