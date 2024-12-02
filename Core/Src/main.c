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

#define ADDR_APP_PROGRAM 0x08008000
#define CRC32_POLYNOMIAL 0x04C11DB7

extern uint8_t flag_handle_csv;
extern uint8_t buffer[];
uint8_t flag_readSD=0;
char g_rx1_char;
uint8_t g_debugEnable=0;
rtc_time g_time=
		{
				.year=2024,
				.month=11,
				.date = 25,
				.hour = 16,
				.min = 47,
				.sec = 30
		};

uint32_t g_NbSector;
uint8_t g_forcesend=0;
uint8_t g_isMqttPublished=0;
uint32_t g_espcomm_tick=0;
uint32_t time_force_send=0;
uint32_t time_wdi=0;

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

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

SD_HandleTypeDef hsd;

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
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

DIR dir;
FILINFO fno;
FRESULT fresult;
FATFS *pfs;
DWORD fre_clust;

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
  /* USER CODE BEGIN 2 */
  EspComm_init();
//  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,0);
//  HAL_Delay(1000);
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,1);
  if(BSP_SD_Init()==MSD_OK)
  {
	 mqtt_debug_send("SD card available\n");
  }
  else
  {
	 mqtt_debug_send("SD card not available\n");
  }
  RAM_FATFS_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (flag_handle_csv ==1 &&(g_ota==0||Timer_frame_ota == 0) )
	  	  {
	  		  FS_FileOperations();
	  	  	  flag_handle_csv =0;
	  	  }
	  if(HAL_GetTick()-time_wdi >WDI_PERIOD)
	  	  {
	  		  time_wdi= HAL_GetTick();
	  		  HAL_GPIO_TogglePin(WDI_GPIO_Port,WDI_Pin);
	  	  }
	  if (flag_readSD ==1 &&(g_ota==0||Timer_frame_ota == 0) )
	  	  	  {
	  	  		  SD_FileOperations();
	  	  		  flag_readSD =0;
	  	  	  }
	  if(HAL_GetTick()-g_espcomm_tick>ESP_COMM_PERIOD)
	  		{
	  			EspCmdHandler();
	  			g_espcomm_tick = HAL_GetTick();
	  		}

	  if(HAL_GetTick()-time_force_send >ESP_FORCESEND_PERIOD&&(g_ota==0||Timer_frame_ota == 0))
	  {
		  time_force_send = HAL_GetTick();
		  g_debugEnable=1;
		  debugPrint("M[%d] Force Send = %d",HAL_GetTick()/1000,g_forcesend);
		  g_debugEnable=0;
		  uint8_t data_force_send[50];
		  sprintf(data_force_send,"%04d%02d%02d%02d%02d%02d\tdi1:%01d\tdi2:%01d\tdi3:%01d\tdi4:%01d\t",g_time.year,g_time.month,g_time.date,g_time.hour,g_time.min,g_time.sec,HAL_GPIO_ReadPin(DI1_GPIO_Port, DI1_Pin),HAL_GPIO_ReadPin(DI2_GPIO_Port, DI2_Pin),HAL_GPIO_ReadPin(DI3_GPIO_Port, DI3_Pin),HAL_GPIO_ReadPin(DI4_GPIO_Port, DI4_Pin));
		  mqtt_data_send((char *)data_force_send);
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, USB_PWR_EN_Pin|RS485_DE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RST_WIFI_GPIO_Port, RST_WIFI_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_Pin|DO1_Pin|DO2_Pin|DO3_Pin
                          |DO4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(WDI_GPIO_Port, WDI_Pin, GPIO_PIN_SET);

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

  /*Configure GPIO pins : LED_Pin WDI_Pin DO1_Pin DO2_Pin
                           DO3_Pin DO4_Pin */
  GPIO_InitStruct.Pin = LED_Pin|WDI_Pin|DO1_Pin|DO2_Pin
                          |DO3_Pin|DO4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
enum alarm  {
            alarm_Low_UV_Lamp=0,
            alarm_IO_I2C_error,
            alarm_Dose,
            alarm_Lamps_not_striking,
            Not_Used_0	,
            alarm_PCB_temp_too_high,
            alarm_System_temp,
            alarm_Lamp_no_output,
            alarm_Front_Panel_Ajar,
            alarm_UV_Sensor,
            alarm_water_temp

};

enum warn  {
          warn_End_of_Lamp_Life=0,              // Warning 1
          Not_Used_1,        //         2 --- Not Used
          warn_Wiper_not_turning,             //         3
          warn_Water_temperature_high,        //         4
          warn_System_temp_limit,             //         5
          warn_Water_temp_sensor,             //         6
          warn_UV_temp_sensor,                //         7
          Not_Used_2,     //         8 --- Not Used
          warn_System_temp_sensor,            //         9
          warn_Lamp_Lifetime                 //         10
};
uint16_t  getValue(const char *str, const char *label) {
    char *pos = strstr(str, label);  // Tìm nhãn trong chuỗi
    if (pos) {
        pos += strlen(label) + 1;    // Dịch con tr�? sau nhãn và dấu ':'
        return atoi(pos);            // Chuyển đổi giá trị thành số nguyên
    }
    return 0; // Trả v�? 0 nếu không tìm thấy nhãn
}
const char* getBitAsString(unsigned int value, int index) {
    // Kiểm tra bit tại vị trí index và trả v�? chuỗi tương ứng
    return ((value >> index) & 1) ? "1" : "0";
}
char format_string[1024] = {0};
void format_data(char *data, char *final_string) {
    char *token;
    char time_value[20] = {0};  // Chuỗi để lưu giá trị Time
    int index = 0;

    // Danh sách các nhãn cho từng mục

    const char *labels[] = {
        "time", "TS", "UD", "UVT", "UVI", "LL", "LW", "RL", "RW", "PT",
        "ST", "WAT", "LT", "AL", "WN", "OT", "24V", "IT", "FUV", "FCB",
        "H1", "H2", "SS", "PS", "UVF", "WPS", "PV", "ICB", "ICT",
        "ICE", "ICG", "ICC", "IOG"
    };
    const char *alarm_labels[] ={
    		  "ALL",
    		  "AIE",
    		  "AD",
    		  "ALS",
    		  "Not_Used_0"	,
    		  "APH",
    		  "AS",
    		  "ALP",
    		  "AA",
    		  "AUS",
    		  "AW"
    };
    const char *warn_labels[] = {
          "WE",              // Warning 1
          "Not_Used_1",        //         2 --- Not Used
          "WW",             //         3
          "WH",        //         4
          "WL",             //         5
          "WWS",             //         6
          "WUS",                //         7
          "Not_Used_2",     //         8 --- Not Used
          "WS",            //         9
          "WT"
    };

    // Sử dụng strtok để tách chuỗi
    token = strtok(data, ",");

    // Bước 1: Tạo giá trị cho "Time" bằng cách nối các mục đầu tiên
    snprintf(time_value, sizeof(time_value), "%s", token);  // 20240919
    token = strtok(NULL, ",");
    strcat(time_value, token);  // 15
    token = strtok(NULL, ",");
    strcat(time_value, token);  // 09
    token = strtok(NULL, ",");
    strcat(time_value, token);  // 15

    // Thêm giá trị "Time" vào chuỗi kết quả
    snprintf(final_string, 1024, "%s\t", time_value);  // 20240919150915

    // Bước 2: Gán các giá trị còn lại và nối vào chuỗi kết quả
    index = 1;  // Bắt đầu từ TreatSt
    while (token != NULL && index < sizeof(labels)/sizeof(labels[0])) {
        token = strtok(NULL, ",");
        if (token != NULL) {
            // Nối nhãn và giá trị vào chuỗi kết quả
            strcat(final_string, labels[index]);
            strcat(final_string,":");
            strcat(final_string, token);
            strcat(final_string, "\t");
        }
        index++;
    }
    if (strstr(final_string,labels[13])!= NULL)
    {
    	for(uint8_t i = alarm_Low_UV_Lamp;i<alarm_water_temp+1;i++)
    	{  if(i != Not_Used_0)
    		{
    		strcat(final_string,alarm_labels[i]);
    		strcat(final_string,":");
    	    strcat(final_string,getBitAsString(getValue(final_string,"ALARM"),i));
    	    strcat(final_string, "\t");
    		};
    	}
    }
    if (strstr(final_string,labels[14])!= NULL)
        {
        	for(uint8_t i = warn_End_of_Lamp_Life;i<warn_Lamp_Lifetime+1;i++)
        	{  if(i != Not_Used_1 && i!= Not_Used_2)
        		{
        		strcat(final_string,warn_labels[i]);
        		strcat(final_string,":");
        	    strcat(final_string,getBitAsString(getValue(final_string,"WARN"),i));
        	    strcat(final_string, "\t");
        		};
        	}
        }
    final_string[strlen(final_string)-1]= '\t';
}
void parse_date(const char *date_str, rtc_time *time) {
    char year_str[5] = {0};   // Chuỗi năm (4 ký tự + null-terminator)
    char month_str[3] = {0};  // Chuỗi tháng (2 ký tự + null-terminator)
    char day_str[3] = {0};    // Chuỗi ngày (2 ký tự + null-terminator)

    // Tách năm, tháng, ngày từ chuỗi gốc
    strncpy(year_str, date_str, 4);   // Lấy 4 ký tự đầu tiên (năm)
    strncpy(month_str, date_str + 4, 2); // Lấy 2 ký tự tiếp theo (tháng)
    strncpy(day_str, date_str + 6, 2);   // Lấy 2 ký tự cuối (ngày)

    // Chuyển đổi chuỗi thành số và lưu vào cấu trúc rtc_time
    time->year = (uint16_t)atoi(year_str);
    time->month = (uint8_t)atoi(month_str);
    time->date = (uint8_t)atoi(day_str);
}
char extracted_csv[20] = {0};
void process_data(char *data,const char *filename)
{

    char formatted_time[20] = {0};
    char final_data[512] = {0};  // Dữ liệu kết quả cuối cùng
    char *csv_start = strstr(filename, ".CSV");
    if (csv_start != NULL) {
        if (csv_start != NULL) {
            strncpy(extracted_csv,filename, csv_start-filename);  // Lấy phần 20240918
        }
    }

        int hours, minutes, seconds;
        char *time_start = data;
        // �?�?c định dạng th�?i gian "15:1:15"
        sscanf(time_start, "%d:%d:%d", &hours, &minutes, &seconds);

        // �?ịnh dạng lại thành "15,01,15"
        snprintf(formatted_time, sizeof(formatted_time), "%02d,%02d,%02d", hours, minutes, seconds);
        g_time.hour= hours;
        g_time.min= minutes;
        g_time.sec= seconds;
        // B�? qua phần th�?i gian trong chuỗi để lấy phần dữ liệu tiếp theo
        char *data_start = strchr(time_start, ',');
        if (data_start != NULL && *(data_start+1) != '\0' && *(data_start+1) != '\n') {
            data_start += 1;  // B�? qua dấu phẩy đầu tiên
            // Bước 3: Kết hợp chuỗi đã xử lý với phần dữ liệu còn lại
            snprintf(final_data, sizeof(final_data), "%s,%s,%s", extracted_csv, formatted_time, data_start);
            parse_date(extracted_csv,&g_time);
            memset(format_string,0,1024);
            format_data(final_data,format_string);
            // In ra kết quả cuối cùng
            mqtt_data_send((char *)format_string);
            memset(format_string,0,1024);
    }
}





FRESULT res;
char lineBuffer[256];
 uint8_t ramtoSD[1000];
 UINT br=0,bw=0;

 uint8_t *second_line;
void ReadFirstLineFromFile(const char* filename)
{
	memset(lineBuffer,0,sizeof(lineBuffer));
	memset(ramtoSD,0,sizeof(ramtoSD));
    // M? file CSV c?n d?c
    res = f_open(&USERFile, filename, FA_READ);
    if (res == FR_OK) {
			while (f_gets(lineBuffer, sizeof(lineBuffer), &USERFile) != NULL) {
				process_data(lineBuffer, filename);

    }

        f_close(&USERFile);
        res = f_open(&USERFile, filename, FA_READ);
        if (res == FR_OK) {
                f_read(&USERFile, ramtoSD, f_size(&USERFile), &br);
                f_close(&USERFile);
        }
        res = f_mount(NULL, (TCHAR const*)USERPath, 1);
        SD_FATFS_Init();
        fresult =  f_mount(&SDFatFS, (TCHAR const*)SDPath,1);
        if(fresult == FR_OK)
        {
        			fresult = f_stat(filename, &fno);
        	        second_line = &ramtoSD[0];
        	        if (fresult == FR_OK) {
        	        	 uint8_t *newline_pos = (uint8_t *)strchr((char *)ramtoSD, '\r');
        	        	 if (newline_pos != NULL) {
        	        		 *newline_pos = '\0';
        	        		  second_line = newline_pos + 2;
        	        	 }
        	        	 fresult = f_open(&SDFile, filename, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
        	        }else  fresult = f_open(&SDFile, filename, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
        	        f_lseek(&SDFile, f_size(&SDFile));
        	        fresult = FR_DISK_ERR;
        	        mqtt_debug_send((char *)second_line);
        	        fresult = f_write(&SDFile,(char *)second_line,strlen((char *)second_line),&bw);
        	        if (fresult == FR_OK)
        	        	{
        	        	mqtt_debug_send("Write data to SD successed\n");
        	        	}
        	        f_close(&SDFile);
        	        f_mount(NULL, (TCHAR const*)SDPath, 1);
        	        RAM_FATFS_Init();
        }

    } else {
    	debugPrint("Could not open file in RAM\n");
    }
    memset(lineBuffer,0,sizeof(lineBuffer));
    memset(ramtoSD,0,sizeof(ramtoSD));
    memset(buffer,0,STORAGE_BLK_SIZ*STORAGE_BLK_NBR);
    create_fat12_disk(buffer,STORAGE_BLK_SIZ,STORAGE_BLK_NBR );
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(1000);
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}



FRESULT res;
void FS_FileOperations()
{
//	send_debug("Debug FS_FileOperations\n");
  /* Register the file system object to the FatFs module */
	RAM_FATFS_Init();
//	send_debug("Debug f_mount(&USERFatFS, (TCHAR const*)USERPath, 1)\n");
	res = f_mount(&USERFatFS, (TCHAR const*)USERPath, 1);
  if(res == FR_OK)
  {
          /* Open the text file object with read access */
					res = f_opendir(&dir, (TCHAR const*)USERPath);
					if (res == FR_OK) {
//						send_debug("Debug while((fno.fattrib & AM_DIR) || fno.fname[0] != 0)\n");
						do {
											memset(fno.fname,0,sizeof(fno.fname));
											res = f_readdir(&dir, &fno);

										// Ki?m tra n?u không còn file nào
										if (res != FR_OK || fno.fname[0] == 0) {
//												debugPrint("Read end\n");
												break;
										}

										// B? qua thu m?c
										if (fno.fattrib & AM_DIR) {
//											debugPrint("Found folder\n");
										}

										// Ki?m tra n?u file có ph?n m? r?ng .csv
										if (strstr(fno.fname, ".CSV") != NULL) {
//											debugPrint("Found CSV file: ");
//											debugPrint(fno.fname);
//											debugPrint("\n");
												// �??c dòng d?u tiên t? file CSV
												ReadFirstLineFromFile(fno.fname);
										}

									}while((fno.fattrib & AM_DIR) || fno.fname[0] != 0);
						res= f_closedir(&dir);

//						res = f_mount(NULL, (TCHAR const*)USERPath, 1);
					}
  }

 //

}

void SD_FileOperations()
{

  /* Register the file system object to the FatFs module */
	SD_FATFS_Init();
	res = f_mount(&SDFatFS, (TCHAR const*)SDPath, 1);
  if(res == FR_OK)
  {
          /* Open the text file object with read access */
					res = f_opendir(&dir, (TCHAR const*)SDPath);
					if (res == FR_OK) {
						do {
											memset(fno.fname,0,sizeof(fno.fname));
											res = f_readdir(&dir, &fno);

										// Ki?m tra n?u không còn file nào
										if (res != FR_OK || fno.fname[0] == 0) {
												break;
										}

										// B? qua thu m?c
										if (fno.fattrib & AM_DIR) {
											mqtt_debug_send("Found folder\n");
										}

										// Ki?m tra n?u file có ph?n m? r?ng .csv
										if (strstr(fno.fname, ".CSV") != NULL) {
											mqtt_debug_send(fno.fname);
										}

									}while((fno.fattrib & AM_DIR) || fno.fname[0] != 0);
						res= f_closedir(&dir);
					}
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
