/*
 * devicetype.c
 *
 *  Created on: Dec 24, 2024
 *      Author: Salmon1611
 */
#include "devicetype.h"
#include "stdio.h"
#include "string.h"
#include "espcomm.h"
#include "fatfs.h"
#include "usbd_storage_if.h"
#include "time.h"

uint32_t g_alive_tick=0;
uint16_t DeviceRegs[DEVICE_REGISTERS_NUMBER];
uint16_t g_NbMessUp = 20;
uint8_t g_regsupdate;
uint16_t g_qpos; //Current param pointer in queque
uint8_t upload_pnt; //current param upload
uint8_t g_paramupdate=0;
float g_24V;
float g_4V2;
param_value g_param_queue[PARAMETER_QUEUE_SIZE];

extern uint8_t buffer[];
extern Time hallet_time,utc_time,adjust_time;
extern TimeDifference time_diff;
extern uint8_t flag_handle_csv;
extern uint8_t g_ota;
extern uint8_t Timer_frame_ota;

void FS_FileOperations()
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
  /* Register the file system object to the FatFs module */
	RAM_FATFS_Init();
	res = f_mount(&USERFatFS, (TCHAR const*)USERPath, 1);
  if(res == FR_OK)
  {
	res = f_opendir(&dir, (TCHAR const*)USERPath);
	if (res == FR_OK)
	{
	do {
		memset(fno.fname,0,sizeof(fno.fname));
		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0)	break;
		if (strstr(fno.fname, ".CSV") != NULL)
			{
				ReadFirstLineFromFile(fno.fname);
			}

		}while((fno.fattrib & AM_DIR) || fno.fname[0] != 0);
	res= f_closedir(&dir);
	}
  }
}

char lineBuffer[256];
uint8_t ramtoSD[1000];
uint8_t *second_line;
void ReadFirstLineFromFile(const char* filename)
{
	FRESULT res;
	UINT br=0,bw=0;
	FILINFO fno;
	uint8_t line=0;
	memset(lineBuffer,0,sizeof(lineBuffer));
	memset(ramtoSD,0,sizeof(ramtoSD));
    // M? file CSV c?n d?c
    res = f_open(&USERFile, filename, FA_READ);
    if (res == FR_OK)
	{
    	while (f_gets(lineBuffer, sizeof(lineBuffer), &USERFile) != NULL)
		{
    		line++;
    		if(line==2) ParseData(lineBuffer, filename,DeviceRegs);
//    		process_data(lineBuffer, filename);
		}
		f_close(&USERFile);
		res = f_open(&USERFile, filename, FA_READ);
		if (res == FR_OK)
		{
			res = f_read(&USERFile, ramtoSD, f_size(&USERFile), &br);
			f_close(&USERFile);
		}
		if(res != FR_OK) return;
		res = f_mount(NULL, (TCHAR const*)USERPath, 1);
		SD_FATFS_Init();
		res =  f_mount(&SDFatFS, (TCHAR const*)SDPath,1);
		if(res == FR_OK)
		{
			res = f_stat(filename, &fno);
			second_line = &ramtoSD[0];
			if (res == FR_OK)
			{
				uint8_t *newline_pos = (uint8_t *)strchr((char *)ramtoSD, '\r');
				if (newline_pos != NULL)
				{
					*newline_pos = '\0';
					second_line = newline_pos + 2;
				}
				res = f_open(&SDFile, filename, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
			}else  res = f_open(&SDFile, filename, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
			f_lseek(&SDFile, f_size(&SDFile));
			res = FR_DISK_ERR;
			mqtt_debug_send((char *)second_line);
			res = f_write(&SDFile,(char *)second_line,strlen((char *)second_line),&bw);
			if (res == FR_OK)
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
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,0);
    HAL_Delay(500);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,1);
    flag_handle_csv=0;
}

void ParseData(const char* input, char* filename,uint16_t * Value) {
	if (sscanf(filename, "%04d%02d%02d", &hallet_time.year, &hallet_time.month, &hallet_time.day)!=3)
		return;
    char buffer[256];  // Tạo bản sao của chuỗi đầu vào
    strncpy(buffer, input, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0'; // Đảm bảo kết thúc chuỗi

    if (sscanf(buffer, "%d:%d:%d", &hallet_time.hour, &hallet_time.minute, &hallet_time.second)!=3)
    		return;
    // Bỏ qua phần "11:48:43" bằng cách tìm dấu phẩy đầu tiên
    char* dataStart = strchr(buffer, ',');
    if (dataStart == NULL)  return;
    dataStart++; // Di chuyển qua dấu phẩy để bắt đầu từ số đầu tiên sau thời gian
    // Tách các số theo dấu phẩy
    char* token = strtok(dataStart, ",");
    int index = 0;

    while (token != NULL) {
        Value[index] = atoi(token); // Chuyển token thành số nguyên
        index++;
        token = strtok(NULL, ","); // Lấy token tiếp theo
    }
}

void Device_Handler()
{
	Hallet_Program();
//	ParamQueueToMQTT();
}

Hallet_Program()
{
	if((HAL_GetTick()-g_alive_tick) > KEEP_ALIVE_PERIOD)
		{
			g_alive_tick = HAL_GetTick();
//			Hallet_RegsToParam(0);
		}
	if(flag_handle_csv ==1 &&(g_ota==0||Timer_frame_ota == 0))
		{
			memset(DeviceRegs,0,sizeof(DeviceRegs));
			FS_FileOperations();
//			Hallet_RegsToParam(1);

		}
}
