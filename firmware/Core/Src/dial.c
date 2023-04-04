#include "dial.h"

const uint16_t position_values[NUM_POSITIONS] = {
    20, 65, 110, 155, 200, 245, 285, 330, 370, 410, 445, 550
};

uint8_t current_position[NUM_DIALS] = {0,0};
uint8_t new_position[NUM_DIALS] = {0,0};
uint32_t deglitch_counter[NUM_DIALS] = {0,0};


uint8_t convert_position(uint16_t adc_val){
    uint8_t pos;
    for(pos = 0; adc_val > position_values[pos] && pos < NUM_POSITIONS; ++pos) {
        
    }
    return pos;
}

void update_dial_state(uint16_t dial0, int16_t dial1){
    uint16_t adc_values[NUM_DIALS] = {dial0, dial1};
    // Loop through each dial
    for(uint8_t dial=0; dial<NUM_DIALS; ++dial){
        // Convert ADC value to position number
        uint8_t dial_pos = convert_position(adc_values[dial]);

        // Check if it's in the same position. If so do nothing.
        if(dial_pos != current_position[dial]){
            // Dial position does not match car state.
            // Check if it matches the previous new position:
            if(dial_pos == new_position[dial]){
                // Does match, increment counter 
                deglitch_counter[dial]++;
                // Once counter reaches DEGLITCH_COUNT set the state and reset the counter
                if(deglitch_counter[dial] > DEGLITCH_COUNT) {
                    deglitch_counter[dial] = 0;
                    current_position[dial] = dial_pos;
                }
            } else {
                // Dial position does not match previous new value, reset counter and set new counter.
                deglitch_counter[dial] = 0;
                new_position[dial] = dial_pos;
            }
        }
    }
}


uint8_t get_dial(enum DialIdx_t dial){
    return current_position[dial];
}