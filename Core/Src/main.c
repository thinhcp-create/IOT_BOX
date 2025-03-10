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
#include "fatfs.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FAT12.h"
#include "usbd_storage_if.h"
#include "espcomm.h"
#include "usbd_core.h"
#include "FLASH_PAGE_F1.h"
#include "time.h"
#include "devicetype.h"

#define ADDR_APP_PROGRAM 0x08005000
#define CRC32_POLYNOMIAL 0x04C11DB7

extern uint8_t flag_handle_csv;
extern uint8_t buffer[];
extern uint32_t usbTick;
extern uint8_t usbStatus;
extern uint32_t g_24V_mV;
extern uint32_t g_4V2_mV;
//uint8_t usb_reconnect=0;
char g_rx1_char;
uint8_t g_debugEnable=0;
Time hallet_time={

                .year=2014,
                .month=11,
                .day = 25,
                .hour = 16,
                .minute = 47,
                .second = 30
        };
uint32_t g_NbSector;
uint8_t g_forcesend=0;
uint8_t g_isMqttPublished=0;
uint32_t g_espcomm_tick=0;
uint32_t g_device_tick=0;
uint32_t g_adc_tick=0;
LIFO_inst g_q;
uint32_t SD_DATA_SECTOR_BEGIN =0;
uint32_t SD_DATA_SECTOR_END =0;
//Dành cho ota
extern uint8_t g_ota;
extern uint8_t Timer_frame_ota;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
volatile uint8_t SCI1_rxdone=0;
uint16_t g_rx1_cnt;
uint8_t cntTimeRev1;
char g_rx1_buffer[MAX_BUFFER_UART1];
uint8_t usb_retry=0;
//RTC_TimeTypeDef sTime = {0};
//RTC_DateTypeDef sDate={0};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

CRC_HandleTypeDef hcrc;

SD_HandleTypeDef hsd;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_CRC_Init(void);
static void MX_TIM3_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
/* USER CODE BEGIN PFP */
Time g_time ={0};
uint16_t g_sec_flag =0;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_IncTick(void)
{
  uwTick += uwTickFreq;
  g_sec_flag++;
  	if(g_sec_flag==1000)
  	{
  		g_sec_flag=0;
  		g_time.second++;
  		if (g_time.second == 60)
  		{
  			g_time.second = 0;
  			g_time.minute++;
  			if (g_time.minute == 60)
  			{
  				g_time.minute = 0;
  				g_time.hour++;
  				if (g_time.hour == 24)
  				{
  					g_time.hour = 0;
  					g_time.day++;
  					if ((g_time.day == 32 && (g_time.month == 1 || g_time.month == 3 ||g_time.month == 5 ||g_time.month == 7
  					||g_time.month == 8 || g_time.month == 10 ||g_time.month == 12))
  					|| (g_time.day == 31 && (g_time.month == 4 || g_time.month == 6 ||g_time.month == 9 ||g_time.month == 11))
  					||(g_time.day == 30 && g_time.month == 2 && g_time.year % 4 == 0)
  					||(g_time.day == 29 && g_time.month == 2 && g_time.year % 4 > 0))
  					{
  						g_time.day = 1;
  						g_time.month++;
  						if (g_time.month == 13)
  						{
  							g_time.month = 1;
  							g_time.year++;
  						}
  					}
  				}
  			}
  		}
  	}
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	SCI1_rxdone=1;
	HAL_UART_Receive_IT(&huart1,(uint8_t*)&g_rx1_char,sizeof(g_rx1_char));
		if(g_rx1_cnt < MAX_BUFFER_UART1)
		{
			g_rx1_buffer[g_rx1_cnt++] = g_rx1_char;
		}
		else{
			g_rx1_cnt = 0;
			memset(g_rx1_buffer,0,sizeof(g_rx1_buffer));
		}
		cntTimeRev1 = RECV_END_TIMEOUT;
}

uint32_t crc32_table[256]={0};

// Khởi tạo bảng tra cứu CRC32
void generate_crc32_table() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i << 24;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
        crc32_table[i] = crc;
    }
}

// Hàm tính CRC32 từ mảng bất kỳ
uint32_t calculate_crc32(const void *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF; // Giá trị khởi tạo
    const uint8_t *byte_data = (const uint8_t *)data; // Chuyển dữ liệu thành mảng uint8_t

    for (size_t i = 0; i < length; i++) {
        uint8_t lookup_index = (crc >> 24) ^ byte_data[i];
        crc = (crc << 8) ^ crc32_table[lookup_index];
    }

    return crc ^ 0xFFFFFFFF; // XOR với 0xFFFFFFFF để lấy kết quả cuối
}
extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	SCB->VTOR = (uint32_t)ADDR_APP_PROGRAM ;
	__enable_irq();
	create_fat12_disk(buffer,STORAGE_BLK_SIZ,STORAGE_BLK_NBR );
	generate_crc32_table();

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
  MX_USB_DEVICE_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  MX_USART2_UART_Init();
  MX_SDIO_SD_Init();
  MX_CRC_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  htim3.Instance->CCR4 = 1000;
  LoadPointer(&g_q);
  EspComm_init();
  if(BSP_SD_Init()==MSD_OK)
  {
	 mqtt_debug_send("SD card available\n");
	 g_NbSector = hsd.SdCard.BlockNbr;
	 SD_DATA_SECTOR_END = g_NbSector;
	 SD_DATA_SECTOR_BEGIN = g_NbSector - 1024*1024;
  }
  else
  {
	 mqtt_debug_send("SD card not available\n");
  }
  RAM_FATFS_Init();
// Để hallet nhận diện lại mạch là usb vì lúc boot mạch đã nhận diện được mạch ko là usb và sẽ ko refresh
  HAL_PCD_MspDeInit(&hUsbDeviceFS);
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,0);
  HAL_GPIO_WritePin(PW_USB_GPIO_Port,PW_USB_Pin,1);
  HAL_Delay(500);
  MX_USB_DEVICE_Init();
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,1);
  HAL_GPIO_WritePin(PW_USB_GPIO_Port,PW_USB_Pin,0);
  HAL_Delay(500);
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,0);
  HAL_GPIO_WritePin(PW_USB_GPIO_Port,PW_USB_Pin,1);
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADCEx_Calibration_Start(&hadc2);
  VoltMeasure();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(HAL_GetTick()-g_espcomm_tick>ESP_COMM_PERIOD)
	  {
		  EspCmdHandler();
		  g_espcomm_tick = HAL_GetTick();
	  }
	  if((HAL_GetTick()-g_device_tick>DEVICE_HANDLER_PERIOD) && (g_ota==0||Timer_frame_ota == 0) )
	  {

		  Device_Handler();
//		  if(HAL_GPIO_ReadPin(LED_GPIO_Port, LED_Pin) == 0)
//			  {
//			  	  //cần xử lý lại cho mượt tránh ảnh hưởng đến kết thúc ota
//			  	  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,1);
//			  }
		  g_device_tick = HAL_GetTick();
		  // Reset usb khi mất kết nối 60s
		  if(HAL_GetTick()-usbTick > 60000 && usbStatus == 1)
		  {
			  mqtt_debug_send("Usb reset\n");
			  usbTick = HAL_GetTick();
			  usb_retry ++;
			  if(usb_retry>=6) HAL_NVIC_SystemReset();
			  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,1);
			  HAL_GPIO_WritePin(PW_USB_GPIO_Port,PW_USB_Pin,0);
			  HAL_PCD_MspDeInit(&hUsbDeviceFS);
			  MX_USB_DEVICE_Init();
			  HAL_Delay(1000);
			  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,0);
			  HAL_GPIO_WritePin(PW_USB_GPIO_Port,PW_USB_Pin,1);

		  }
	  }
	  if(HAL_GetTick()-g_adc_tick>3000)
	  {
		  VoltMeasure();
		  g_adc_tick = HAL_GetTick();
	  }

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 16;
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 18000-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 2000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(PW_USB_GPIO_Port, PW_USB_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, USB_PWR_EN_Pin|RS485_DE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RST_WIFI_GPIO_Port, RST_WIFI_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_Pin|DO1_Pin|DO2_Pin|DO3_Pin
                          |DO4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PW_USB_Pin */
  GPIO_InitStruct.Pin = PW_USB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PW_USB_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DI4_Pin DI3_Pin DI2_Pin DI1_Pin */
  GPIO_InitStruct.Pin = DI4_Pin|DI3_Pin|DI2_Pin|DI1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_PWR_EN_Pin RS485_DE_Pin RST_WIFI_Pin */
  GPIO_InitStruct.Pin = USB_PWR_EN_Pin|RS485_DE_Pin|RST_WIFI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_Pin DO1_Pin DO2_Pin DO3_Pin
                           DO4_Pin */
  GPIO_InitStruct.Pin = LED_Pin|DO1_Pin|DO2_Pin|DO3_Pin
                          |DO4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void VoltMeasure()
{
	uint32_t ADC_24V;
	uint32_t ADC_4V2;
	HAL_ADC_Start(&hadc1);
	// Chờ quá trình chuyển đổi hoàn thành (Timeout = 10ms)
	if (HAL_ADC_PollForConversion(&hadc1,10) == HAL_OK) {
	    // Đọc giá trị ADC
		ADC_4V2 = HAL_ADC_GetValue(&hadc1);
		g_4V2_mV = ((uint32_t)ADC_4V2 * 3300 * 2 + 2048) / 4096;
	}
	else mqtt_debug_send("Adc 4v2 error\n");
	HAL_ADC_Stop(&hadc1);
	HAL_ADC_Start(&hadc2);
	if (HAL_ADC_PollForConversion(&hadc2,10) == HAL_OK) {
		    // Đọc giá trị ADC
		ADC_24V = HAL_ADC_GetValue(&hadc2);
		g_24V_mV = ((uint32_t)ADC_24V * 3300 * 11 + 2048) / 4096;
	}
	else mqtt_debug_send("Adc 24v error\n");
	HAL_ADC_Stop(&hadc2);

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
  HAL_NVIC_SystemReset();
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
