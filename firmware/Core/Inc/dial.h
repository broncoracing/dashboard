#ifndef DIAL_H
#define DIAL_H

#include "main.h"

enum DialIdx_t {
    DIAL_0, // TODO Better dial names
    DIAL_1,
    NUM_DIALS
};

#define DEGLITCH_COUNT 10
#define NUM_POSITIONS 12

// TODO Take pointer/array instead of individual values to allowe changing number of dials more easily
void update_dial_state(uint16_t dial0, int16_t dial1);

uint8_t get_dial(enum DialIdx_t dial);

#endif