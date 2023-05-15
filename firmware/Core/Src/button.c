#include "button.h"


struct ButtonState_t {
    uint8_t gpio_state;
    uint8_t state;
    uint8_t debounce_counter;
    int8_t edge; // 1 for rising, -1 for falling, 0 for no edge
} button_state[NUM_BTNS];

const struct ButtonPin {
    GPIO_TypeDef *port;
    uint16_t pin;
} button_pin[NUM_BTNS] = {
    {BTN0_GPIO_Port, BTN0_Pin},
    {BTN1_GPIO_Port, BTN1_Pin},
    {BTN2_GPIO_Port, BTN2_Pin},
    {BTN3_GPIO_Port, BTN3_Pin},
    {BTN4_GPIO_Port, BTN4_Pin},
    {BTN5_GPIO_Port, BTN5_Pin},
};

void init_buttons(void) {
    for(uint8_t i = 0; i < NUM_BTNS; ++i) {
        button_state[i].gpio_state = HAL_GPIO_ReadPin(button_pin[i].port, button_pin[i].pin);
        button_state[i].state = button_state[i].gpio_state;
        button_state[i].debounce_counter = 50;
    }
}

uint8_t get_button(enum ButtonInput_t channel){
    if(button_state[channel].gpio_state) return 0;
    else return 1;
}

// Updates buttons with debouncing
void update_buttons(void) {
    for(uint8_t i = 0; i < NUM_BTNS; ++i) {
        // Clear edge from last update
        button_state[i].edge = 0;
        // Read new state of GPIO pin
        uint8_t gpio_state = HAL_GPIO_ReadPin(button_pin[i].port, button_pin[i].pin);

        // Check if we're in a debounce period
        if(button_state[i].debounce_counter == 0){
            // Not in a debounce period - update state and set debounce counter if button changed
            if(gpio_state != button_state[i].state) {
                button_state[i].state = gpio_state;
                button_state[i].debounce_counter = BUTTON_DEBOUNCE_PERIOD;

                if(gpio_state) button_state[i].edge = 1;
                else button_state[i].edge = -1;
            }
        } else {
            // In a debounce state - decrement counter and do nothing else
            button_state[i].debounce_counter--;
        }
        // Update GPIO state
        button_state[i].gpio_state = gpio_state;
    }
}