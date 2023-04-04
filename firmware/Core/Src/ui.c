#include "ui.h"
#include "auto_brightness.h"
#include "CAN.h"
#include "dial.h"
#include "ws2812.h"

#include <math.h>

enum UI_State_t ui_state = UI_STARTUP_ANIM;

uint8_t auto_brighness_enabled = 0;

int32_t startup_anim_timer;
int32_t startup_wipe_var;
enum StartupAnimState_t {
  SA_Bronco,
  SA_Racing,
  SA_BR23
} startup_anim_state;

void update_state(enum UI_State_t new_state){
  // Clean up from current state
  switch(ui_state) {
    case UI_STARTUP_ANIM:
      brightness = DEFAULT_BRIGHTNESS; // Reset brightness after startup anim
      break;
    default:
      break;
  }

  // Set up for new state
  switch(new_state) {
    case UI_STARTUP_ANIM:
      startup_anim_timer = 0;
      startup_wipe_var = 0;
      startup_anim_state = SA_Bronco;
      break;
    default:
      break;
  }

  // update state variable
  ui_state = new_state;
}


void init_ui(void){
  ws2812_init();
  wipe_display();
  update_display();

  update_state(UI_STARTUP_ANIM);
}

// Wipe animation for startup
uint8_t wipe_anim_brightness;
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
union color_t gold_wipe(struct xy_t coord){
  int32_t cx = coord.x - 67;
  int32_t cy = coord.y - 10;

  int32_t dist = sqrt(cx * cx + cy * cy);

  int32_t anim_brightness = MAX(MIN((startup_wipe_var - dist) * 2, 40), 0) * wipe_anim_brightness / 256;
  return hsv(20, 255, anim_brightness);
}


void startup_animation(void) {
  startup_anim_timer++;
  switch(startup_anim_state){
    case SA_Bronco:
        write_char(b_7SEG, DIGIT_0, 0, COLOR_GOLD);
        write_char(r_7SEG, DIGIT_1, 0, COLOR_GOLD);
        write_char(o_7SEG, DIGIT_2, 0, COLOR_GOLD);
        write_char(n_7SEG, DIGIT_3, 0, COLOR_GOLD);
        write_char(c_7SEG, DIGIT_4, 0, COLOR_GOLD);
        write_char(o_7SEG, DIGIT_5, 0, COLOR_GOLD);
        if(startup_anim_timer > 75) {
          startup_anim_state = SA_Racing;
          startup_anim_timer = 0;
        }
      break;
    case SA_Racing:
        write_char(r_7SEG, DIGIT_0, 0, COLOR_GOLD);
        write_char(a_7SEG, DIGIT_1, 0, COLOR_GOLD);
        write_char(c_7SEG, DIGIT_2, 0, COLOR_GOLD);
        write_char(i_7SEG, DIGIT_3, 0, COLOR_GOLD);
        write_char(n_7SEG, DIGIT_4, 0, COLOR_GOLD);
        write_char(g_7SEG, DIGIT_5, 0, COLOR_GOLD);
        if(startup_anim_timer > 75) {
          startup_anim_state = SA_BR23;
          startup_anim_timer = 0;
        }
      break;
    case SA_BR23:
      if(startup_anim_timer < 50){
        wipe_anim_brightness = 50;
        startup_wipe_var += 2;
      }
      // fade out
      if(startup_anim_timer > 50) {
        wipe_anim_brightness = MAX(100 - startup_anim_timer, 0);
      }

      shade_display(gold_wipe);
      union color_t text_col = COLOR_RED;
      write_char(b_7SEG, DIGIT_1, 0, text_col);
      write_char(r_7SEG, DIGIT_2, 0, text_col);
      write_char(dash_7SEG, DIGIT_3, 0, text_col);
      write_digit(2, DIGIT_4, 0, text_col);
      write_digit(3, DIGIT_5, 0, text_col);
      break;
  }
}


union color_t voltage_color(void){
    uint32_t voltage = carState.battery_voltage;

    if(voltage < 10 * ECU_3_battery_voltage.divisor){
      return flash(COLOR_RED, 250, 125);
    } else if(voltage < 11 * ECU_3_battery_voltage.divisor){
      return COLOR_ORANGE;
    } else if(voltage < 12 * ECU_3_battery_voltage.divisor){
      return COLOR_YELLOW;
    } else {
      return COLOR_GREEN;
    }
}

uint32_t can_pulse_brightness;
union color_t no_can_pulse(struct xy_t coord){
  return (union color_t) {.color={.r=can_pulse_brightness, .g=can_pulse_brightness}};
}


void draw_shift_lights(void) {
  if(carState.rpm < FLASH_RPM){
    if(carState.rpm > MIN_RPM) { // draw first set of shift lights
      write_shift_lights(0, 2, COLOR_GREEN);
      write_shift_lights(10, 2, COLOR_GREEN);
    }
    if(carState.rpm > MID_RPM) {
      write_shift_lights(2, 2, COLOR_YELLOW);
      write_shift_lights(8, 2, COLOR_YELLOW);
    }
    if(carState.rpm > HIGH_RPM) {
      write_shift_lights(4, 4, COLOR_RED);
    }
  } else {
    write_shift_lights(0, 12, flash(COLOR_VERY_RED, 100, brightness / 2));
  }
}

void draw_tach(void){
  float num_leds = (float) carState.rpm * 12.0f / ((float)TACH_MAX_RPM);
  if(carState.rpm > TACH_MAX_RPM) {
    write_tach(0, 12, flash(COLOR_VERY_RED, 100, 75));
  } else {
    for(uint8_t i = 0; i < num_leds; ++i){
      write_tach(i, 1, hsv(4 * i, 255, brightness));
    }
    uint8_t last_led = (uint8_t)floor(num_leds);
    write_tach(last_led, 1, hsv(last_led * 4, 255, ((uint8_t)((float)brightness * (num_leds - floor(num_leds))))));
  }

}

uint8_t rainbow_offset = 0;
union color_t rainbow(struct xy_t coord){
  return hsv((coord.x + coord.y) * 3 + rainbow_offset, 255, 2);
}

void update_ui(void) {
    wipe_display();

    // If CAN is missing, switch to the CANT state
    if(HAL_GetTick() - carState.last_message_tick > NO_CAN_TIMEOUT) {
      if(ui_state != UI_CANT && ui_state != UI_STARTUP_ANIM) {
        update_state(UI_CANT);
      }
    }

    switch(ui_state) {
        case UI_ERROR:
          shade_display(&rainbow);
          rainbow_offset++;

          write_char(E_7SEG, DIGIT_0, 0, COLOR_WHITE);
          write_char(r_7SEG, DIGIT_1, 0, COLOR_WHITE);
          write_char(r_7SEG, DIGIT_2, 0, COLOR_WHITE);
          write_char(o_7SEG, DIGIT_3, 0, COLOR_WHITE);
          write_char(r_7SEG, DIGIT_4, 1, COLOR_WHITE);
          break;
        case UI_STARTUP_ANIM:
          if(carState.rpm > 0) { // skip anim if engine is running
            update_state(UI_ENGINE_RUNNING);
            break;
          }
          startup_animation();
          if(startup_anim_timer > 150 && startup_anim_state == SA_BR23) {
            update_state(UI_ENGINE_OFF);
          }
          break;
        case UI_CANT:
          can_pulse_brightness = HAL_GetTick() % 1000 > 900 ? brightness : 0;
          // if(can_pulse_brightness > 1000) can_pulse_brightness = 000 - can_pulse_brightness;
          // can_pulse_brightness = gamma_16(can_pulse_brightness * 30) * brightness / 256 / 256;
          shade_display(no_can_pulse);

          union color_t text_col = COLOR_VERY_RED;
          write_char(C_7SEG, DIGIT_1, 0, text_col);
          write_char(a_7SEG, DIGIT_2, 0, text_col);
          write_char(n_7SEG, DIGIT_3, 0, text_col);
          write_char(t_7SEG, DIGIT_4, 0, text_col);

          break;
        case UI_ENGINE_OFF:
          {
            if(carState.rpm > 0) { // Check if engine is running
              update_state(UI_ENGINE_RUNNING);
            }

            // Write dial state
            // write_digit(get_dial(DIAL_0), DIGIT_0, 1, COLOR_RED);

            // Write "OFF"
            write_digit(0, DIGIT_0, 0, COLOR_WHITE);
            write_char(F_7SEG, DIGIT_1, 0, COLOR_WHITE);
            write_char(F_7SEG, DIGIT_2, 0, COLOR_WHITE);
            
            // Draw voltage and voltage light
            union color_t v_col = voltage_color();
            write_status(4, v_col);
            write_fixedpoint(carState.battery_voltage, DIGIT_3, 3, 1, v_col);

            draw_shift_lights();
            draw_tach();
          }
          
          break;

        case UI_ENGINE_RUNNING:
          {
            if(carState.rpm == 0) { // Check if engine is running
              update_state(UI_ENGINE_OFF);
            }
            // Write dial state
            // write_digit(get_dial(DIAL_0), DIGIT_0, 1, COLOR_RED);

            // Write RPM
            write_int(carState.rpm, DIGIT_0, 6, COLOR_RED);
            
            // Draw  voltage light
            union color_t v_col = voltage_color();
            write_status(4, v_col);

            draw_shift_lights();
            draw_tach();
          }
          
          break;

        default:
          update_state(UI_ERROR);
    }
    update_display();
}
