#include "espcomm.h"
#include "main.h"
#include "stdbool.h"
#include "stdarg.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "FLASH_PAGE_F1.h"
#include "fatfs.h"
#include "time.h"

char SendDebugtoMqtt[MQTT_BUFF_SIZE];
char SendParameterstoMqtt[MQTT_BUFF_SIZE]; //Store MQTT messages of Parameters

volatile uint8_t g_ota=0;
uint32_t crc32_firmwave=0;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern char g_rx1_char;
extern uint8_t g_debugEnable;
extern uint32_t g_NbSector;
extern uint8_t g_forcesend;
extern uint8_t cntTimeRev1;
extern char g_rx1_buffer[MAX_BUFFER_UART1];
extern uint16_t g_rx1_cnt;
extern uint8_t g_isMqttPublished;
extern uint32_t calculate_crc32(const void *data, size_t length);
extern CRC_HandleTypeDef hcrc;
extern SD_HandleTypeDef hsd;
extern uint8_t flag_readSD;
extern LIFO_inst g_q;
extern RTC_HandleTypeDef hrtc;
extern RTC_TimeTypeDef sTime;
extern RTC_DateTypeDef sDate;

// Dành cho adjust time hallet to utc
Time utc_time={

                .year=2014,
                .month=11,
                .day = 25,
                .hour = 16,
                .minute = 47,
                .second = 30

};
uint8_t flag_sync_time = 0;
extern Time adjust_time;
// Dành cho ota
#define FLASH_ADDR_FIRMWARE_UPDATE_INFO 0x0803F000
#define FLASH_ADDR_FIRMWARE_UPDATE_DOWNLOAD 0x8023000
typedef struct __attribute__((packed))			//__attribute__((packed)) noi voi complie k add padding alignment
{
	uint32_t firmwareSize;
	uint32_t isNeedUpdateFirmware;
	uint32_t crc32;
} firmware_update_info_t;
firmware_update_info_t fwUpdateInfo = {0};
extern uint8_t Timer_frame_ota;
#define OTA_BUFFER_SIZE FLASH_PAGE_SIZE
#define CHUNK_SIZE      256
static uint8_t data_ota[OTA_BUFFER_SIZE]={0};
static uint8_t process_count = 0;
static uint16_t ota_index = 0;

static uint8_t CalcSumData(uint8_t *pData, uint32_t Length)
{
    static uint16_t loop;
    static uint8_t CheckSum;

    for (loop = 0, CheckSum = 0; loop < Length; loop++)
    {
        CheckSum += *pData;
        pData++;
    }
    CheckSum = (uint8_t)(0 - CheckSum);

    return (CheckSum);
}


// Hàm xử lý dữ liệu khi đủ 2048 byte ota
static void Process_OTA_Data(uint8_t *data, uint16_t len,uint16_t count_process)
{
	Flash_Write_Array(FLASH_ADDR_FIRMWARE_UPDATE_DOWNLOAD+count_process*2048,data,len);
}

// Hàm nhận và xử lý dữ liệu OTA
void Handle_OTA_Chunk(uint8_t *chunk, uint16_t len)
{
    // Kiểm tra dữ liệu hợp lệ
    if (len != CHUNK_SIZE)
    {
        printf("Error: Invalid chunk size\n");
        return;
    }

    // Thêm chunk vào mảng đệm
    crc32_firmwave = calculate_crc32(chunk, len);
    memcpy(&data_ota[ota_index], chunk, len);
    ota_index += len;

    // Kiểm tra nếu mảng đệm đủ 2048 byte
    if (ota_index >= OTA_BUFFER_SIZE)
    {

        // Xử lý dữ liệu
        Process_OTA_Data(data_ota, OTA_BUFFER_SIZE,process_count);
        process_count++;
        // Reset mảng đệm
        memset(data_ota, 0, OTA_BUFFER_SIZE);
        ota_index = 0;
    }
}

void EspComm_init()
{
	HAL_UART_Receive_IT(&huart1,(uint8_t*)&g_rx1_char,1);
	SyncTime();
}

void debugPrint(const char *fmt, ...)
{
	 if(g_debugEnable)
	 {
		char buff[300] = {0};
		char buff2[320] = {0};
		va_list arg;
		va_start(arg, fmt);
		vsprintf(buff, fmt, arg);
		va_end(arg);
		sprintf(buff2,"@>%03d%s ",strlen(buff),buff);
		HAL_UART_Transmit(&huart1,(uint8_t*)buff2,strlen(buff2),1000);
	 }
}
void mqtt_data_send(char* data)
{
//	char info[256]={0};
//	sprintf(info,"%s:%ld\t%s:%d\t%s:%d\t%s:%d\t","MCUU",HAL_GetTick()/1000,"TYPE",TYPE,"MFW",FW_VER,"HW",HW_VER);
//	strcat(data,info);
//	memset(SendParameterstoMqtt,0,sizeof(SendParameterstoMqtt));
//	sprintf(SendParameterstoMqtt,"@>%03d%%%s\t",strlen(data)+1,data);
//	HAL_UART_Transmit(&huart1,(uint8_t*)SendParameterstoMqtt,strlen(SendParameterstoMqtt),100);

}
void mqtt_debug_send(char* data)
{
	memset(SendDebugtoMqtt,0,sizeof(SendDebugtoMqtt));
	sprintf(SendDebugtoMqtt,"@>%03d%s ",strlen(data),data);
	HAL_UART_Transmit(&huart1,(uint8_t*)SendDebugtoMqtt,strlen(SendDebugtoMqtt),1000);
}
void mqtt_saved_data_send(char* data)
{
	data[5]='$';
	HAL_UART_Transmit(&huart1,(uint8_t*)data,strlen(data),1000);
}
void SyncTime()
{
		char sync[10];
		memset(sync,0,sizeof(sync));
		sprintf(sync,"c:sync");
		mqtt_debug_send(sync);
}
void info()
{
	g_debugEnable =1;
	debugPrint("M[%d] STM32 FW = %d ",HAL_GetTick()/1000, FW_VER);
	debugPrint("M[%d] HW Version = %d ",HAL_GetTick()/1000, HW_VER);
	debugPrint("M[%d] Hallet utc time = %04d/%02d/%02d %02d:%02d:%02d",HAL_GetTick()/1000,adjust_time.year,adjust_time.month,adjust_time.day,adjust_time.hour,adjust_time.minute,adjust_time.second);
	debugPrint("M[%d] front = %d rear = %d",HAL_GetTick()/1000,g_q.pnt_front,g_q.pnt_rear);
//	debugPrint("M[%d] Upload Rate = %d ",HAL_GetTick()/1000, g_uprate);
	debugPrint("M[%d] SD sectors = %d ",HAL_GetTick()/1000, hsd.SdCard.BlockNbr);
//	TestFlash();
	g_debugEnable =0;
}

bool UART1_IsDoneFrame(void)
{
	if(cntTimeRev1>0)
	{
		cntTimeRev1--;
		if(cntTimeRev1 == 0)
		{
			return true;
		}
	}
	return false;
}

void FullSystemReset(void)
{
    // 1. Tắt tất cả GPIO (nếu không cần thiết sẽ giữ lại trạng thái)
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_All);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_All);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_All);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_All);
    // Thêm các cổng GPIO khác nếu cần (GPIOC, GPIOD...)

    // 2. Tắt các ngoại vi đã bật
    HAL_DeInit(); // Hủy khởi tạo toàn bộ ngoại vi theo HAL

    // 3. Reset thủ công tất cả các ngoại vi chính
    __HAL_RCC_GPIOA_FORCE_RESET();
    __HAL_RCC_GPIOA_RELEASE_RESET();
    __HAL_RCC_GPIOB_FORCE_RESET();
    __HAL_RCC_GPIOB_RELEASE_RESET();
    // Thêm các reset ngoại vi khác nếu cần

    // 4. Xóa cấu hình của Vector Table
    __set_MSP(*(__IO uint32_t*)0x08000000); // Đặt lại Stack Pointer
    SCB->VTOR = 0x08000000;                // Đặt Vector Table về Flash


    // 5. Reset hệ thống bằng lệnh NVIC
    __disable_irq();          // Tắt ngắt
    HAL_NVIC_SystemReset();   // Reset toàn bộ vi điều khiển
}


void GeneralCmd()
{
//	debugPrint("%s",g_rx1_buffer);
	for(uint16_t i=0;i< g_rx1_cnt;i++)
	{
	if(g_ota==0 || Timer_frame_ota == 0){
		if(strncmp(g_rx1_buffer+i,"c:rst",5)==0)
		{
			debugPrint("M[%d] MCU Reset ",HAL_GetTick()/1000);
			FullSystemReset();
		}
		if(strncmp(g_rx1_buffer+i,"c:info",6)==0)
		{
			g_debugEnable = 1;
			info();
		}
		else if(strncmp(g_rx1_buffer+i,"c:do1:",6)==0)
		{
			if(strcmp(g_rx1_buffer+i+6,"0")==0)
			{
				HAL_GPIO_WritePin(DO1_GPIO_Port,DO1_Pin, 0);
				debugPrint("M[%d] DO1 OFF ",HAL_GetTick()/1000);
				g_forcesend=1;
			}
			else if(strcmp(g_rx1_buffer+i+6,"1")==0)
			{
				HAL_GPIO_WritePin(DO1_GPIO_Port,DO1_Pin, 1);
				debugPrint("M[%d] DO1 ON ",HAL_GetTick()/1000);
				g_forcesend=1;
			}
			else
			{
				debugPrint("M[%d] DO1 - Wrong command ",HAL_GetTick()/1000);
			}
		}
		else if(strncmp(g_rx1_buffer+i,"c:do2:",6)==0)
		{
			if(strcmp(g_rx1_buffer+i+6,"0")==0)
			{
				HAL_GPIO_WritePin(DO2_GPIO_Port,DO2_Pin, 0);
				debugPrint("M[%d] DO2 OFF ",HAL_GetTick()/1000);
				g_forcesend=1;
			}
			else if(strcmp(g_rx1_buffer+i+6,"1")==0)
			{
				HAL_GPIO_WritePin(DO2_GPIO_Port,DO2_Pin, 1);
				debugPrint("M[%d] DO2 ON ",HAL_GetTick()/1000);
				g_forcesend=1;
			}
			else
			{
				debugPrint("M[%d] DO2 - Wrong command ",HAL_GetTick()/1000);
			}
		}
		else if(strncmp(g_rx1_buffer+i,"c:do3:",6)==0)
		{
			if(strcmp(g_rx1_buffer+i+6,"0")==0)
			{
				HAL_GPIO_WritePin(DO3_GPIO_Port,DO3_Pin, 0);
				debugPrint("M[%d] DO3 OFF ",HAL_GetTick()/1000);
				g_forcesend=1;
			}
			else if(strcmp(g_rx1_buffer+i+6,"1")==0)
			{
				HAL_GPIO_WritePin(DO3_GPIO_Port,DO3_Pin, 1);
				debugPrint("M[%d] DO3 ON ",HAL_GetTick()/1000);
				g_forcesend=1;
			}
		}
		else if(strncmp(g_rx1_buffer+i,"c:do4:",6)==0)
		{
			if(strcmp(g_rx1_buffer+i+6,"0")==0)
			{
				HAL_GPIO_WritePin(DO4_GPIO_Port,DO4_Pin, 0);
				debugPrint("M[%d] DO4 OFF ",HAL_GetTick()/1000);
				g_forcesend=1;
			}
			else if(strcmp(g_rx1_buffer+i+6,"1")==0)
			{
				HAL_GPIO_WritePin(DO4_GPIO_Port,DO4_Pin, 1);
				debugPrint("M[%d] DO4 ON ",HAL_GetTick()/1000);
				g_forcesend=1;
			}
		}
		else if(strncmp(g_rx1_buffer+i,"c:time:",7)==0)
		{
//			uint8_t sec,min,hr,date,month;
//			uint16_t yr;
			char tmp[5];
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+7,4);
			sDate.Year = atoi(tmp)-2000;
		    memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+11,2);
			sDate.Month = atoi(tmp);
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+13,2);
			sDate.Date = atoi(tmp);
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+15,2);
			sTime.Hours = atoi(tmp);
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+17,2);
			sTime.Minutes = atoi(tmp);
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+19,2);
			sTime.Seconds = atoi(tmp);
			HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
			HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
//			utc_time.year = yr;
//			utc_time.month = month;
//			utc_time.day = date;
//			utc_time.hour = hr;
//			utc_time.minute = min;
//			utc_time.second = sec;
			g_debugEnable=1;
			debugPrint("M[%d] RTC - Saved sync time: %04d/%02d/%02d %02d:%02d:%02d",HAL_GetTick()/1000,utc_time.year,utc_time.month,utc_time.day,utc_time.hour,utc_time.minute,utc_time.second);
			g_debugEnable=0;
			g_forcesend=1;
			flag_sync_time=1;
		}
		else if(strncmp(g_rx1_buffer+i,"c:MqttPublished",15)==0)
		{
			g_isMqttPublished=1;
			debugPrint("M[%d] Mqtt Published",HAL_GetTick()/1000);
		}
//		else if(strncmp(g_rx1_buffer+i,"c:flashread",11)==0)
//		{
//			ReadData(&g_q);
//		}
//		else if(strncmp(g_rx1_buffer+i,"c:flashtest",11)==0)
//		{
//			TestFlash();
//		}
		else if(strncmp(g_rx1_buffer+i,"c:forcesend",11)==0)
		{
			g_forcesend =1;
			debugPrint("M[%d] Force Send = %d",HAL_GetTick()/1000,g_forcesend);
		}
//		else if(strncmp(g_rx1_buffer+i,"c:appinit:",10)==0)
//		{
//			g_BLT_state =0;
//			g_DeviceType = atoi(g_rx1_buffer+i+10);
//			SaveDeviceInfo();
//		}
		else if(strncmp(g_rx1_buffer+i,"c:dben:",7)==0)
		{
			g_debugEnable = atoi(g_rx1_buffer+i+7);
			debugPrint("M[%d] Debug Enable",HAL_GetTick()/1000);
		}
		else if(strncmp(g_rx1_buffer+i,"c:sd",4)==0)
		{

			if(BSP_SD_Init()==MSD_OK)
			{
				mqtt_debug_send("Wait for read sd card");
				flag_readSD=1;
			}
			else
			{
				mqtt_debug_send("SD card not available\n");
			}

		}
		else if(strncmp(g_rx1_buffer+i,"c:usbrst",8)==0)
				{
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,0);
			    HAL_Delay(500);
			    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,1);
			    mqtt_debug_send("USB refresh\n");
				}
		else if(strncmp(g_rx1_buffer+i,"[IN_CHECK,1",11)==0)
		{
			g_debugEnable = 1;
			debugPrint("[IN_CHECK,1,%d]",HAL_GPIO_ReadPin(DI1_GPIO_Port, DI1_Pin));
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[IN_CHECK,2",11)==0)
		{
			g_debugEnable = 1;
			debugPrint("[IN_CHECK,2,%d]",HAL_GPIO_ReadPin(DI2_GPIO_Port, DI2_Pin));
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[IN_CHECK,3",11)==0)
		{
			g_debugEnable = 1;
			debugPrint("[IN_CHECK,3,%d]",HAL_GPIO_ReadPin(DI3_GPIO_Port, DI3_Pin));
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[IN_CHECK,4",11)==0)
		{
			g_debugEnable = 1;
			debugPrint("[IN_CHECK,4,%d]",HAL_GPIO_ReadPin(DI4_GPIO_Port, DI4_Pin));
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[OUT_CHECK,1,1",14)==0)
		{
			HAL_GPIO_WritePin(DO1_GPIO_Port,DO1_Pin, 1);
			g_debugEnable = 1;
			debugPrint("[OUT_CHECK,1,]");
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[OUT_CHECK,1,0",14)==0)
		{
			HAL_GPIO_WritePin(DO1_GPIO_Port,DO1_Pin, 0);
			g_debugEnable = 1;
			debugPrint("[OUT_CHECK,1,]");
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[OUT_CHECK,2,1",14)==0)
		{
			HAL_GPIO_WritePin(DO2_GPIO_Port,DO2_Pin, 1);
			g_debugEnable = 1;
			debugPrint("[OUT_CHECK,2,]");
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[OUT_CHECK,2,0",14)==0)
		{
			HAL_GPIO_WritePin(DO2_GPIO_Port,DO2_Pin, 0);
			g_debugEnable = 1;
			debugPrint("[OUT_CHECK,2,]");
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[OUT_CHECK,3,1",14)==0)
		{
			HAL_GPIO_WritePin(DO3_GPIO_Port,DO3_Pin, 1);
			g_debugEnable = 1;
			debugPrint("[OUT_CHECK,3,]");
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[OUT_CHECK,3,0",14)==0)
		{
			HAL_GPIO_WritePin(DO3_GPIO_Port,DO3_Pin, 0);
			g_debugEnable = 1;
			debugPrint("[OUT_CHECK,3,]");
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[OUT_CHECK,4,1",14)==0)
		{
			HAL_GPIO_WritePin(DO4_GPIO_Port,DO4_Pin, 1);
			g_debugEnable = 1;
			debugPrint("[OUT_CHECK,4,]");
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[OUT_CHECK,4,0",14)==0)
		{
			HAL_GPIO_WritePin(DO4_GPIO_Port,DO4_Pin, 0);
			g_debugEnable = 1;
			debugPrint("[OUT_CHECK,4,]");
			g_debugEnable = 0;
		}
		else if(strncmp(g_rx1_buffer+i,"[POWER_4G,1",11)==0)
		{
			g_debugEnable = 1;
			debugPrint("[POWER_4G,1,]");
			g_debugEnable = 0;
			HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port,USB_PWR_EN_Pin, 1);
			HAL_Delay(1000);
		}
		else if(strncmp(g_rx1_buffer+i,"[POWER_4G,0",11)==0)
		{
			g_debugEnable = 1;
			debugPrint("[POWER_4G,0,]");
			g_debugEnable = 0;
			HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port,USB_PWR_EN_Pin, 0);
			HAL_Delay(1000);
		}
		else if(strncmp(g_rx1_buffer+i,"[RS485_CHECK",12)==0)
		{
			char tmp[20];
			sprintf(tmp,"[RS485_CHECK,0]");
			HAL_GPIO_WritePin(RS485_DE_GPIO_Port,RS485_DE_Pin,1);
			HAL_UART_Transmit(&huart2,(uint8_t *)tmp, strlen(tmp), 100);
			HAL_GPIO_WritePin(RS485_DE_GPIO_Port,RS485_DE_Pin,0);
		}
		else if(strncmp(g_rx1_buffer+i,"[SD_CHECK",9)==0)
			{
			if(BSP_SD_Init()==MSD_OK)
			  {
				g_debugEnable = 1;
				debugPrint("[SD_CHECK,1,]");
				g_debugEnable = 0;
			  }
			else
			  {
				g_debugEnable = 1;
				debugPrint("[SD_CHECK,0,]");
				g_debugEnable = 0;
			  }
			}
		// Dành cho ota thiết bị hallet uv
		else if(strncmp(g_rx1_buffer+i,"c:hvbegin:",10)==0)
		{
			g_debugEnable = 1;
			fwUpdateInfo.firmwareSize = atoi(g_rx1_buffer+i+10);
			fwUpdateInfo.isNeedUpdateFirmware=0;
			fwUpdateInfo.crc32=0;
			g_ota=1;
			Timer_frame_ota =10;
			crc32_firmwave=0;
			memset(data_ota,0xFF,2048);
			HAL_UART_Transmit(&huart1,(uint8_t *)"ok",2,100);
		}
		else if(strncmp(g_rx1_buffer+i,"c:hvend:",8)==0)
		{
			g_ota=0;
			if(ota_index != 0)
			{
				Process_OTA_Data(data_ota, ota_index,process_count);
//				fwUpdateInfo.firmwareSize = fwUpdateInfo.firmwareSize+ota_index;
				process_count=0;
				ota_index=0;
			}

			memset(data_ota, 0, OTA_BUFFER_SIZE);
			if(atoi(g_rx1_buffer+8)==crc32_firmwave)
			{
				fwUpdateInfo.isNeedUpdateFirmware = 1;
				fwUpdateInfo.crc32 = HAL_CRC_Calculate(&hcrc,(uint32_t*)&fwUpdateInfo, 2);
				Flash_Write_Data(FLASH_ADDR_FIRMWARE_UPDATE_INFO,(uint32_t*)&fwUpdateInfo,3);
				HAL_UART_Transmit(&huart1, "ok",2,100);
				FullSystemReset();
			}
		}
	}
	else 	if(strncmp(g_rx1_buffer+i,"c:hvota:",8)==0)
			{
				g_debugEnable = 1;
				Timer_frame_ota =10;
				if(g_rx1_cnt>260)
				{
					if(g_rx1_buffer[264]==CalcSumData(g_rx1_buffer,264))
					{
						Timer_frame_ota = 10;
						Handle_OTA_Chunk(g_rx1_buffer+8,256);
						memset(g_rx1_buffer,0xFF, 265);
						HAL_UART_Transmit(&huart1,(uint8_t *) "ok",2,10);
					}
				}
			}
//
//		else if(strncmp(g_rx1_buffer+i,"[WD_CHECK",9)==0)
//		{
//			WDI_MCU = 0;
//			R_Config_CMT0_Stop();
//			R_BSP_SoftwareDelay(3000,BSP_DELAY_MILLISECS);
//			g_debugEnable = 1;
//			debugPrint("[WD_CHECK,1,]");
//			g_debugEnable = 0;
//			R_Config_CMT0_Start();
//		}
	}
}

void EspCmdHandler()
{
	if(UART1_IsDoneFrame()&&g_rx1_cnt>=5)
	{
//		debugPrint("M[%d] Intra-Comm Received [%d][%d]:",HAL_GetTick()/1000,g_rx1_cnt,strlen(g_rx1_buffer));
//		mqtt_debug_send(g_rx1_buffer);
		GeneralCmd();
//		Device_Cmd_Handler(g_rx1_buffer);
		g_rx1_cnt=0;
		memset(g_rx1_buffer,0,sizeof(g_rx1_buffer));
	}
}
