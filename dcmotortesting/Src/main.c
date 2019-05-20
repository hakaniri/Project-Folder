/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "lcd_i2cModule.h"
#include "dwt_stm32_delay.h" //microsecond delay for temperature sensor

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SLAVE_ADDRESS_LCD 0x4E

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else 
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /*  GNUC */

PUTCHAR_PROTOTYPE {
      HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 10);
 return ch;
}

uint32_t echo1, echo2, echo3, delay;
float dis1, dis2, dis3;

char* buffer = "Hello!\n\r";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
uint32_t val = 0;


uint8_t Rh_byte1, Rh_byte2, Temp_byte1, Temp_byte2;
uint16_t sum, RH, TEMP;
int temp_low, temp_high, rh_low, rh_high;
char temp_char1[2], temp_char2, rh_char1[2], rh_char2;
uint8_t check = 0;

GPIO_InitTypeDef GPIO_InitStruct;
void set_gpio_output (void)
{
	/*Configure GPIO pin output: PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void set_gpio_input (void)
{
	/*Configure GPIO pin input: PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void DHT11_start (void)
{
	set_gpio_output ();  // set the pin as output
	HAL_GPIO_WritePin (GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);   // pull the pin low
	DWT_Delay_us (18000);   // wait for 18ms
	set_gpio_input ();   // set as input
}

void check_response (void)
{
	DWT_Delay_us (40);
	if (!(HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1)))
	{
		DWT_Delay_us (80);
		if ((HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1))) check = 1;
	}
	while ((HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1)));   // wait for the pin to go low
}

uint8_t read_data (void)
{
	uint8_t i,j;
	for (j=0;j<8;j++)
	{
		while (!(HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1)));   // wait for the pin to go high
		DWT_Delay_us (40);   // wait for 40 us
		if ((HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1)) == 0)   // if the pin is low 
		{
			i&= ~(1<<(7-j));   // write 0
		}
		else i|= (1<<(7-j));  // if the pin is high, write 1
		while ((HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_1)));  // wait for the pin to go low
	}
	return i;
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
  MX_TIM9_Init();	 // front motors
  MX_TIM12_Init(); //back motors
  MX_TIM11_Init(); // trigger timer
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();  // echo timer
  /* USER CODE BEGIN 2 */
	__HAL_UART_ENABLE_IT(&huart2, UART_IT_TC);
	//lcd initialize
	LCD_i2cDeviceCheck();		
	LCD_Init();
  LCD_BackLight(LCD_BL_ON);
	LCD_SetCursor(1,1);
	
	HAL_TIM_Base_Start(&htim11);
	
	HAL_TIM_Base_Start_IT(&htim9);
	HAL_TIM_Base_Start_IT(&htim12);
	
	HAL_TIM_PWM_Start(&htim11, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1); 
  HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
	
	HAL_TIM_IC_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_IC_Start(&htim1, TIM_CHANNEL_2);
	
	TIM11->CCR1 = 3;
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		if (HAL_GetTick() - delay >= 100) { //ultrasonic sensor
			
			delay = HAL_GetTick();
			echo1 = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_1);
			echo2 = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_2);

			dis1 = echo1 / 58.0f;
			dis2 = echo2 / 58.0f;

		}
		
//	LCD_SetCursor(1,1);
//	LCD_Print("Distance: %.2f", dis1); //Example of sending distance.
//	HAL_Delay(500);
//	LCD_Clear();
//	HAL_Delay(20);
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		//temperature and humidity sensor
		DHT11_start ();
		check_response ();
		Rh_byte1 = read_data ();
		Rh_byte2 = read_data ();
		Temp_byte1 = read_data ();
		Temp_byte2 = read_data ();
		sum = read_data();
		if (sum == (Rh_byte1+Rh_byte2+Temp_byte1+Temp_byte2)){    // if the data is correct
		
			int temp1 = (Temp_byte1/10)+48;

			LCD_Clear();
			HAL_Delay(20);
			lcd_send_cmd (0x80);
			lcd_send_string ("TMP: ");
			lcd_send_data ((Temp_byte1/10)+48);
			lcd_send_data ((Temp_byte1%10)+48);
			lcd_send_string (" C");
			
			lcd_send_cmd (0xC0);
			lcd_send_string ("HUM: ");
			lcd_send_data ((Rh_byte1/10)+48);
			lcd_send_data ((Rh_byte1%10)+48);
			lcd_send_data ('%');		
			HAL_Delay(1000);
				
			HAL_UART_Transmit_IT(&huart2, (uint8_t *)&temp1, 8);
		
			if( temp1 > 2 ){
				HAL_Delay(1000);
				HAL_SuspendTick();
				HAL_PWR_EnterSLEEPMode( PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI); //standby mode
				HAL_ResumeTick();
			}
		}
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int speed=50000;
int speed2=60000;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	
  if(val == 0){//forward
	TIM12->CCR1 = speed;	
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_9,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_10,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_13,GPIO_PIN_RESET);
		
		TIM9->CCR1 = speed;	
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOE,GPIO_PIN_6,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_SET);
		
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_15, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12, GPIO_PIN_RESET);
		
		
		val++;
	} 
	
	else if(val == 1){//left
		
		TIM12->CCR1 = speed2;	
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_9,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_10,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_13,GPIO_PIN_RESET);
		
		TIM9->CCR1 = speed2;	
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE,GPIO_PIN_6,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_SET);
	
	  HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_15, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12, GPIO_PIN_SET);;
		
		val ++;
} 
	
else if(val == 2){ //right
		TIM12->CCR1 = speed2;	
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_9,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_10,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_13,GPIO_PIN_SET);
		
		TIM9->CCR1 = speed2;	
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOE,GPIO_PIN_6,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
	
	  HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_15, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12, GPIO_PIN_RESET);;
		
		val++;
} 
	
else if(val == 3){ //stop
		TIM12->CCR1 = 0;	
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_9,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_10,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_13,GPIO_PIN_RESET);
		
		TIM9->CCR1 = 0;	
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE,GPIO_PIN_6,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
	
	  HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_15, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12, GPIO_PIN_SET);
		val++;
		
} 

	else if(val == 4){ //backward
		TIM12->CCR1 = speed;	
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_9,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_10,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_10,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_13,GPIO_PIN_SET);
		
		TIM9->CCR1 = speed;	
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE,GPIO_PIN_6,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
	
	  HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_15, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12, GPIO_PIN_RESET);;
		
		val=0;
} 
	
		
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
