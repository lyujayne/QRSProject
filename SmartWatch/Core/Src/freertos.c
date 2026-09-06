/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "lcd_init.h"

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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
/* ===== LCD 自检（放任务开头，只执行一次） ===== */
  LCD_Init();              // ST7789 初始化（含复位+初始化序列，内部忙等约300ms）
  LCD_Open_Light();        // 启动背光 PWM（TIM2_CH1）
  LCD_Set_Light(50);       // 亮度 50%

  /* ① 单色填充循环 */
  LCD_Fill(0, 0, LCD_W, LCD_H, RED);
  osDelay(500);
  LCD_Fill(0, 0, LCD_W, LCD_H, GREEN);
  osDelay(500);
  LCD_Fill(0, 0, LCD_W, LCD_H, BLUE);
  osDelay(500);
  LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
  osDelay(500);

  /* ② 四色分区：验证坐标方向/RGB顺序 */
  LCD_Fill(0,   0,   120, 140, RED);
  LCD_Fill(120, 0,   240, 140, GREEN);
  LCD_Fill(0,   140, 120, 280, BLUE);
  LCD_Fill(120, 140, 240, 280, WHITE);
  osDelay(1000);

  /* ③ 图形：线/矩形/圆 */
  LCD_DrawLine(0, 0, 239, 279, YELLOW);
  LCD_DrawRectangle(10, 10, 100, 100, CYAN);
  Draw_Circle(120, 140, 50, MAGENTA);
  osDelay(1000);

  /* ④ 文字+数字 */
  LCD_ShowString(20, 30, (const u8*)"LCD TEST OK", RED, WHITE, 16, 0);
  LCD_ShowIntNum(20, 60, 12345, 5, BLUE, WHITE, 16);

  /* Infinite loop */
  for(;;)
  {
     osDelay(1);


  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

