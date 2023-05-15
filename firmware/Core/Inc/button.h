#ifndef BUTTON_H
#define BUTTON_H

#include "main.h"

#define BUTTON_DEBOUNCE_PERIOD 50

enum ButtonInput_t {
    BTN_0,
    BTN_UPSHIFT=1,
    BTN_2,
    BTN_DOWNDHIFT=3,
    BTN_4,
    BTN_LAUNCH_CTRL=5,
    NUM_BTNS
};

// Initialize buttons. This sets up the debouncing algorithms.
void init_buttons(void);

// Updates buttons with debouncing
void update_buttons(void);

// Get debounced button value
uint8_t get_button(enum ButtonInput_t channel);

// Returns 1 when button went hight last time update_buttons() was called
uint8_t get_button_just_pressed(enum ButtonInput_t channel);

#endif