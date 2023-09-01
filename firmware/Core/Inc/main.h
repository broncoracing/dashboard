/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

struct CarState {
  // Things from ECU
  uint16_t rpm;
  uint16_t lambda;
  
  uint16_t oil_temp;
  uint16_t water_temp;
  
  uint16_t oil_pressure;
  uint16_t battery_voltage;
  uint8_t gear_position;
  uint16_t fuel_pressure;
  uint8_t speedometer;

  uint8_t apps;
  uint8_t tps;
  uint16_t bse_front;
  uint16_t bse_rear;

  // Things from dyno controller
  uint16_t dyno_rpm;
  uint16_t dyno_target;
  uint16_t dyno_load;
  uint16_t dyno_valve_pos;
  
  // CAN Timeout
  uint32_t last_message_tick; // Warning - will overflow if left running for a few months!

  uint8_t check_engine;
  uint8_t ecu_fault_code;
};
extern struct CarState carState;


/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

// Period for CAN0 frame (Dials, temperature, etc)
#define CAN0_PERIOD_MS 100

// Period for CAN1 frame (Shifting, launch control, other buttons)
#define CAN1_PERIOD_MS 10

// Don't allow downshifts above this RPM
#define MONEY_SHIFT_THRESHOLD 10200


// Time before the dashboard goes into CANT state
#define NO_CAN_TIMEOUT 500 // milliseconds

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BTN0_Pin GPIO_PIN_2
#define BTN0_GPIO_Port GPIOB
#define BTN1_Pin GPIO_PIN_10
#define BTN1_GPIO_Port GPIOB
#define BTN2_Pin GPIO_PIN_11
#define BTN2_GPIO_Port GPIOB
#define BTN3_Pin GPIO_PIN_12
#define BTN3_GPIO_Port GPIOB
#define BTN4_Pin GPIO_PIN_14
#define BTN4_GPIO_Port GPIOB
#define BTN5_Pin GPIO_PIN_8
#define BTN5_GPIO_Port GPIOA
#define LED_7_Pin GPIO_PIN_9
#define LED_7_GPIO_Port GPIOA
#define LED_6_Pin GPIO_PIN_10
#define LED_6_GPIO_Port GPIOA
#define LED_5_Pin GPIO_PIN_11
#define LED_5_GPIO_Port GPIOA
#define LED_4_Pin GPIO_PIN_12
#define LED_4_GPIO_Port GPIOA
#define LED_3_Pin GPIO_PIN_4
#define LED_3_GPIO_Port GPIOB
#define LED_2_Pin GPIO_PIN_5
#define LED_2_GPIO_Port GPIOB
#define LED_1_Pin GPIO_PIN_6
#define LED_1_GPIO_Port GPIOB
#define LED_0_Pin GPIO_PIN_7
#define LED_0_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
