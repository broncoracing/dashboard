#include "ws2812.h"

void write_strip(union color_t *buffer, uint8_t length, GPIO_TypeDef *port, uint16_t pin_mask){
    uint32_t pin_reset = pin_mask << 16;

    uint32_t bssr0[24];

    for(uint8_t i = 0; i < length; ++i){
        // get color from buffer
        uint32_t color = buffer[i].u32;

        // Pre-calculate BSRR for 0 bits
        for(uint8_t bit = 0; bit < 24; bit++){
            if((color >> bit) & 1){
                bssr0[bit] = 0; // stays high
            } else {
                bssr0[bit] = pin_reset; // goes to zero
            }
        }

        for(int8_t bit = 23; bit >= 0; bit--){ // loop through bits and write
            port->BSRR = pin_mask; // set to high

            for(uint8_t j=0; j<DELAY_0H; ++j) // delay
                __NOP();
            
            port->BSRR = bssr0[bit]; // 0 goes low

            for(uint8_t j=0; j<DELAY_1H; ++j) // delay
                __NOP();

            
            port->BSRR = pin_reset; // 1 goes low

            for(uint8_t j=0; j<DELAY_L; ++j) // delay
                __NOP();
        }
    }
}