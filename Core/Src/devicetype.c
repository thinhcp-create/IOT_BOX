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
int16_t DeviceRegs[DEVICE_REGISTERS_NUMBER];
uint16_t g_NbMessUp = 5;
uint16_t g_qpos; //Current param pointer in queque
uint8_t upload_pnt; //current param upload
uint8_t g_paramupdate=0;
uint32_t g_24V_mV;
uint32_t g_4V2_mV;
uint16_t g_DeviceType = 10030;
param_value g_param_queue[PARAMETER_QUEUE_SIZE];
uint8_t flag_handle_csv_done=0;
const uint8_t g_uprate = 60;
DWORD fre_clust = 0,fre_sect=0;

extern uint8_t buffer[];
extern Time hallet_time;
extern Time g_time;
extern uint8_t flag_sync_time;
extern uint8_t flag_handle_csv;
extern uint8_t g_forcesend;
extern uint8_t g_isMqttPublished;
extern UART_HandleTypeDef huart1;
extern char SendParameterstoMqtt[MQTT_BUFF_SIZE];
extern LIFO_inst g_q;
extern uint8_t usbStatus;
extern uint8_t usb_retry;
extern uint32_t SD_DATA_SECTOR_BEGIN ;
extern uint32_t SD_DATA_SECTOR_END ;
uint8_t param_quantity=32;

void FS_FileOperations()
{
	mqtt_debug_send("FS_FileOperations\n");
	flag_handle_csv=0;
	flag_handle_csv_done=0;
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
  mqtt_debug_send("End FS_FileOperations\n");
}


//uint8_t ramtoSD[1000];
uint8_t ramtoSD[256];
char lineBuffer[256];
uint8_t *second_line;
void ReadFirstLineFromFile(const char* filename)
{
	mqtt_debug_send("ReadFirstLineFromFile\n");
	FRESULT res;
	UINT br=0,bw=0;
	FILINFO fno;
	uint8_t line=0;
	DIR dir;

	memset(lineBuffer,0,sizeof(lineBuffer));
	memset(ramtoSD,0,256);

    // M? file CSV c?n d?c
    res = f_open(&USERFile, filename, FA_READ);
    if (res == FR_OK)
	{

		if (sscanf(filename, "%04d%02d%02d.CSV", &hallet_time.year, &hallet_time.month, &hallet_time.day)!=3)
		{
			return;
		}

		if (flag_sync_time==1 && g_isMqttPublished==1)
		{
		       flag_sync_time = 2;
		}
		while (f_gets(lineBuffer, sizeof(lineBuffer), &USERFile) != NULL)
							{
								line++;
								if(line>=2)
								{
									memset(DeviceRegs,0,sizeof(DeviceRegs));
									ParseData(lineBuffer,DeviceRegs);
									if (flag_handle_csv_done)
									{
										if(line == 2)
										{
											Hallet_RegsToParam(flag_handle_csv_done);
											//	if(line==2)
											ParamQueueToMQTT(decreaseTimeSeconds(g_time, 30));
										}
										else
										{
											Hallet_RegsToParam(flag_handle_csv_done);
											ParamQueueToMQTT(g_time);
										}
//
									}

								}
							}
//		g_isMqttPublished=0;
		f_close(&USERFile);
		res = f_open(&USERFile, filename, FA_READ);
		if (res == FR_OK)
		{
			res = f_read(&USERFile, ramtoSD, f_size(&USERFile), &br);
			f_close(&USERFile);
			f_unlink(filename);
			res = f_mount(NULL, (TCHAR const*)USERPath, 1);
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,1);
			HAL_GPIO_WritePin(PW_USB_GPIO_Port,PW_USB_Pin,0);
			// usbStatus=0;
			SD_FATFS_Init();
			res =  f_mount(&SDFatFS, (TCHAR const*)SDPath,1);
			if(res == FR_OK)
			{
				res = f_getfree((TCHAR const*)SDPath, &fre_clust, &SDFatFS);
				if (res == FR_OK)
				{
					// Tính toán thông tin dung lượng
//				    tot_sect = (SDFatFS.n_fatent - 2) * SDFatFS.csize;   // Tổng số sector
				    fre_sect = fre_clust * SDFatFS.csize;          // Số sector còn trống
				    // khi fre <= 200*100 thì xóa file cũ nhất
				    if(fre_sect <= SD_DATA_SECTOR_END-SD_DATA_SECTOR_BEGIN+100)
				    {
				    	res = f_opendir(&dir, (TCHAR const*)SDPath);
				    	if (res == FR_OK)
				    	{
				    		char oldest_file[64] = {0};
				    		DWORD oldest_time = 0xFFFFFFFF; // Giá trị lớn nhất để so sánh // Giá trị lớn nhất để so sánh
				    		do
				    		{
				    			memset(fno.fname,0,sizeof(fno.fname));
				    			res = f_readdir(&dir, &fno);
				    			if (fno.fattrib & AM_DIR) {
				    			continue;
				  				}
				    			if (res != FR_OK || fno.fname[0] == 0)	break;
				    			if (fno.fdate < oldest_time)
				    			{
				    				oldest_time = fno.fdate;
				    			    strcpy(oldest_file, fno.fname);
				    			}

				    		}	while((fno.fattrib & AM_DIR) || fno.fname[0] != 0);
				    		res= f_closedir(&dir);
				    	     // Xóa file cũ nhất
				    	    if (strlen(oldest_file) > 0)
				    	    {
//				    	    	mqtt_debug_send(oldest_file);
//				    	      printf("Deleting oldest file: %s\n", oldest_file);
				    	      res = f_unlink(oldest_file);
				    	    }
				    	}
				    }
				}
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
				} else  res = f_open(&SDFile, filename, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
				f_lseek(&SDFile, f_size(&SDFile));
				res = FR_DISK_ERR;
//				mqtt_debug_send((char *)second_line);
				res = f_write(&SDFile,(char *)second_line,strlen((char *)second_line),&bw);
//				if (res == FR_OK)
//				{
////					mqtt_debug_send("Write data to SD successed\n");
//					mqtt_debug_send(second_line);
//				}
				f_sync(&SDFile);
				f_close(&SDFile);
				f_mount(NULL, (TCHAR const*)SDPath, 1);

			}
		}


    } else {
    	debugPrint("Could not open file in RAM\n");
    }
    RAM_FATFS_Init();

    memset(lineBuffer,0,sizeof(lineBuffer));
    memset(ramtoSD,0,256);

    memset(buffer,0,STORAGE_BLK_SIZ*STORAGE_BLK_NBR);
    create_fat12_disk(buffer,STORAGE_BLK_SIZ,STORAGE_BLK_NBR );

    HAL_Delay(500);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,0);
    HAL_GPIO_WritePin(PW_USB_GPIO_Port,PW_USB_Pin,1);
    mqtt_debug_send("End ReadFirstLineFromFile\n");
}
void ParseData(const char* input,int16_t * Value) {
	mqtt_debug_send("ParseData\n");
	char buffer[256];  // Tạo bản sao của chuỗi đầu vào
	strncpy(buffer, input, sizeof(buffer));
	if (sscanf(buffer, "%d:%d:%d,", &hallet_time.hour, &hallet_time.minute, &hallet_time.second)!=3)
		return;
    // Bỏ qua phần "11:48:43" bằng cách tìm dấu phẩy đầu tiên
    char* dataStart = strchr(buffer, ',');
    if (dataStart == NULL)  return;
    dataStart++; // Di chuyển qua dấu phẩy để bắt đầu từ số đầu tiên sau thời gian
    // Tách các số theo dấu phẩy
    char* token = strtok(dataStart, ",");
    uint8_t index = 0;

    while (token != NULL) {
        Value[index] = atoi(token); // Chuyển token thành số nguyên
        index++;
        token = strtok(NULL, ","); // Lấy token tiếp theo
    }
    param_quantity = index-1;
    flag_handle_csv_done = 1;
}

void Device_Handler()
{
	Hallet_Program();
	ParamQueueToMQTT(g_time);
}

Hallet_Program()
{
	if( flag_sync_time == 0 || (g_isMqttPublished == 0 && flag_sync_time != 2))
	{
		if((HAL_GetTick()-g_alive_tick) > ESP_FORCESEND_PERIOD || g_forcesend == 1)
		{
			g_alive_tick = HAL_GetTick();
			VoltMeasure();
			Hallet_RegsToParam(0);
			g_forcesend = 1;
		}
	}
	else
	{
		if((HAL_GetTick()-g_alive_tick) > KEEP_ALIVE_PERIOD || g_forcesend == 1)
		{
			g_alive_tick = HAL_GetTick();
			VoltMeasure();
			Hallet_RegsToParam(0);
			g_forcesend = 1;
		}
	}
	if(flag_handle_csv ==1 && g_forcesend == 0 )
	{
		// usbStatus=0;
		memset(DeviceRegs,0,sizeof(DeviceRegs));
		VoltMeasure();
		FS_FileOperations();
	}
}

void Hallet_RegsToParam(uint8_t sts)
{
	mqtt_debug_send("Hallet_RegsToParam\n");
	g_qpos=0;
	upload_pnt=0;
	flag_handle_csv_done = 0;
	if(sts)
	{
		for(uint8_t i=0; i<param_quantity; i++)
		{
			sprintf(g_param_queue[i].code,"%d",i+1);
			sprintf(g_param_queue[i].value,"%d",DeviceRegs[i]);
			g_qpos++;
		}

	}
	sprintf(g_param_queue[g_qpos].code,"MBCF");
	sprintf(g_param_queue[g_qpos].value,"%01d",1);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"VSRC");
	sprintf(g_param_queue[g_qpos].value,"%d",g_24V_mV);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"VBAT");
	sprintf(g_param_queue[g_qpos].value,"%d",g_4V2_mV);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"DI1");
	sprintf(g_param_queue[g_qpos].value,"%01d",HAL_GPIO_ReadPin(DI1_GPIO_Port, DI1_Pin));
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"DI2");
	sprintf(g_param_queue[g_qpos].value,"%01d",HAL_GPIO_ReadPin(DI2_GPIO_Port, DI2_Pin));
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"DI3");
	sprintf(g_param_queue[g_qpos].value,"%01d",HAL_GPIO_ReadPin(DI3_GPIO_Port, DI3_Pin));
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"DI4");
	sprintf(g_param_queue[g_qpos].value,"%01d",HAL_GPIO_ReadPin(DI4_GPIO_Port, DI4_Pin));
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"DO1");
	sprintf(g_param_queue[g_qpos].value,"%01d",HAL_GPIO_ReadPin(DO1_GPIO_Port, DO1_Pin));
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"DO2");
	sprintf(g_param_queue[g_qpos].value,"%01d",HAL_GPIO_ReadPin(DO2_GPIO_Port, DO2_Pin));
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"DO3");
	sprintf(g_param_queue[g_qpos].value,"%01d",HAL_GPIO_ReadPin(DO3_GPIO_Port, DO3_Pin));
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"DO4");
	sprintf(g_param_queue[g_qpos].value,"%01d",HAL_GPIO_ReadPin(DO4_GPIO_Port, DO4_Pin));
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"MCUU");
	sprintf(g_param_queue[g_qpos].value,"%d",(HAL_GetTick()/1000));
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"TYPE");
	sprintf(g_param_queue[g_qpos].value,"%d",g_DeviceType);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"MFW");
	sprintf(g_param_queue[g_qpos].value,"%d",FW_VER);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"HW");
	sprintf(g_param_queue[g_qpos].value,"%d",HW_VER);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"UPR");
	sprintf(g_param_queue[g_qpos].value,"%d",g_uprate);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"USB");
	sprintf(g_param_queue[g_qpos].value,"%01d",(usbStatus && usb_retry==0)?1:0);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"SD");
	sprintf(g_param_queue[g_qpos].value,"%d",fre_sect/2);
	g_paramupdate=1;
}

/*
 * Send Parameters Queue to MQTT server
 */
void ParamQueueToMQTT( Time time) //Upload Param to MQTT or Save
{
	if(g_paramupdate==1 && upload_pnt <= g_qpos && g_qpos>0)
	{
		mqtt_debug_send("ParamQueueToMQTT\n");
		char tmp[4];
		uint32_t pos=0;
		memset(SendParameterstoMqtt,0,sizeof(SendParameterstoMqtt));
		sprintf(SendParameterstoMqtt,"@>%03d%%%04d%02d%02d%02d%02d%02d\t",pos,time.year,time.month,time.day,time.hour,time.minute,time.second);
		pos = strlen(SendParameterstoMqtt);

		while(pos<MQTT_BUFF_SIZE-15 && upload_pnt <= g_qpos)
		{
			sprintf(SendParameterstoMqtt+pos,"%s:%s\t",g_param_queue[upload_pnt].code,g_param_queue[upload_pnt].value);
			pos = strlen(SendParameterstoMqtt);
			upload_pnt++;
		}
		pos -= 5;
		sprintf(tmp,"%03d",pos);
		SendParameterstoMqtt[2] = tmp[0];
		SendParameterstoMqtt[3] = tmp[1];
		SendParameterstoMqtt[4] = tmp[2];

		if(g_forcesend == 1 || flag_sync_time == 0 || flag_sync_time == 1)
		{
			HAL_UART_Transmit(&huart1,(uint8_t*)SendParameterstoMqtt,strlen(SendParameterstoMqtt),1000);
			g_forcesend =0;
			g_isMqttPublished=0;
		}
		else
		{

			if(g_isMqttPublished==0)
			{
				if(BSP_SD_Init()==MSD_OK)
				{
					SaveData(&g_q,SendParameterstoMqtt);
				}
			}
			g_isMqttPublished=0;
			HAL_UART_Transmit(&huart1,(uint8_t*)SendParameterstoMqtt,strlen(SendParameterstoMqtt),1000);
		}
//		mqtt_debug_send(SendParameterstoMqtt);
		if(upload_pnt> g_qpos) //finish process queue
		{
			g_paramupdate=0;
		}

	}
	if(g_isMqttPublished==1)
				{
					static uint32_t send_data_saved_period =0;
					if(HAL_GetTick()- send_data_saved_period >= 30000)
					{
						send_data_saved_period = HAL_GetTick();
						if(BSP_SD_Init()==MSD_OK)
						{
							if(QueueIsEmpty(&g_q) == 0)
								SendData(&g_q,g_NbMessUp);
						}
					}

				}
}
