/**
 ******************************************************************************
 * @file    ui.c
 * @brief   EV Dashboard User Interface - 2026 Edition
 * @author  Your Name
 * @date    2026
 ******************************************************************************
 */

#include "ui.h"
#include "auto_brightness.h"
#include "CAN.h"
#include "dial.h"
#include "ws2812.h"
#include <math.h>

// UI State Machine
enum UI_State_t {
    UI_STARTUP_ANIM,
    UI_PRECHARGE,           // HV system precharging
    UI_READY_TO_DRIVE,      // Main driving display
    UI_CHARGING,            // Charging mode display
    UI_FAULT,               // Fault/error display
    UI_SLEEP,               // Low power / parked
    UI_DIAGNOSTICS,         // Engineering mode
    UI_NO_COMM              // Lost communication with inverter/BMS
} ui_state = UI_STARTUP_ANIM;

// UI Timers
int32_t startup_anim_timer = 0;
int32_t fault_flash_timer = 0;
int32_t comm_loss_timer = 0;

// Animation variables
uint8_t rainbow_offset = 0;
int32_t startup_wipe_var = 0;
uint8_t wipe_anim_brightness = 50;

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CLAMP(x, min, max) (MAX(MIN((x), (max)), (min)))

// Calculate power in kW from voltage and current
void calculate_power(void) {
    // Power = V * I / 100 (since voltage is in V*10 and current is in A*10)
    carState.motor_power_kw = (int32_t)carState.dc_bus_voltage * 
                              (int32_t)carState.dc_bus_current / 1000;
}

// Check if communication is healthy
void check_communications(void) {
    uint32_t now = HAL_GetTick();
    
    carState.inverter_comm_ok = (now - carState.last_inverter_msg_tick) < INVERTER_TIMEOUT_MS;
    carState.bms_comm_ok = (now - carState.last_bms_msg_tick) < BMS_TIMEOUT_MS;
    
    // If we lose communication, switch to NO_COMM state
    if (!carState.inverter_comm_ok && ui_state != UI_STARTUP_ANIM && ui_state != UI_NO_COMM) {
        ui_state = UI_NO_COMM;
        comm_loss_timer = 0;
    }
}

// =============================================================================
// COLOR FUNCTIONS
// =============================================================================

union color_t get_temp_color(int16_t temp, int16_t warn_temp, int16_t max_temp) {
    if (temp > max_temp) {
        return flash(COLOR_VERY_RED, 250, 125);
    } else if (temp > warn_temp) {
        return COLOR_RED;
    } else if (temp > warn_temp / 2) {
        return COLOR_YELLOW;
    } else {
        return COLOR_GREEN;
    }
}

union color_t get_soc_color(void) {
    if (carState.soc_percent <= SOC_CRITICAL) {
        return flash(COLOR_VERY_RED, 500, 250);
    } else if (carState.soc_percent <= SOC_LOW) {
        return COLOR_RED;
    } else if (carState.soc_percent < 60) {
        return COLOR_YELLOW;
    } else {
        return COLOR_GREEN;
    }
}

union color_t get_power_color(int32_t power_kw) {
    if (power_kw < -100) {  // Strong regen
        return COLOR_CYAN;
    } else if (power_kw < 0) {  // Light regen
        return COLOR_BLUE;
    } else if (power_kw < 500) {  // Normal power
        return COLOR_GREEN;
    } else if (power_kw < 1000) {  // High power
        return COLOR_YELLOW;
    } else {  // Maximum power
        return COLOR_RED;
    }
}

// =============================================================================
// DISPLAY ELEMENTS
// =============================================================================

void draw_soc_gauge(void) {
    // 12 LEDs showing battery state of charge
    uint8_t num_leds = (carState.soc_percent * 12) / 100;
    if (num_leds > 12) num_leds = 12;
    
    // Determine color based on SOC
    union color_t soc_color = get_soc_color();
    
    // Draw filled portion
    for (uint8_t i = 0; i < num_leds; i++) {
        write_shift_lights(i, 1, soc_color);
    }
    
    // Optionally draw empty portion dimly
    for (uint8_t i = num_leds; i < 12; i++) {
        write_shift_lights(i, 1, (union color_t){.color={.r=1, .g=1, .b=1}});
    }
}

void draw_power_bar(void) {
    // Power bar: Regen (blue) ← 0 → Power (green/yellow/red)
    // Uses tach LEDs (12 LEDs)
    
    int32_t power = carState.motor_power_kw;
    
    if (power >= 0) {
        // Positive power (acceleration)
        // Scale: 0 to 150 kW maps to 0-12 LEDs
        uint8_t num_leds = (power * 12) / 1500;  // 1500 = 150kW * 10
        if (num_leds > 12) num_leds = 12;
        
        for (uint8_t i = 0; i < num_leds; i++) {
            union color_t led_color;
            if (i < 4) {
                led_color = COLOR_GREEN;
            } else if (i < 8) {
                led_color = COLOR_YELLOW;
            } else {
                led_color = COLOR_RED;
            }
            write_tach(i, 1, led_color);
        }
    } else {
        // Negative power (regeneration)
        // Scale: 0 to -50 kW maps to 0-6 LEDs (left side)
        int32_t abs_power = -power;
        uint8_t num_leds = (abs_power * 6) / 500;  // 500 = 50kW * 10
        if (num_leds > 6) num_leds = 6;
        
        for (uint8_t i = 0; i < num_leds; i++) {
            write_tach(i, 1, COLOR_CYAN);
        }
    }
}

void draw_status_indicators(void) {
    // Status LED indicators (8 total)
    // 0: Drive Mode (E/N/S)
    // 1: Ready to Drive
    // 2: Regen Active
    // 3: Fault Warning
    // 4: 12V Battery
    // 5: Motor Temp
    // 6: Inverter Temp  
    // 7: Battery Temp
    
    // [0] Drive mode indicator
    union color_t mode_color = COLOR_GREEN;
    if (carState.drive_mode == 2) mode_color = COLOR_RED;      // Sport
    else if (carState.drive_mode == 0) mode_color = COLOR_CYAN; // Eco
    write_status(0, mode_color);
    
    // [1] Ready to drive
    if (carState.inverter_enable && carState.run_mode) {
        write_status(1, COLOR_GREEN);
    } else {
        write_status(1, COLOR_RED);
    }
    
    // [2] Regen active
    if (carState.motor_power_kw < -50) {  // > 5kW regen
        write_status(2, COLOR_CYAN);
    }
    
    // [3] Fault warning
    if (carState.post_fault || carState.run_fault || carState.bms_fault_code) {
        write_status(3, flash(COLOR_RED, 500, 250));
    }
    
    // [4] 12V battery voltage
    uint16_t glv_voltage_v = carState.glv_voltage / 100;  // Convert to volts
    if (glv_voltage_v < 110) {  // < 11.0V
        write_status(4, flash(COLOR_RED, 250, 125));
    } else if (glv_voltage_v < 120) {  // < 12.0V
        write_status(4, COLOR_YELLOW);
    } else {
        write_status(4, COLOR_GREEN);
    }
    
    // [5] Motor temperature
    union color_t motor_temp_color = get_temp_color(
        carState.motor_temp, 
        MOTOR_MAX_TEMP - 300,  // Warn at 120C (if max is 150C)
        MOTOR_MAX_TEMP
    );
    write_status(5, motor_temp_color);
    
    // [6] Inverter temperature (highest of IGBT temps)
    int16_t max_igbt = MAX(carState.igbt_a_temp, 
                       MAX(carState.igbt_b_temp, carState.igbt_c_temp));
    union color_t inverter_temp_color = get_temp_color(
        max_igbt,
        INVERTER_MAX_TEMP - 200,  // Warn at 60C (if max is 80C)
        INVERTER_MAX_TEMP
    );
    write_status(6, inverter_temp_color);
    
    // [7] Battery pack temperature
    union color_t battery_temp_color = get_temp_color(
        carState.pack_temp_max,
        BATTERY_MAX_TEMP - 200,  // Warn at 40C (if max is 60C)
        BATTERY_MAX_TEMP
    );
    write_status(7, battery_temp_color);
}

// =============================================================================
// STARTUP ANIMATION
// =============================================================================

union color_t gold_wipe(struct xy_t coord) {
    int32_t cx = coord.x - 67;
    int32_t cy = coord.y - 10;
    int32_t dist = sqrt(cx * cx + cy * cy);
    int32_t anim_brightness = MAX(MIN((startup_wipe_var - dist) * 2, 40), 0) * 
                             wipe_anim_brightness / 256;
    return hsv(20, 255, anim_brightness);
}

void startup_animation(void) {
    startup_anim_timer++;
    
    if (startup_anim_timer < 100) {
        // Wipe in
        wipe_anim_brightness = 50;
        startup_wipe_var += 2;
        
        shade_display(gold_wipe);
        
        // Display "EV-26" or your vehicle name
        write_char(E_7SEG, DIGIT_0, 0, COLOR_CYAN);
        write_char(v_7SEG, DIGIT_1, 0, COLOR_CYAN);
        write_char(dash_7SEG, DIGIT_2, 0, COLOR_CYAN);
        write_digit(2, DIGIT_3, 0, COLOR_CYAN);
        write_digit(6, DIGIT_4, 0, COLOR_CYAN);
        
    } else if (startup_anim_timer < 150) {
        // Fade out
        wipe_anim_brightness = MAX(150 - startup_anim_timer, 0);
        shade_display(gold_wipe);
        
        write_char(E_7SEG, DIGIT_0, 0, COLOR_CYAN);
        write_char(v_7SEG, DIGIT_1, 0, COLOR_CYAN);
        write_char(dash_7SEG, DIGIT_2, 0, COLOR_CYAN);
        write_digit(2, DIGIT_3, 0, COLOR_CYAN);
        write_digit(6, DIGIT_4, 0, COLOR_CYAN);
        
    } else {
        // Animation complete, check inverter state
        if (carState.vsm_state >= 4) {  // VSM state 4+ means precharge complete
            ui_state = UI_READY_TO_DRIVE;
        } else {
            ui_state = UI_PRECHARGE;
        }
    }
}

// =============================================================================
// UI STATE HANDLERS
// =============================================================================

void ui_handle_precharge(void) {
    // Display "HV" alternating with precharge percentage
    if ((HAL_GetTick() / 500) % 2) {
        write_char(H_7SEG, DIGIT_0, 0, COLOR_YELLOW);
        write_char(v_7SEG, DIGIT_1, 0, COLOR_YELLOW);
        
        // Show VSM state on right side
        write_digit(carState.vsm_state, DIGIT_5, 0, COLOR_YELLOW);
    } else {
        // Calculate precharge percentage (rough estimate from DC bus voltage)
        uint16_t precharge_pct = (carState.dc_bus_voltage * 100) / PACK_MAX_VOLTAGE;
        if (precharge_pct > 100) precharge_pct = 100;
        
        write_int(precharge_pct, DIGIT_3, 3, COLOR_YELLOW);
    }
    
    // Show minimal power bar during precharge
    draw_soc_gauge();
    
    // Check if precharge complete
    if (carState.vsm_state >= 4 || carState.inverter_enable) {
        ui_state = UI_READY_TO_DRIVE;
    }
}

void ui_handle_ready_to_drive(void) {
    // Main driving display
    // Left side: SOC% or Motor RPM
    // Right side: Speed or Power
    
    // Calculate derived values
    calculate_power();
    
    // Display SOC on left (or motor RPM if moving)
    if (abs(carState.motor_speed) > 100) {
        // Show motor RPM
        write_int(abs(carState.motor_speed), DIGIT_0, 4, COLOR_WHITE);
    } else {
        // Show SOC%
        write_int(carState.soc_percent, DIGIT_0, 3, get_soc_color());
    }
    
    // Display power or speed on right
    if (carState.speed_mph > 5) {
        // Show speed
        write_int(carState.speed_mph, DIGIT_3, 3, COLOR_WHITE);
    } else {
        // Show power in kW
        int32_t power_display = abs(carState.motor_power_kw / 10);
        union color_t power_col = get_power_color(carState.motor_power_kw);
        write_int(power_display, DIGIT_3, 3, power_col);
    }
    
    // Draw bargraphs
    draw_soc_gauge();
    draw_power_bar();
    draw_status_indicators();
    
    // Check for faults
    if (carState.post_fault || carState.run_fault) {
        ui_state = UI_FAULT;
        fault_flash_timer = 0;
    }
}

void ui_handle_charging(void) {
    // Charging mode display
    // Show SOC% and charge power
    
    // Left: SOC%
    write_int(carState.soc_percent, DIGIT_0, 3, COLOR_CYAN);
    
    // Right: Charge power (kW)
    // TODO: Get charge power from BMS
    
    // Animate SOC gauge during charging
    draw_soc_gauge();
    
    // Flash "CHG" on power bar
    if ((HAL_GetTick() / 500) % 2) {
        write_shift_lights(4, 4, COLOR_CYAN);
    }
}

void ui_handle_fault(void) {
    // Display fault codes
    fault_flash_timer++;
    
    // Flash background red
    if ((fault_flash_timer / 25) % 2) {
        shade_display(&rainbow);
        rainbow_offset += 5;
    }
    
    // Display "Err" on left
    write_char(E_7SEG, DIGIT_0, 0, COLOR_RED);
    write_char(r_7SEG, DIGIT_1, 0, COLOR_RED);
    write_char(r_7SEG, DIGIT_2, 0, COLOR_RED);
    
    // Display fault code on right
    if (carState.post_fault) {
        // Show POST fault code (lower 16 bits)
        write_int(carState.post_fault & 0xFFFF, DIGIT_3, 4, COLOR_RED);
    } else if (carState.run_fault) {
        // Show RUN fault code (lower 16 bits)
        write_int(carState.run_fault & 0xFFFF, DIGIT_3, 4, COLOR_ORANGE);
    } else if (carState.bms_fault_code) {
        // Show BMS fault
        write_int(carState.bms_fault_code, DIGIT_3, 3, COLOR_YELLOW);
    }
    
    // Flash all status LEDs red
    for (uint8_t i = 0; i < 8; i++) {
        write_status(i, flash(COLOR_RED, 250, 125));
    }
    
    // Clear faults if they're resolved
    if (!carState.post_fault && !carState.run_fault && !carState.bms_fault_code) {
        ui_state = UI_READY_TO_DRIVE;
    }
}

void ui_handle_no_comm(void) {
    // Communication lost
    comm_loss_timer++;
    
    // Flash "CONN" or similar
    if ((comm_loss_timer / 50) % 2) {
        write_char(C_7SEG, DIGIT_0, 0, COLOR_RED);
        write_char(o_7SEG, DIGIT_1, 0, COLOR_RED);
        write_char(n_7SEG, DIGIT_2, 0, COLOR_RED);
        write_char(n_7SEG, DIGIT_3, 0, COLOR_RED);
    }
    
    // Show which system lost communication
    if (!carState.inverter_comm_ok) {
        write_status(0, flash(COLOR_RED, 250, 125));
        write_status(1, flash(COLOR_RED, 250, 125));
    }
    if (!carState.bms_comm_ok) {
        write_status(6, flash(COLOR_RED, 250, 125));
        write_status(7, flash(COLOR_RED, 250, 125));
    }
    
    // Return to normal if communication restored
    if (carState.inverter_comm_ok && carState.bms_comm_ok) {
        ui_state = UI_READY_TO_DRIVE;
    }
}

void ui_handle_diagnostics(void) {
    // Engineering/diagnostic mode
    // Show raw inverter temperatures or other debug info
    
    // Cycle through different diagnostic screens based on dial position
    uint8_t diag_screen = carState.dial_pos[0] % 4;
    
    switch (diag_screen) {
        case 0:
            // Motor temp
            write_int(carState.motor_temp, DIGIT_0, 4, COLOR_YELLOW);
            write_int(carState.igbt_a_temp, DIGIT_3, 3, COLOR_ORANGE);
            break;
        case 1:
            // DC bus voltage and current
            write_int(carState.dc_bus_voltage, DIGIT_0, 4, COLOR_GREEN);
            write_int(abs(carState.dc_bus_current), DIGIT_3, 3, COLOR_CYAN);
            break;
        case 2:
            // Torque command vs feedback
            write_int(carState.commanded_torque, DIGIT_0, 4, COLOR_WHITE);
            write_int(carState.torque_feedback, DIGIT_3, 3, COLOR_YELLOW);
            break;
        case 3:
            // VSM state and other status
            write_digit(carState.vsm_state, DIGIT_0, 0, COLOR_MAGENTA);
            write_digit(carState.inverter_state, DIGIT_1, 0, COLOR_MAGENTA);
            write_int(carState.glv_voltage, DIGIT_3, 3, COLOR_GREEN);
            break;
    }
    
    draw_status_indicators();
}

// =============================================================================
// MAIN UI UPDATE FUNCTION
// =============================================================================

void init_ui(void) {
    ws2812_init();
    wipe_display();
    update_display();
    
    ui_state = UI_STARTUP_ANIM;
    startup_anim_timer = 0;
}

void update_ui(void) {
    // Check communication health
    check_communications();
    
    // Clear display
    wipe_display();
    
    // Run state machine
    switch (ui_state) {
        case UI_STARTUP_ANIM:
            startup_animation();
            break;
            
        case UI_PRECHARGE:
            ui_handle_precharge();
            break;
            
        case UI_READY_TO_DRIVE:
            ui_handle_ready_to_drive();
            break;
            
        case UI_CHARGING:
            ui_handle_charging();
            break;
            
        case UI_FAULT:
            ui_handle_fault();
            break;
            
        case UI_NO_COMM:
            ui_handle_no_comm();
            break;
            
        case UI_DIAGNOSTICS:
            ui_handle_diagnostics();
            break;
            
        default:
            ui_state = UI_READY_TO_DRIVE;
            break;
    }
    
    // Update physical display
    update_display();
}