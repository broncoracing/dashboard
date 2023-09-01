#include "ui.h"
#include "auto_brightness.h"
#include "CAN.h"
#include "dial.h"
#include "ws2812.h"

#include <math.h>
#include "button.h"
enum UI_State_t ui_state = UI_STARTUP_ANIM;

uint8_t ui_clear_faults = 0;

uint8_t auto_brighness_enabled = 0;

int32_t ui_diag_timer = 0;
int32_t rainbow_timer = 0;
uint8_t rainbow_mode = 0;

int32_t startup_anim_timer;
int32_t startup_wipe_var;
enum StartupAnimState_t {
  SA_Bronco,
  SA_Racing,
  SA_BR23,
} startup_anim_state;

void update_state(enum UI_State_t new_state){
  ui_clear_faults = 0;
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

void connect_dyno(void) {
  update_state(UI_DYNO_DISPLAY);
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

void engine_status_lights(void) {
  // voltage status light
  union color_t v_col = voltage_color();
  write_status(4, v_col);

  // temp light
  union color_t ect_color;
  if(carState.water_temp > 95 * ECU_2_water_temp.divisor) {
    ect_color = flash(COLOR_VERY_RED, 500, 250);
  } else if (carState.water_temp > 85 * ECU_2_water_temp.divisor) {
    ect_color = COLOR_RED;
  } else if (carState.water_temp > 45 * ECU_2_water_temp.divisor) {
    ect_color = COLOR_GREEN;
  } else {
    ect_color = COLOR_CYAN;
  }

  write_status(5, ect_color);

  // fuel pressure light
  union color_t fuel_pressure_color;
  if(carState.fuel_pressure > 300) {
    fuel_pressure_color = COLOR_GREEN;
  } else if(carState.fuel_pressure > 250) {
    fuel_pressure_color = COLOR_ORANGE;
  } else {
    fuel_pressure_color = COLOR_RED;
  }
  write_status(6, fuel_pressure_color);

  // oil pressure light
  if(carState.oil_pressure < 100) {
    write_status(7, COLOR_RED);
  } else {
    write_status(7, COLOR_GREEN);
  }
  // check engine light
  if(carState.check_engine){
    write_status(3, COLOR_RED);
  }
}

// process toggling into/out of diagnostic mode
void diagnostic_toggle(void) {
  if(get_button(BTN_LOG_MARKER) && get_button(BTN_LAUNCH_CTRL)) {
    write_shift_lights(0, 8, COLOR_CYAN);
    ui_diag_timer++;
    if(ui_diag_timer > 200) {
      if(ui_state == UI_DIAGNOSTICS) {
        update_state(UI_ENGINE_OFF);
      } else {
        update_state(UI_DIAGNOSTICS);
      }
      ui_diag_timer = 0;
    }
  } else {
    ui_diag_timer = 0;
  }
}

void update_ui(void) {
    if(get_button(BTN_UPSHIFT) && get_button(BTN_DOWNDHIFT)) {
      rainbow_timer++;
      if(rainbow_timer > 300) {
        rainbow_mode = !rainbow_mode;
        rainbow_timer = 0;
      }
    }

    if(rainbow_mode) {
      rainbow_offset += 5;
      shade_display(rainbow);
    } else {
      wipe_display();
    }

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
          if(HAL_GetTick() - carState.last_message_tick < NO_CAN_TIMEOUT) {
            update_state(UI_ENGINE_OFF);
          }
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
            // uint8_t btn_s = 0;
            // for(uint8_t i = 0; i < NUM_BTNS; ++i) {
            //   if(get_button(i)) btn_s |= 1 << i;
            // }
            // write_fixedpoint(btn_s, DIGIT_0, 3, 0, COLOR_CYAN);
            
            // Draw voltage and voltage light
            union color_t v_col = voltage_color();
            
            write_fixedpoint(carState.battery_voltage / 100, DIGIT_3, 3, 1, v_col);

            draw_shift_lights();
            draw_tach();

            engine_status_lights();

            diagnostic_toggle();
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
            write_fixedpoint(carState.rpm / 100, DIGIT_3, 3, 1, COLOR_RED);
            
            // Draw  status lights
            engine_status_lights();

            draw_shift_lights();
            draw_tach();

            diagnostic_toggle();
          }
          
          break;
        case UI_DIAGNOSTICS:
          {
            for(uint8_t btn = 0; btn < NUM_BTNS; ++btn) {
              if(get_button(btn)) {
                write_shift_lights(btn, 1, COLOR_WHITE);
              }
            }

            ui_clear_faults = get_button(BTN_UPSHIFT) && get_button(BTN_DOWNDHIFT);
            write_int(carState.ecu_fault_code, DIGIT_0, 3, COLOR_RED);
            diagnostic_toggle();
          }
          break;
        case UI_DYNO_DISPLAY:
         {
            // Write target RPM
            write_fixedpoint(carState.dyno_target / 100, DIGIT_0, 3, 1, COLOR_WHITE);

            // Write RPM
            int32_t dyno_err = carState.dyno_target - carState.dyno_rpm;
            if(dyno_err < 0) dyno_err = -dyno_err;
            union color_t dyno_rpm_col;

            if(dyno_err < 100){
              dyno_rpm_col = COLOR_GREEN;
            }else if(dyno_err < 250) {
              dyno_rpm_col = COLOR_YELLOW;
            } else if(dyno_err < 500) {
              dyno_rpm_col = COLOR_ORANGE;
            } else {
              dyno_rpm_col = COLOR_RED;
            }
            write_fixedpoint(carState.dyno_rpm / 100, DIGIT_3, 3, 1, dyno_rpm_col);
            
            // Draw status lights
          
            engine_status_lights();

            // use shift ligts for valve pos
            uint8_t n_shift_lights = (uint32_t) carState.dyno_valve_pos * 12 / 12000;
            if(n_shift_lights > 12) n_shift_lights = 12;
            write_shift_lights(0, n_shift_lights, COLOR_RED);
            draw_tach();
         }
         break;
        default:
          update_state(UI_ERROR);
    }
    update_display();
}
