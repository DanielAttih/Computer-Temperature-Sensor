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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "fonts.h"
#include "MCP9808.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TEMP_HOT_F	90.0f 	/* red LED turns ON at or above this */
#define TEMP_OK_F	85.0f	/* red LED turns OFF only below this	 */

#define MAX_READ_FAILS	3	/* consecutive failed reads before declaring a fault */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
void debug_print(const char *msg);
void myprintf(const char *fmt, ...);

//----------------------- Variables for OLED Display-------------------

char buf_C [16]; // Temperature in Celsius
char buf_F [16]; // Temperature in Fahrenheit

//----------------------- Variables for temperature logic and fault tolerance-----------------------
bool tooHot = false;	/* remembers the current state between loop passes */

uint8_t failCount   = 0;       /* consecutive failed reads so far */
bool sensorFault 	= false;   /* true while the fault is active   */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C3_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void debug_print(const char *msg)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
}

void myprintf(const char *fmt, ...)
{
  static char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  int len = strlen(buffer);
  HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

}
 // Helper functions that redraws the OLED's fixed text after error screen wipes it
static void oled_draw_static(void)
{
    SSD1306_Fill(SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(0, 0);
    SSD1306_Puts("PC Case Temp", &Font_7x10, 1);
    SSD1306_GotoXY(0, 10);
    SSD1306_Puts("Temp: ", &Font_7x10, 1);
}
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
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  debug_print("\r\n ~ START ~\r\n\r"); // for PuTTY

  /*------------------------------------------------- Fault tolerance for startup----------------------------------------------------------------*/
  /* ---- MCP9808: retry forever. Attempts 1-3 are silent; from attempt 4 on,
   *      the red LED flashes between retries until the sensor answers. ---- */
  uint8_t attempts = 0;
  while (!MCP9808_Init(&hi2c3))
  {
	  /* Attempts is a uint8_t, so after 255 failed attempts it wraps to 0 and the red LED stops flashing for 3 passes.
	   * Cap it at 255, so it stays at 255, soo attempts >= 3 stay true and the LEED keeps flashing 					*/
	  if (attempts < 255) attempts++;
	  myprintf("MCP9808 FAIL(attempt %u)\r\n", attempts);
	  if (attempts >= 3)
	  {
		  HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
	  }
	  HAL_Delay(250);
  }
  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);  /* stop flashing */
  myprintf("MCP9808 OK\r\n");

  /* ---- SSD1306: 3 attempts, then continue without the display ---- */
  bool oledOK = false;
  for (uint8_t i = 0; i < 3 && !oledOK; i++) {
      oledOK = SSD1306_Init();            /* returns 1 on success, 0 on failure */
      if (!oledOK) {
          myprintf("SSD1306 FAIL (attempt %u)\r\n", i + 1);
          HAL_Delay(100);
      }
  }

  if (oledOK) {
      myprintf("SSD1306 OK\r\n");
      oled_draw_static();
  } else {
      myprintf("SSD1306 FAIL - continuing without display\r\n");
  }
/*---------------------------------------------------------------------------------------------------------------------------------------------*/
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /* Blue LED: solid = display OK, flashing = display failed.
	   * The loop runs every 500 ms, so toggling here gives a steady blink. */
	  if (oledOK) {
	     HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_SET);
	  } else {
	     HAL_GPIO_TogglePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin);
	  }

	  float tempC;		// receives the temperature in Celsius
	  float tempF; 		// receives the temperature in Fahrenheit

	  if (MCP9808_ReadTempC(&hi2c3, &tempC, &tempF)) // &tempC = "address of tempC"
	  {												 // &tempF = "address of tempF"

		  failCount = 0;			/* a good read resets the streak */

		  if (sensorFault)			/* we just recovered from a fault */
		  {
			  sensorFault = false;
			  myprintf("Sensor recovered\r\n");
			  if (oledOK)
			  {
				  oled_draw_static();		/* redraw the title and label */
			  }

		  }

		  if(oledOK)
		  {
			  // Print in Celsius
			  SSD1306_GotoXY(42, 10);
			  snprintf(buf_C, sizeof(buf_C), "%.2f C", tempC);
			  SSD1306_Puts(buf_C, &Font_7x10, 1);

			  // Print in Fahrenheit
			  SSD1306_GotoXY(42, 20);
			  snprintf(buf_F, sizeof(buf_F), "%.2f F", tempF);
			  SSD1306_Puts(buf_F, &Font_7x10, 1);

		  }

		  // Debug print
		  myprintf("\nTemp: %.2f C\r\n", tempC);
		  myprintf("%.2f F\r\n", tempF);

		  /* Hysteresis: only change state when a threshold is crossed */
		  if (!tooHot && tempF >= TEMP_HOT_F)
		  {
			 tooHot = true;
		  } else if (tooHot && tempF < TEMP_OK_F)
		  {
			  tooHot = false;
		  }

		  HAL_GPIO_WritePin(RED_LED_GPIO_Port,   RED_LED_Pin,   tooHot ? GPIO_PIN_SET   : GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, tooHot ? GPIO_PIN_RESET : GPIO_PIN_SET);

	  } else {
	      myprintf("Temp read failed (%u)\r\n", failCount + 1);

	      /* Reset the I2C3 peripheral so it can recover after a bus glitch */
	      HAL_I2C_DeInit(&hi2c3);
	      MX_I2C3_Init();

	      if (failCount < MAX_READ_FAILS)
	      {
	    	  failCount++;
	      }

	      if (failCount >= MAX_READ_FAILS)
	      {
	    	  if (!sensorFault)				/* first pass of the fault only */
	    	  {
	    		  sensorFault =  true;
	    		  tooHot = false;			/* forget the old hot/ok state */
	    		  HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);

	    		  if(oledOK)
	    		  {
	    			  SSD1306_Fill(SSD1306_COLOR_BLACK);
	    			  SSD1306_GotoXY(0, 0);
	    			  SSD1306_Puts("PC Case Temp", &Font_7x10, 1);
	    			  SSD1306_GotoXY(0, 20);
	    			  SSD1306_Puts("SENSOR ERR", &Font_7x10, 1);
	    		  }
	    	  }

	    	  HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);   /* flash red */
	      }
	  }

	  if(oledOK)
	  {
		  SSD1306_UpdateScreen();
	  }

	  HAL_Delay(500);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x0060112F;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x0060112F;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, BLUE_LED_Pin|RED_LED_Pin|GREEN_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BLUE_LED_Pin RED_LED_Pin GREEN_LED_Pin */
  GPIO_InitStruct.Pin = BLUE_LED_Pin|RED_LED_Pin|GREEN_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
