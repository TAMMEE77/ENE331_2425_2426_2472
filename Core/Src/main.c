/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

// Delay function to wait for specified microseconds
void delay_us(uint32_t us)
{
  for (uint32_t i = 0; i < (us * 72); i++)  // 72 cycles per microsecond for 72 MHz system clock
  {
    __NOP(); // No operation
  }
}

uint8_t ledPattern[] = {
    0b001,  // State 0 → PA1
    0b010,  // State 1 → PA2
    0b100,  // State 2 → PA3
    0b111   // State 3 → PA1 + PA2 + PA3
};

uint8_t state = 0;
uint8_t lastBtn = 1;  // Variable to store the last button state
uint8_t buttonPressed = 0;  // Variable to check if the button has been pressed
uint8_t transitionInProgress = 0;  // Variable to check if a state change is in progress

int main(void)
{
  HAL_Init();  // Initialize the HAL library
  SystemClock_Config();  // Configure the system clock

  // Enable GPIOA and GPIOB clocks
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

  // Set PA6 as output for LED
  GPIOA->MODER &= ~(0b11 << (6 * 2));
  GPIOA->MODER |=  (0b01 << (6 * 2));
  GPIOA->OTYPER &= ~(1 << 6);  // Set PA6 as push-pull output
  GPIOA->PUPDR  &= ~(0b11 << (6 * 2));  // No pull-up or pull-down
  GPIOA->OSPEEDR |= (0b01 << (6 * 2));  // Low speed

  // Set PA1, PA2, PA3 as output for LED array
  GPIOA->MODER &= ~(0b11 << 2) & ~(0b11 << 4) & ~(0b11 << 6);
  GPIOA->MODER |= (0b01 << 2) | (0b01 << 4) | (0b01 << 6);

  // Set PA0 as input with pull-up
  GPIOA->MODER &= ~(0b11 << 0);
  GPIOA->PUPDR &= ~(0b11 << 0);
  GPIOA->PUPDR |= (0b01 << 0);

  // Configure PB10 for TIM2 CH3 (Timer output)
  GPIOB->MODER &= ~(0b11 << (10 * 2));
  GPIOB->MODER |= (0b10 << (10 * 2));  // Alternate function mode
  GPIOB->AFR[1] &= ~(0b1111 << ((10 - 8) * 4));
  GPIOB->AFR[1] |= (0b0001 << ((10 - 8) * 4));  // Set AF1 for TIM2 CH3

  // Set up Timer2 to generate 1.2 ms period for PA6
  TIM2->PSC = 71;  // Prescaler value, Timer clock = 1 MHz
  TIM2->ARR = 1200 - 1;  // Auto-reload value for 1.2 ms (1 MHz / 1200 = 1 ms)
  TIM2->CCR3 = 600;  // Duty cycle 50% (600 us high time)
  TIM2->CCMR2 &= ~(0b111 << 4);
  TIM2->CCMR2 |= (0b110 << 4);  // PWM mode
  TIM2->CCMR2 |= TIM_CCMR2_OC3PE;  // Enable preload
  TIM2->CCER |= TIM_CCER_CC3E;  // Enable capture compare on CH3
  TIM2->CR1 |= TIM_CR1_ARPE;  // Enable auto-reload preload
  TIM2->EGR |= TIM_EGR_UG;  // Generate update event to load values
  TIM2->CR1 |= TIM_CR1_CEN;  // Start the timer

  while (1)
  {
    GPIOA->ODR ^= (1 << 6);  // Toggle PA6 LED (flashing)
    delay_us(1200);  // Delay for 1.2 ms to create flashing effect

    uint8_t btn = (GPIOA->IDR >> 0) & 0x01;  // Read button state (PA0)

    // Detect button press (from high to low)
    if (btn == 0 && lastBtn == 1 && !buttonPressed && !transitionInProgress)
    {
      buttonPressed = 1;  // Set buttonPressed flag to true
      transitionInProgress = 1;  // Set transition flag to true

      state++;  // Increment the state
      if (state >= sizeof(ledPattern)) state = 0;  // Reset state if out of bounds

      GPIOA->ODR &= ~(0b111 << 1);  // Clear PA1, PA2, PA3
      GPIOA->ODR |= ((ledPattern[state]) << 1);  // Set new state

      // No delay after button press to show state immediately
    }

    // Reset buttonPressed flag when button is released
    if (btn == 1)
    {
      buttonPressed = 0;
      transitionInProgress = 0;  // Reset transition flag
    }

    lastBtn = btn;  // Update last button state
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* Enable GPIO Ports Clock */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/**
  * @brief Error handler
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
