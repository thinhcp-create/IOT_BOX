/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define FW_VER 58
#define HW_VER 2 //remove flash chip, add SD card


#if HW_VER == 1
#define EXTERN_MEMORY_TYPE 1 // 1:en25Q64
#endif

#if HW_VER == 2
#define EXTERN_MEMORY_TYPE 2 // 2:SD Card
#endif




#define UPRATE_MIN 5 //sec
#define UPRATE_DEFAUT 30
#define DEVICE_HANDLER_PERIOD 20 //ms
#define KEEP_ALIVE_PERIOD 5*60*1000 //ms

#define RECV_END_TIMEOUT 30 //ms
#define ESP_COMM_PERIOD 1 //ms
#define ESP_SYNC_PERIOD (30*1000) //ms
#define BLUE_TOOTH_DURATION (5*60*1000) //ms

#define MANUAL_RESET_HOLD_TIME 3000 //ms
#define MAX_QUEUE_DATA_MODBUS 145 //= DEVICE_REGISTERS_NUMBER*2+5
#define DEVICE_REGISTERS_NUMBER 70
#define PARAMETER_QUEUE_SIZE 150
#define MQTT_BUFF_SIZE 256
#define MAX_BUFFER_UART1 265
#define MAX_BUFFER_UART5 128
#define MMC_SECTOR_SIZE 512
#define MODBUS_NB_CONFIG 21



typedef struct {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
//	uint8_t dow;
	uint8_t date;
	uint8_t month;
	uint16_t year;
} rtc_time;
typedef struct
{
	char code[5];
	char value[10];
} param_value;

typedef struct
{
	uint8_t cmd;//decimal value: modbus read command 1,2,3,4
	uint16_t start_add;
	uint16_t quantity;
} mb_conf;


#define MAX_TYPE 100000
typedef enum {
	NO_TYPE =0,
//	PUMP_CTRL_PNL =1,
	nXtPro =2,
	iSolar =3,
	PETROL_GEN =5,
	SINGLE_PMP_PRO_CP =6,
	SMART_PETROL_GEN =9,
	DUAL_PMP_PRO_CP =10
} device_type;

//static const char * const Device_Type_Name[] = {
//	[NO_TYPE] = "NO_TYPE",
////	[PUMP_CTRL_PNL] = "PUMP_CTRL_PNL",
//	[nXtPro] = "nXtPro",
//	[iSolar] = "iSolar",
//	[PETROL_GEN] = "SIMPLE_PETROL_GEN",
//	[SINGLE_PMP_PRO_CP] = "SINGLE_PMP_PRO_CP",
//	[SMART_PETROL_GEN] = "SMART_PETROL_GEN",
//	[DUAL_PMP_PRO_CP] = "DUAL_PMP_PRO_CP"
//};
typedef struct {
	int32_t pnt_front;
	int32_t pnt_rear;
//	int32_t addr_begin;
//	int32_t addr_end;
}LIFO_inst;

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DI4_Pin GPIO_PIN_0
#define DI4_GPIO_Port GPIOC
#define DI3_Pin GPIO_PIN_1
#define DI3_GPIO_Port GPIOC
#define DI2_Pin GPIO_PIN_2
#define DI2_GPIO_Port GPIOC
#define DI1_Pin GPIO_PIN_3
#define DI1_GPIO_Port GPIOC
#define USB_PWR_EN_Pin GPIO_PIN_0
#define USB_PWR_EN_GPIO_Port GPIOA
#define RS485_DE_Pin GPIO_PIN_1
#define RS485_DE_GPIO_Port GPIOA
#define RS485_TX_Pin GPIO_PIN_2
#define RS485_TX_GPIO_Port GPIOA
#define RS485_RX_Pin GPIO_PIN_3
#define RS485_RX_GPIO_Port GPIOA
#define RST_WIFI_Pin GPIO_PIN_4
#define RST_WIFI_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_0
#define LED_GPIO_Port GPIOB
#define DO1_Pin GPIO_PIN_12
#define DO1_GPIO_Port GPIOB
#define DO2_Pin GPIO_PIN_13
#define DO2_GPIO_Port GPIOB
#define DO3_Pin GPIO_PIN_14
#define DO3_GPIO_Port GPIOB
#define DO4_Pin GPIO_PIN_15
#define DO4_GPIO_Port GPIOB
#define TX_MCU_Pin GPIO_PIN_9
#define TX_MCU_GPIO_Port GPIOA
#define RX_MCU_Pin GPIO_PIN_10
#define RX_MCU_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
