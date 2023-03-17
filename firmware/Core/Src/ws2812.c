#include "ws2812.h"

extern TIM_HandleTypeDef htim2;
extern DMA_HandleTypeDef hdma_tim2_up;
extern DMA_HandleTypeDef hdma_tim2_ch1;
extern DMA_HandleTypeDef hdma_tim2_ch2_ch4;

#define BITS_PER_PIXEL 24
#define DMA_LENGTH ((STRIP_LENGTH + 1) * BITS_PER_PIXEL)

const uint16_t group_pins[2][4] = {
    {LED_0_Pin, LED_1_Pin, LED_2_Pin, LED_3_Pin},
    {LED_4_Pin, LED_5_Pin, LED_6_Pin, LED_7_Pin}
};

GPIO_TypeDef group_ports[2] = {{LED_0_GPIO_Port, LED_4_GPIO_Port}}; // double braces to appease GCC

// half-word to be written to bit set register to set all current LED pins to high
// or to the bit reset register to set all to low
uint16_t pin_set[NUM_GROUPS];
// Mask for each bit to set necessary pins to zero.
// bssr0[1][2] contains the mask to write the zeros for the third bit of the second pixel color.
// There is one extra LED worth at the end because apparently WS2812s like that.
uint16_t bssr0[NUM_GROUPS][STRIP_LENGTH + 1][BITS_PER_PIXEL];

// Start DMA for a group of four strips.
void ws2812_dma_start(uint16_t *bssr0_buffer, uint8_t group_idx) {
    GPIO_TypeDef *port = &group_ports[group_idx];
    // Make sure timer is stopped so we don't prematurely do anything
    __HAL_TIM_DISABLE(&htim2);
    // Set the timer to just before reset, so that the reset event gets called first
    __HAL_TIM_SET_COUNTER(&htim2, __HAL_TIM_GET_AUTORELOAD(&htim2) - 1);

    // Make sure timer DMA triggers are enabled
    __HAL_TIM_ENABLE_DMA(&htim2, TIM_DMA_UPDATE);
    __HAL_TIM_ENABLE_DMA(&htim2, TIM_DMA_CC1);
    __HAL_TIM_ENABLE_DMA(&htim2, TIM_DMA_CC2);

    // Clear DMA transfer complete flags. This way we can tell when the DMA transfer is complete because they will be set again.
    __HAL_DMA_CLEAR_FLAG(&hdma_tim2_up, __HAL_DMA_GET_TC_FLAG_INDEX(&hdma_tim2_up));
    __HAL_DMA_CLEAR_FLAG(&hdma_tim2_ch1, __HAL_DMA_GET_TC_FLAG_INDEX(&hdma_tim2_ch1));
    __HAL_DMA_CLEAR_FLAG(&hdma_tim2_ch2_ch4, __HAL_DMA_GET_TC_FLAG_INDEX(&hdma_tim2_ch2_ch4));

    // Start DMA transfers
    HAL_DMA_Start(&hdma_tim2_up,      (uint32_t) &pin_set[group_idx], (uint32_t) &port->BSRR, DMA_LENGTH); // Set all to high
    HAL_DMA_Start(&hdma_tim2_ch1,     (uint32_t) bssr0_buffer,        (uint32_t) &port->BRR,  DMA_LENGTH); // set zeros to low
    HAL_DMA_Start(&hdma_tim2_ch2_ch4, (uint32_t) &pin_set[group_idx], (uint32_t) &port->BRR,  DMA_LENGTH); // set all to low

    // Start timer. Now data will start flowing.
    __HAL_TIM_ENABLE(&htim2);
}


void ws2812_dma_wait() {
    // Wait for DMA to finish
    while(!__HAL_DMA_GET_TC_FLAG_INDEX(&hdma_tim2_ch2_ch4));
    
    // Now that DMA is finished, clean up after ourselves.
    // Stop timer
    __HAL_TIM_DISABLE(&htim2);
}

void ws2812_init(void){
    for(uint8_t group_idx = 0; group_idx < NUM_GROUPS; ++group_idx){
        // Create BSRR mask to write all pins to 1 or 0
        pin_set[group_idx] = 0;
        for(uint8_t i = 0; i < STRIPS_PER_GROUP; ++i) pin_set[group_idx] |= group_pins[group_idx][i];
    }
}

// Directly write to 4x GPIO outputs to control 4 strips of LEDs simultaneously.
void write_strip_4x(union color_t buffer[STRIPS_PER_GROUP][STRIP_LENGTH], uint8_t group_idx){
    const uint16_t *pin_mask = group_pins[group_idx]; 

    // Loop through pixels (from start to end of strip)
    for(uint8_t i = 0; i < STRIP_LENGTH; ++i){
        // Fetch and invert the colors for each strip (because we care about the zeros, not the ones)
        uint32_t c0_inv = ~(buffer[0][i].u32);
        uint32_t c1_inv = ~(buffer[1][i].u32);
        uint32_t c2_inv = ~(buffer[2][i].u32);
        uint32_t c3_inv = ~(buffer[3][i].u32);

        // Set up bssr0 for each bit
        for(uint8_t bit = 0; bit < 24; bit++){
            // create bssr0 for this bit
            uint16_t bssr_val = 0;

            // Branchless unrolled loop for performance^2, 22uS delay (within datasheet spec)
            bssr_val |= pin_mask[0] * ((c0_inv >> bit) & 1);
            bssr_val |= pin_mask[1] * ((c1_inv >> bit) & 1);
            bssr_val |= pin_mask[2] * ((c2_inv >> bit) & 1);
            bssr_val |= pin_mask[3] * ((c3_inv >> bit) & 1);

            // Write to bssr buffer
            bssr0[group_idx][i][bit] = bssr_val;

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
    }
}

// Blocking function to write to all 8 LED strips.
// Since it uses DMA for the actual writing, it never disables interrupts.
void write_strip_8x(union color_t buffer[STRIPS_PER_GROUP * NUM_GROUPS][STRIP_LENGTH]){
    // Generate data for first group
    write_strip_4x(buffer, 0);
    // Start DMA for first group
    ws2812_dma_start(&bssr0[0][0][0], 0);
    // Generate data for second group
    write_strip_4x(&buffer[4][0], 1);
    // If the DMA transaction is still happening, wait for it to finish.
    ws2812_dma_wait();
    // Clear the port to zero (should already be zero)
    group_ports[0].BRR = pin_set[0];
    //start DMA for second group
    ws2812_dma_start(&bssr0[1][0][0], 1);
    // Wait for DMA transaction to finish
    ws2812_dma_wait();
    // Clear the port to zero (should already be zero)
    group_ports[0].BRR = pin_set[0];
}
