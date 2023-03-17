#include "adc.h"

extern ADC_HandleTypeDef hadc1;


volatile uint16_t adc_buffer[2][11];
volatile uint8_t adc_buffer_idx = 0;
volatile uint8_t adc_ready = 1;


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
  /* This is called after the conversion is completed */
  adc_buffer_idx = (adc_buffer_idx + 1) % 2;
  adc_ready = 1;
}


// Start an ADC conversion if the ADC is ready
void update_adc(void) {
    if(adc_ready){
      uint8_t new_idx = (adc_buffer_idx + 1) % 2;

    // Start ADC conversion
      HAL_ADC_Start_DMA(&hadc1, (uint32_t *) adc_buffer[new_idx], 11);
      adc_ready = 0;
    }
}

// Read an ADC value
uint16_t read_adc(uint8_t channel) {
    return adc_buffer[adc_buffer_idx][channel];
}

// Read the mcu temperature value
uint16_t read_temperature(void){
    uint16_t raw_value = read_adc(10);
}