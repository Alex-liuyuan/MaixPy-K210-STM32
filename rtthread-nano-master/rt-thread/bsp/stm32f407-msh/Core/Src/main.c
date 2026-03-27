/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include <rtthread.h>
#include "maix_runtime_app.h"
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

/* Private function prototypes -----------------------------------------------*/
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = BOARD_LED_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOARD_LED_GPIO_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(BOARD_LED_GPIO_PORT, BOARD_LED_PIN, GPIO_PIN_RESET);
}

void maix_board_app_init(void)
{
  /*
   * RT-Thread 已经在 entry() -> rtthread_startup() -> rt_hw_board_init()
   * 中完成 HAL_Init/SystemClock_Config/SysTick 初始化。这里仅做应用层外设配置。
   */
  MX_GPIO_Init();
}

void maix_board_fill_profile(maix_runtime_profile_t *profile)
{
  if (profile == RT_NULL)
  {
    return;
  }

  profile->board_name = "stm32f407_nucleo";
  profile->runtime_name = "rtthread/stm32f407";
  profile->console_name = "USART2 @ 115200 8N1";
  profile->cpu_arch = "arm-cortex-m4";
  profile->led = MAIX_CAP_AVAILABLE;
  profile->camera = MAIX_CAP_PLANNED;
  profile->display = MAIX_CAP_PLANNED;
}

void maix_board_heartbeat(unsigned long heartbeat_count)
{
  (void)heartbeat_count;
  HAL_GPIO_TogglePin(BOARD_LED_GPIO_PORT, BOARD_LED_PIN);
}

int main(void)
{
  return maix_runtime_main();
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
