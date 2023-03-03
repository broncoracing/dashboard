#include "ws2812.h"

// Directly write to 4x GPIO outputs to control 4 strips of LEDs simultaneously.
void write_strip_4x(union color_t buffer[STRIPS_PER_GROUP][STRIP_LENGTH], GPIO_TypeDef *port, const uint16_t pin_mask[STRIPS_PER_GROUP]){
    // Create BSRR mask to write all pins to 1
    uint16_t pin_set = 0;
    for(uint8_t i = 0; i < STRIPS_PER_GROUP; ++i) pin_set |= pin_mask[i];
    
    // BSRR mask to write all pins to zero
    uint32_t pin_reset = pin_set << 16;

    // Mask for each bit to set necessary pins to zero.
    // bssr0[0] contains the mask to write the zeros for the first bit of the pixel color.
    static uint16_t bssr0[24];

    // Loop through pixels (from start to end of strip)
    __disable_irq(); // no interrupts allowed while writing
    for(uint8_t i = 0; i < STRIP_LENGTH; ++i){
        // Fetch and invert the colors for each strip (because we care about the zeros, not the ones)
        uint32_t c0_inv = ~(buffer[0][i].u32);
        uint32_t c1_inv = ~(buffer[1][i].u32);
        uint32_t c2_inv = ~(buffer[2][i].u32);
        uint32_t c3_inv = ~(buffer[3][i].u32);

        // Set up bssr0 for each bit
        for(uint8_t bit = 0; bit < 24; bit++){
            // Reset bssr0 for this bit
            bssr0[bit] = 0;

            // Branchless unrolled loop for performance^2, 22uS delay (within datasheet spec)
            bssr0[bit] |= pin_mask[0] * ((c0_inv >> bit) & 1);
            bssr0[bit] |= pin_mask[1] * ((c1_inv >> bit) & 1);
            bssr0[bit] |= pin_mask[2] * ((c2_inv >> bit) & 1);
            bssr0[bit] |= pin_mask[3] * ((c3_inv >> bit) & 1);

            // // Unrolled loop for performance, 30uS delay (on the edge of datasheet spec)
            // uint32_t bit_mask = 1 << bit;
            // if(!(buffer[0][i].u32 & bit_mask)){
            //     bssr0[bit] |= pin_mask[0];
            // }
            // if(!(buffer[1][i].u32 & bit_mask)){
            //     bssr0[bit] |= pin_mask[1];
            // }
            // if(!(buffer[2][i].u32 & bit_mask)){
            //     bssr0[bit] |= pin_mask[2];
            // }
            // if(!(buffer[3][i].u32 & bit_mask)){
            //     bssr0[bit] |= pin_mask[3];
            // }

            // // Loop version, 50uS delay (Longer than recommended by datasheet)
            // // Loop through each strip
            // for(uint8_t strip = 0; strip < NUM_STRIPS; ++strip){
            //     uint32_t color = buffer[strip][i].u32; // Get individual pixel color
            //     // Add bit to bssr0 for 0 bits
            //     if(!((color >> bit) & 1)){ // If bit is zero
            //         bssr0[bit] |= pin_mask[strip]; // write the reset bit for this strip
            //     }
            // }
        }
        

        for(int8_t bit = 23; bit >= 0; bit--){ // loop through bits from 23 down to zero
            port->BSRR = pin_set; // set to high

            for(uint8_t j=0; j<DELAY_0H; ++j) // delay
                __NOP();
            
            port->BSRR = bssr0[bit] << 16; // 0 goes low

            for(uint8_t j=0; j<DELAY_1H; ++j) // delay
                __NOP();

            
            port->BSRR = pin_reset; // 1 goes low

            for(uint8_t j=0; j<DELAY_L; ++j) // delay
                __NOP();
        }
    }
    __enable_irq();
}