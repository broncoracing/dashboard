#ifndef UI_H
#define UI_H

#include "main.h"

#define ENGINE_COLD_TEMP      50 // degrees C
#define ENGINE_HOT_TEMP       95
#define ENGINE_VERY_HOT_TEMP  105


#define MIN_RPM 5800 // Min RPM for the shift lights
#define MID_RPM 9000
#define HIGH_RPM 11200
#define FLASH_RPM 11800
#define TACH_MAX_RPM 13800 // Above this RPM the tachometer will flash
#define FLASH_TIME_S 0.15 // On time for the flashes

enum UI_State_t {
    UI_ERROR,
    UI_STARTUP_ANIM,
    UI_CANT, // bus is unable to can
    UI_ENGINE_OFF,
    UI_ENGINE_RUNNING,
    UI_DYNO_DISPLAY,
    UI_NUM_STATES
};

extern uint8_t auto_brightness_enabled;

extern enum UI_State_t ui_state;

void init_ui(void);

void update_ui(void);

void connect_dyno(void);

#endif