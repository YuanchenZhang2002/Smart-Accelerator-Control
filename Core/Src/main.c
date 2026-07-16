/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_CHANNEL_NUM 3
#define FILTER_WINDOW 10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t activation_flag=0;
volatile uint16_t adc_dma_buffer[ADC_CHANNEL_NUM*FILTER_WINDOW];
volatile uint16_t adc_filtered_APP1=0;
volatile uint16_t adc_filtered_APP2=0;
volatile uint16_t adc_filtered_RING=0;
volatile uint8_t calibration_save_flag=0;
volatile uint8_t brake_active=0;
extern volatile CalibrationData_t g_current_cal_data;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_DAC1_Init();
  MX_USART2_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED); // MUST run before ADC start to correct internal offset error
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_CHANNEL_NUM*FILTER_WINDOW);
  HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
  HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);
  HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Process_ADC_DATA();
    state_transition(g_system_state);
    state_action(g_system_state);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Prevent unused argument(s) compilation warning */
  if (GPIO_Pin==GPIO_PIN_13)
  {
    activation_flag=1;
  }
  else if (GPIO_Pin==GPIO_PIN_1)
  {
    calibration_save_flag=1;
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) 
    {
        static uint8_t brake_press_count = 0;
        static uint8_t brake_release_count = 0;
        GPIO_PinState current_brake_pin = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7); 
        if (current_brake_pin == GPIO_PIN_RESET) 
        {
            brake_release_count = 0; 
            
            
            if (brake_press_count < 3) 
            {
                brake_press_count++;
            }
            else if (brake_press_count == 3) 
            {
                brake_active = SET;
                brake_press_count++; 
            }
        }
        else 
        {
            brake_press_count = 0; 
            if (brake_release_count < 3) 
            {
                brake_release_count++;
            }
            else if (brake_release_count == 3) 
            {
                brake_active = RESET;
                brake_release_count++; 
            }
        }
    }
}

void Process_ADC_DATA(void)
{
  /* sum the values of adc_dma_buffer */
  uint32_t sum_APP1 = 0;
  uint32_t sum_APP2 = 0;
  uint32_t sum_Ring = 0;

  for (uint8_t i=0; i<FILTER_WINDOW; i++) {
    uint8_t offset=i*ADC_CHANNEL_NUM;

    sum_APP1+=adc_dma_buffer[offset];
    sum_APP2+=adc_dma_buffer[offset+1];
    sum_Ring+=adc_dma_buffer[offset+2];
  }
  adc_filtered_APP1=sum_APP1/FILTER_WINDOW;
  adc_filtered_APP2=sum_APP2/FILTER_WINDOW;
  adc_filtered_RING=sum_Ring/FILTER_WINDOW;

  /* calculate the average of each channel */
}

void Save_Calibration_To_Flash(void)
{
    
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks       = FLASH_BANK_1; 
    EraseInitStruct.Page        = 511; // Corresponding address 0x080FF800
    EraseInitStruct.NbPages     = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
        Error_Handler();
    }

    uint64_t *data_to_write = (uint64_t *)&g_current_cal_data;

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, CALIBRATION_ADDR, data_to_write[0]);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, CALIBRATION_ADDR + 8, data_to_write[1]);

    HAL_FLASH_Lock();
}


uint16_t map_accelerator_value(uint16_t adc_filtered_RING, uint16_t pedal_min, uint16_t pedal_max) 
{
    if (adc_filtered_RING <= RING_ADC_MIN) return pedal_min;

    if (adc_filtered_RING >= RING_ADC_MAX) return pedal_max;

    uint32_t mapped_val = (uint32_t)(adc_filtered_RING - RING_ADC_MIN) * (pedal_max - pedal_min) / (RING_ADC_MAX - RING_ADC_MIN) + pedal_min;
    
    return (uint16_t)mapped_val;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
