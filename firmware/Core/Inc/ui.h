#ifndef UI_H
#define UI_H


#define ENGINE_COLD_TEMP      50 // degrees C
#define ENGINE_HOT_TEMP       95
#define ENGINE_VERY_HOT_TEMP  105


#define MIN_RPM 5800
#define FLASH_RPM 13800 // Above this RPM the tachometer will flash
#define FLASH_TIME_S 0.15 // On time for the flashes

enum UI_State_t {
    STARTUP_ANIM,
    NO_CAN,
    ENGINE_OFF,
    ENGINE_RUNNING,
    NUM_UI_STATES
};

extern uint8_t auto_brightness_enabled;

extern enum UI_State_t ui_state;

void update_ui(void);



#endif