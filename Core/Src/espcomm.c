#include "espcomm.h"
#include "main.h"
#include "stdbool.h"
#include "stdarg.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "FLASH_PAGE_F1.h"
// Dành cho ota
#define FLASH_ADDR_FIRMWARE_UPDATE_INFO 0x0803F000
#define FLASH_ADDR_FIRMWARE_UPDATE_DOWNLOAD 0x8023000
extern uint8_t checksum_frame_to_debug;
typedef struct __attribute__((packed))			//__attribute__((packed)) noi voi complie k add padding alignment
{
	uint32_t firmwareSize;
	uint32_t isNeedUpdateFirmware;
	uint32_t crc32;
} firmware_update_info_t;
firmware_update_info_t fwUpdateInfo = {0};
char SendParameterstoMqtt[MQTT_BUFF_SIZE]; //Store MQTT messages of Parameters
char SendDebugtoMqtt[MQTT_BUFF_SIZE];
uint8_t data_ota[2048]={0};
uint8_t index_data_ota=0;
uint8_t flag_write_page_ota=0;
uint16_t page_offset=0;
uint32_t data_ota_u32[512]={0};
extern uint32_t crc32_firmwave;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern char g_rx1_char;
extern uint8_t g_debugEnable;
extern rtc_time g_time;
extern uint32_t g_DeviceType;

extern LIFO_inst g_q;
extern uint16_t g_uprate ;
extern uint32_t g_NbSector;
extern uint8_t g_forcesend;
extern uint8_t cntTimeRev1;
extern char g_rx1_buffer[MAX_BUFFER_UART1];
extern char g_ota_buffer[MAX_BUFFER_UART1];
extern uint8_t g_ota;
extern uint16_t g_rx1_cnt;
extern uint8_t g_isMqttPublished;
extern uint32_t calculate_crc32(const void *data, size_t length);
extern CRC_HandleTypeDef hcrc;
extern uint64_t count;
extern uint8_t  flag_end_frame;
void EspComm_init()
{
	HAL_UART_Receive_IT(&huart1,(uint8_t*)&g_rx1_char,1);
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
		sprintf(buff2,"@>%03d%s",strlen(buff),buff);
		HAL_UART_Transmit(&huart1,(uint8_t*)buff2,strlen(buff2),100);
	 }
}
void mqtt_data_send(const char *fmt,...)
{
		char buff[600] = {0};
		char buff2[602] = {0};
		va_list arg;
		va_start(arg, fmt);
		vsprintf(buff, fmt, arg);
		va_end(arg);
		sprintf(buff2,"%%%s",buff);
		HAL_UART_Transmit(&huart1,(uint8_t*)buff2,strlen(buff2),100);

}
void mqtt_debug_send(char* data)
{
	memset(SendDebugtoMqtt,0,sizeof(SendDebugtoMqtt));
	sprintf(SendDebugtoMqtt,"@>%03d%s",strlen(data),data);
	HAL_UART_Transmit(&huart1,(uint8_t*)SendDebugtoMqtt,strlen(SendDebugtoMqtt),100);
}
void mqtt_saved_data_send(char* data)
{
	data[5]='$';
	HAL_UART_Transmit(&huart1,(uint8_t*)data,strlen(data),100);
}
void SyncTime()
{
	if(g_time.year<2024)
	{
		char sync[10];
		memset(sync,0,sizeof(sync));
		sprintf(sync,"c:sync");
		mqtt_debug_send(sync);
	}
}
void info()
{
	g_debugEnable =1;
	debugPrint("M[%d] Rx130 FW = %d ",HAL_GetTick()/1000, FW_VER);
	debugPrint("M[%d] HW Version = %d ",HAL_GetTick()/1000, HW_VER);
	debugPrint("M[%d] Time = %04d/%02d/%02d %02d:%02d:%02d",HAL_GetTick()/1000,g_time.year,g_time.month,g_time.date,g_time.hour,g_time.min,g_time.sec);
	debugPrint("M[%d] front = %d rear = %d",HAL_GetTick()/1000,g_q.pnt_front,g_q.pnt_rear);
	debugPrint("M[%d] Upload Rate = %d ",HAL_GetTick()/1000, g_uprate);
	debugPrint("M[%d] SD sectors = %d ",HAL_GetTick()/1000, g_NbSector);
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

uint8_t CalcSumData(uint8_t *pData, uint32_t Length)
{
    uint16_t loop;
    uint8_t CheckSum;

    for (loop = 0, CheckSum = 0; loop < Length; loop++)
    {
        CheckSum += *pData;
        pData++;
    }
    CheckSum = (uint8_t)(0 - CheckSum);

    return (CheckSum);
}
void merge_uint8_to_uint32_reverse(const uint8_t *input, size_t input_size, uint32_t *output, size_t output_size) {
	memset(output,0xFF,output_size*4);
    if (input_size / 4 > output_size) {
        return;
    }
    for (size_t i = 0; i < input_size / 4; i++) {
        output[i] = (input[i * 4 + 3] << 24) |
                    (input[i * 4 + 2] << 16) |
                    (input[i * 4 + 1] << 8) |
                    input[i * 4];
    }
}
void GeneralCmd()
{
//	debugPrint("%s",g_rx1_buffer);
	for(uint16_t i=0;i< g_rx1_cnt;i++)
	{
		if(strncmp(g_rx1_buffer+i,"c:rst",5)==0)
		{
			debugPrint("M[%d] MCU Reset ",HAL_GetTick()/1000);
			HAL_NVIC_SystemReset();
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
//				DO1 &= ~(1 << 12);
				HAL_GPIO_WritePin(DO1_GPIO_Port,DO1_Pin, 0);
				debugPrint("M[%d] DO1 OFF ",HAL_GetTick()/1000);
				g_forcesend=1;
			}
			else if(strcmp(g_rx1_buffer+i+6,"1")==0)
			{
//				DO1  |= (1 << 12);
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
			uint8_t sec,min,hr,date,month;
			uint16_t yr;
			char tmp[5];
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+7,4);
			yr = atoi(tmp);
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+11,2);
			month = atoi(tmp);
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+13,2);
			date = atoi(tmp);
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+15,2);
			hr = atoi(tmp);
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+17,2);
			min = atoi(tmp);
			memset(tmp,0,sizeof(tmp));
			memcpy(tmp,g_rx1_buffer+i+19,2);
			sec = atoi(tmp);

			g_time.year = yr;
			g_time.month = month;
			g_time.date = date;
			g_time.hour = hr;
			g_time.min = min;
			g_time.sec = sec;
			debugPrint("M[%d] RTC - Saved sync time: %04d/%02d/%02d %02d:%02d:%02d",HAL_GetTick()/1000,g_time.year,g_time.month,g_time.date,g_time.hour,g_time.min,g_time.sec);
			g_forcesend=1;
		}
		else if(strncmp(g_rx1_buffer+i,"c:MqttPublished",15)==0)
		{
			g_isMqttPublished=1;
//			debugPrint("M[%d] Mqtt Published",HAL_GetTick()/1000);
		}
//		else if(strncmp(g_rx1_buffer+i,"c:flashread",11)==0)
//		{
//			ReadData(&g_q);
//		}
//		else if(strncmp(g_rx1_buffer+i,"c:flashtest",11)==0)
//		{
//			TestFlash();
//		}
//		else if(strncmp(g_rx1_buffer+i,"c:forcesend",11)==0)
//		{
//			g_forcesend =1;
////			debugPrint("M[%d] Force Send = %d",HAL_GetTick()/1000,g_forcesend);
//		}
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
//		else if(strncmp(g_rx1_buffer+i,"c:uprate:",9)==0)
//		{
//			uint32_t tmp = atoi(g_rx1_buffer+i+9);
//			if(tmp<UPRATE_MIN) g_uprate = UPRATE_DEFAUT;
//			else g_uprate = tmp;
//			SaveDeviceInfo();
//			g_forcesend=1;
//		}
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
		// Dành cho ota thiết bị hallet uv
		else if(strncmp(g_rx1_buffer+i,"c:hvbegin",9)==0)
								{
									g_debugEnable = 1;
									index_data_ota = 0;
									page_offset=0;
									crc32_firmwave=0;
									g_ota=1;
									memset(data_ota,0xFF,2048);
									HAL_UART_Transmit(&huart1,(uint8_t *)"ok",2,100);
		//
						}
		else if(strncmp(g_rx1_buffer+i,"c:hvota:",8)==0)
				{

//					g_debugEnable = 0;
				}

		else if(strncmp(g_rx1_buffer+i,"c:hvend:",8)==0)
								{
									g_debugEnable = 1;
									if(atoi(g_rx1_buffer+8)==crc32_firmwave)
										{
											if(index_data_ota<8)
												{

												//flag_write_page_ota =1
												merge_uint8_to_uint32_reverse(data_ota,2048,data_ota_u32,512);
												memset(data_ota,0xFF,2048);
												Flash_Write_Data(FLASH_ADDR_FIRMWARE_UPDATE_DOWNLOAD+page_offset*FLASH_PAGE_SIZE,data_ota_u32,512);
												page_offset++;
												index_data_ota = 0;
												}

												fwUpdateInfo.firmwareSize = page_offset * FLASH_PAGE_SIZE;
												fwUpdateInfo.isNeedUpdateFirmware = 0;
												fwUpdateInfo.crc32 = HAL_CRC_Calculate(&hcrc,(uint32_t*)&fwUpdateInfo, 2);
												Flash_Write_Data(FLASH_ADDR_FIRMWARE_UPDATE_INFO,(uint32_t*)&fwUpdateInfo,3);
												HAL_UART_Transmit(&huart1, "ok",2,100);
								//				__disable_irq();
								//				HAL_NVIC_SystemReset();
										}



								}
//		else if(strncmp(g_rx1_buffer+i,"[SD_CHECK",9)==0)
//		{
//			uint8_t st = USER_SPI_initialize (0);
//			if(!st)
//			{
//				uint32_t cnt;
//				DRESULT dr = USER_SPI_ioctl(0, GET_SECTOR_COUNT, &cnt);
//				if(!dr)
//				{
//					g_NbSector = cnt;
//					g_debugEnable = 1;
//					debugPrint("[SD_CHECK,0,]");
//					g_debugEnable = 0;
//				}
//				else
//				{
//					g_debugEnable = 1;
//					debugPrint("[SD_CHECK,1,]");
//					g_debugEnable = 0;
//				}
//			}
//			else
//			{
//				g_debugEnable = 1;
//				debugPrint("[SD_CHECK,1,]");
//				g_debugEnable = 0;
//			}
//		}
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
void EspOtaHandler()
{
	if( flag_end_frame == 1  )
		{		flag_end_frame=0;
				g_debugEnable = 1;
				if(UART1_IsDoneFrame() && flag_end_frame == 0 )
				{
					g_rx1_cnt = 0;
					memset(g_ota_buffer,0,sizeof(g_rx1_buffer));
					memcpy(g_ota_buffer,g_rx1_buffer,sizeof(g_rx1_buffer));
					flag_end_frame=1;
					memset(g_rx1_buffer,0,sizeof(g_rx1_buffer));
				}
				checksum_frame_to_debug = CalcSumData(g_rx1_buffer,264);
				if(g_ota_buffer[264]==CalcSumData(g_ota_buffer,264)){
					count++;
					memcpy(data_ota+index_data_ota*256,g_ota_buffer+8,256);
					index_data_ota++;
					crc32_firmwave = calculate_crc32(g_rx1_buffer+8, 256);
					if(index_data_ota == 8) {
						merge_uint8_to_uint32_reverse(data_ota,2048,data_ota_u32,512);
						memset(data_ota,0xFF,2048);
						Flash_Write_Data(FLASH_ADDR_FIRMWARE_UPDATE_DOWNLOAD+FLASH_PAGE_SIZE*page_offset,data_ota_u32,512);
						index_data_ota = 0;
						page_offset++;
					}
					HAL_UART_Transmit(&huart1,(uint8_t *) "ok",2,10);
				}

		}
}

