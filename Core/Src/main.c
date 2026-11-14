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
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


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
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


uint8_t rx_byte;
char buffer[32];
volatile uint8_t idx = 0;              //  текущая позиция в buffer
volatile uint8_t command_ready = 0;
uint32_t last_toggle = 0;
const char alph[33][5]={{0,0,0,0,0},
		{1,3,0,0,0},{3,1,1,1,0},{1,3,3,0,0},{3,3,1,0,0},{3,1,1,0,0},{1,0,0,0,0}, //абвгде
		{1,1,1,3,0},{3,3,1,1,0},{1,1,0,0,0},{1,3,3,3,0},{3,1,3,0,0},{1,3,1,1,0}, //жзийкл
		{3,3,0,0,0},{3,1,0,0,0},{3,3,3,0,0},{1,3,3,1,0},{1,3,1,0,0},{1,1,1,0,0}, //мнопрс
		{3,0,0,0,0},{1,1,3,0,0},{1,1,3,1,0},{1,1,1,1,0},{3,1,3,1,0},{3,3,3,1,0}, //туфхцч
		{3,3,3,3,0},{3,3,1,3,0},{3,1,1,3,0},{3,1,1,3,0},{3,1,3,3,0},{1,1,3,1,1}, //шщьъыэ
		{1,1,3,3,0},{1,3,1,3,0}													 //юя
		};


char out_buf[40];
char local_buf[32];

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){

	if (huart->Instance ==USART2){
        if (rx_byte == '\r' || rx_byte == '\n') {
            if (idx > 0) {
                buffer[idx] = '\0';
                command_ready = 1;

            }
            idx = 0;
        }
        else if (idx < sizeof(buffer) - 1) {
            buffer[idx] = rx_byte;
            idx = idx+1;
        }

        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}


typedef enum {
	    MODE_OFF,
		MODE_ON,
	    MODE_BLINK,
	    MODE_BREATHE,
	    MODE_MORSE,
		MODE_MORSE_READ,
	    MODE_COUNT
	} led_mode;

typedef struct {
	led_mode mode;
	uint32_t channel;
    uint16_t spd;		//скорость
    uint16_t bright;
    uint32_t last_toggle;	//
    signed char dir;
    signed char level;
    char morse_letter;
    uint8_t morse_str[16];
    char morse_idx;
    uint32_t morse_next_time;
    char state;


} Led;


void off_tick(Led*led) {
	__HAL_TIM_SET_COMPARE(&htim2, led->channel, 0);
	led->state = 0;
};

void on_tick(Led*led) {
	__HAL_TIM_SET_COMPARE(&htim2, led->channel, led->bright);
	led->state = 1;
};
void blink_tick(Led*led) {
	if (HAL_GetTick() - led->last_toggle >= led->spd) {
		__HAL_TIM_SET_COMPARE(&htim2, led->channel, led->bright*led->state);
		if (led->state >0 ){
			led->state = 0;
		}
		else{
			led->state=1;
		}
		led->last_toggle = HAL_GetTick();

	}
};

void breathe_tick(Led*led) {
	if (HAL_GetTick() - led->last_toggle >= led->spd/10){
		__HAL_TIM_SET_COMPARE(&htim2, led->channel, led->bright*led->level/9);
		led->level = led->level + led->dir;
		if (led->level == 9 || led->level == 0){
		    led->dir = -led->dir;
		}
		led->last_toggle = HAL_GetTick();
		}
}

void morse_tick(Led*led) {


	if (led->morse_idx!=sizeof(alph[led->morse_str[0]])/sizeof(alph[led->morse_str[0]][0])){
			if (led->state==1){
				if (HAL_GetTick() - led->last_toggle >= led->spd*alph[led->morse_str[led->morse_letter]][led->morse_idx]/10){
					__HAL_TIM_SET_COMPARE(&htim2, led->channel, 0);
					led->state=0;
					led->morse_idx+=1;
					led->last_toggle = HAL_GetTick();
				}
			}
			else{
				if (HAL_GetTick() - led->last_toggle >= led->spd/10){
					__HAL_TIM_SET_COMPARE(&htim2, led->channel, led->bright);
					led->state=1;
					led->last_toggle = HAL_GetTick();

				}
			}
		}
		else{
			if (led->state==0){
				if (HAL_GetTick() - led->last_toggle >= led->spd*sizeof(alph[led->morse_str[0]])/sizeof(alph[led->morse_str[0]][0])/10){
					__HAL_TIM_SET_COMPARE(&htim2, led->channel, led->bright);
					led->state=1;
					led->morse_letter = (led->morse_letter + 1) % (sizeof(alph[led->morse_str[0]])/sizeof(alph[led->morse_str[0]][0]));
					led->morse_idx=0;
					led->last_toggle = HAL_GetTick();
				}
			}
		}




}


void morse_read(Led*led){
	if (!command_ready) return;
	command_ready = 0;
	char local[32];

	strcpy(local, buffer);
	strcpy(out_buf, local);
	strcat(out_buf, "\r\n");
	HAL_UART_Transmit_IT(&huart2, (uint8_t*)out_buf, strlen(out_buf));

	char i = 0;
	char j = 0;
	while(local[i] != '\0'){
		uint8_t b1 =(uint8_t)local[i];
		uint8_t b2;
		if(b1 >= 0x80){
			 b2 =(uint8_t)local[i+1];
			//i=i+1;
		}

		if (b1 == 0xD0 && b2 == 0x81) { // Ё
		            led->morse_str[j] = 5;
		            j++;
		        }
		        else if (b1 == 0xD1 && b2 == 0x91) { //  ё
		            led->morse_str[j] = 6;
		            j++;
		        }
		        else if ((b2 >> 4) == 9 || (b2 >> 4) == 0xB) { // А-П
		            led->morse_str[j] = (b2 & 0x0F)+1;
		            j++;
		        }
		        else if ((b2 >> 4) == 8 || (b2 >> 4) == 0xA) { // Р-Я
		            led->morse_str[j] = (b2 & 0x0F) + 17;
		            j++;
		        }
		i = i+2;
		//j = j+1;
	}
	led_switch_mode(led, MODE_MORSE);
}



void led_switch_mode(Led*led, led_mode new_mode){
	switch(led->mode){
	case MODE_MORSE:off_tick(led); break;
	case MODE_BREATHE:off_tick(led); break;
	default: break;
	}
	led->mode=new_mode;

	switch(led->mode){
	case MODE_MORSE_READ: memset(led->morse_str, 0, sizeof(led->morse_str)); break;
	case MODE_MORSE: off_tick(led); led->morse_idx = 0; led->morse_next_time = HAL_GetTick(); break;
	case MODE_BREATHE: led->level = 1; led->dir = 1; break;
	case MODE_BLINK: led->state = 0; led->last_toggle = HAL_GetTick();; break;
	}


}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */


	Led led[3] = {
	        { .channel = TIM_CHANNEL_1, .bright = 500, .spd = 500, .state = 0, .dir = 1, .level = 1, .morse_idx=0, .morse_next_time=0, .morse_letter=0},
	        { .channel = TIM_CHANNEL_2, .bright = 500, .spd = 500, .state = 0, .dir = 1, .level = 1, .morse_idx=0, .morse_next_time=0, .morse_letter=0},
			{ .channel = TIM_CHANNEL_3, .bright = 500, .spd = 500, .state = 0, .dir = 1, .level = 1, .morse_idx=0, .morse_next_time=0, .morse_letter=0},
	    };
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
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	    if (command_ready) {
	        command_ready = 0;

	        strcpy(out_buf, buffer);
	        strcat(out_buf, "\r\n");
	        HAL_UART_Transmit_IT(&huart2, (uint8_t*)out_buf, strlen(out_buf));



	        strcpy(local_buf, buffer);
	        char *ld = strtok(local_buf, " ");
	        char *cmd = strtok(NULL, " ");
	        char *arg_spd = strtok(NULL, " ");
	        char *arg_brg = strtok(NULL, " ");
	        if(cmd !=NULL){
	        	led_mode parsed = MODE_COUNT;
	        	if (strcmp(cmd, "off") == 0)     parsed = MODE_OFF;
	        	if (strcmp(cmd, "on") == 0)     parsed = MODE_ON;
	        	if (strcmp(cmd, "blink") == 0)   parsed = MODE_BLINK;
	        	if (strcmp(cmd, "breathe") == 0) parsed = MODE_BREATHE;
	        	if (strcmp(cmd, "morse") == 0)   parsed = MODE_MORSE_READ;

	   	        if (parsed != MODE_COUNT) {
	   	        	led_switch_mode(&led[atoi(ld)-1], parsed);
	        	}
	        }


	        if (cmd != NULL){
	        	if (arg_spd != NULL){
	        		led[atoi(ld)-1].spd = atoi(arg_spd);
	        	}
	        }
	        if (cmd != NULL){
	        	if (arg_brg != NULL){
	        		led[atoi(ld) -1].bright = atoi(arg_brg);
	        	}
	        }


	    }


	    for (int i = 0; i<3;i+=1){
	    	switch(led[i].mode){
	    	case MODE_OFF:off_tick(&led[i]); break;
	    	case MODE_ON:on_tick(&led[i]); break;
	    	case MODE_BLINK:blink_tick(&led[i]); break;
	    	case MODE_BREATHE:breathe_tick(&led[i]); break;
	    	case MODE_MORSE_READ:morse_read(&led[i]); break;
	    	case MODE_MORSE:morse_tick(&led[i]); break;

	    	default:break;
	    	}
	    }
  }






    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


}
  /* USER CODE END 3 */


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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

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
