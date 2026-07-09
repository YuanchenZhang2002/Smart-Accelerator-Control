/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "system_state.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef struct {
    uint16_t pedal1_min;//add2 minimum threshold
    uint16_t pedal1_max;//add2 maximum threshold
    uint16_t pedal2_min;//add1 minimum threshold
    uint16_t pedal2_max;//add1 maximum threshold
    uint32_t magic;
    uint32_t reserved;//this flash write 8 bytes at a time
} __attribute__((aligned(8))) CalibrationData_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void Process_ADC_DATA(void);
void Save_Calibration_To_Flash(void);
uint16_t map_accelerator_value(uint16_t adc_val, uint16_t pedal_min, uint16_t pedal_max);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define B1_EXTI_IRQn EXTI15_10_IRQn
#define CALIBRATION_Pin GPIO_PIN_1
#define CALIBRATION_GPIO_Port GPIOA
#define CALIBRATION_EXTI_IRQn EXTI1_IRQn
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define BRAKE_Pin GPIO_PIN_7
#define BRAKE_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define RELAY_CTRL_Pin GPIO_PIN_5
#define RELAY_CTRL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define CALIBRATION_ADDR     0x080FF800  // Page 511
#define CALIBRATION_MAGIC    0x55AA55AA

#define RING_ADC_MIN 1170  
#define RING_ADC_MAX 2822  

#define ADC_OUT_OF_RANGE_MIN 100
#define ADC_OUT_OF_RANGE_MAX  4045

#define RATIONALITY_TOLERANCE 100

#define RING_ENTRY_THRESHOLD  150
#define RING_EXIT_THRESHOLD   50
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
