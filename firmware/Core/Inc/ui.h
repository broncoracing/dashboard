/**
 ******************************************************************************
 * @file    ui.h
 * @brief   EV Dashboard User Interface Header
 ******************************************************************************
 */

#ifndef UI_H
#define UI_H

#include "main.h"
#include "display.h"

/* Seven segment character definitions for UI */
// Uppercase letters
#define A_7SEG 0b01110111
#define C_7SEG 0b00111001
#define E_7SEG 0b01111001
#define F_7SEG 0b01110001
#define H_7SEG 0b01110110
#define P_7SEG 0b01110011

// Lowercase letters
#define a_7SEG 0b01011111
#define b_7SEG 0b01111100
#define c_7SEG 0b01011000
#define d_7SEG 0b01011110
#define g_7SEG 0b01101111
#define h_7SEG 0b01110100
#define i_7SEG 0b00010000
#define n_7SEG 0b01010100
#define o_7SEG 0b01011100
#define r_7SEG 0b01010000
#define t_7SEG 0b01111000
#define u_7SEG 0b00011100
#define v_7SEG 0b00011100  // Same as u in 7-segment

// Special characters
#define dash_7SEG 0b01000000

/* Function prototypes */
void init_ui(void);
void update_ui(void);

#endif /* UI_H */