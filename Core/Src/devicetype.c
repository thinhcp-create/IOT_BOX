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
uint32_t g_hallet_tick=0;
uint16_t DeviceRegs[DEVICE_REGISTERS_NUMBER];
uint16_t g_NbMessUp = 20;
uint8_t g_regsupdate;
uint16_t g_qpos; //Current param pointer in queque
uint8_t upload_pnt; //current param upload
uint8_t g_paramupdate=0;
float g_24V;
float g_4V2;
param_value g_param_queue[PARAMETER_QUEUE_SIZE];
uint8_t flag_handle_csv_done=0;

extern uint8_t buffer[];
extern Time hallet_time,utc_time,adjust_time;
extern TimeDifference time_diff;
extern uint8_t flag_sync_time;
extern uint8_t flag_handle_csv;
extern uint8_t g_forcesend;
extern uint8_t g_isMqttPublished;
extern UART_HandleTypeDef huart1;
extern char SendParameterstoMqtt[MQTT_BUFF_SIZE];
extern LIFO_inst g_q;
extern DWORD fre_clust,fre_sect;

const char *params[] = {
        "TS", "UD", "UVT", "UVI", "LL", "LW", "RL", "RW", "PT",
        "ST", "WAT", "LT", "AL", "WN", "OT", "24V", "IT", "FUV", "FCB",
        "H1", "H2", "SS", "PS", "UVF", "WPS", "PV", "ICB", "ICT",
        "ICE", "ICG", "ICC", "IOG"
    };
    const char *alarms[] ={
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
    const char *warns[] = {
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
void FS_FileOperations()
{
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
	DIR dir;
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
			res = f_mount(NULL, (TCHAR const*)USERPath, 1);
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
				    if(fre_sect <= 200*100)
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
				}else  res = f_open(&SDFile, filename, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
				f_lseek(&SDFile, f_size(&SDFile));
				res = FR_DISK_ERR;
//				mqtt_debug_send((char *)second_line);
				res = f_write(&SDFile,(char *)second_line,strlen((char *)second_line),&bw);
				if (res == FR_OK)
				{
					mqtt_debug_send("Write data to SD successed\n");
				}
				f_close(&SDFile);
				f_mount(NULL, (TCHAR const*)SDPath, 1);
				RAM_FATFS_Init();
			}
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

}

void ParseData(const char* input, char* filename,uint16_t * Value) {
	if (sscanf(filename, "%04d%02d%02d.CSV", &hallet_time.year, &hallet_time.month, &hallet_time.day)!=3)
		return;
    char buffer[256];  // Tạo bản sao của chuỗi đầu vào
    strncpy(buffer, input, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0'; // Đảm bảo kết thúc chuỗi

    if (sscanf(buffer, "%d:%d:%d", &hallet_time.hour, &hallet_time.minute, &hallet_time.second)!=3)
    		return;
    if (flag_sync_time==1)
    {
       flag_sync_time = 2;
       time_diff = calculate_time_difference(hallet_time, utc_time);
    }
    adjust_time = add_time_difference(hallet_time, time_diff);

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
    flag_handle_csv_done = 1;
}

void Device_Handler()
{
	Hallet_Program();
	ParamQueueToMQTT();
}

Hallet_Program()
{
	if( flag_sync_time == 0 || (g_isMqttPublished == 0 && flag_sync_time != 2))
	{
		if((HAL_GetTick()-g_alive_tick) > ESP_FORCESEND_PERIOD || g_forcesend == 1)
		{
			g_alive_tick = HAL_GetTick();
			Hallet_RegsToParam(0);
			g_forcesend = 1;
		}
	}
	else
	{
		if((HAL_GetTick()-g_alive_tick) > KEEP_ALIVE_PERIOD || g_forcesend == 1)
		{
			g_alive_tick = HAL_GetTick();
			Hallet_RegsToParam(0);
			g_forcesend = 1;
		}
	}
	if(flag_handle_csv ==1 && g_forcesend == 0 || HAL_GetTick()-g_hallet_tick>=90*1000)
	{
		g_hallet_tick = HAL_GetTick();
		FS_FileOperations();
		if (flag_handle_csv_done)
		{
			memset(DeviceRegs,0,sizeof(DeviceRegs));
			Hallet_RegsToParam(flag_handle_csv_done);
		}
	}
}

uint8_t getBit(unsigned int value, int index) {
    // Kiểm tra bit tại vị trí index và trả v�? chuỗi tương ứng
    return ((value >> index) & 1) ? 1 : 0;
}

void Hallet_RegsToParam(uint8_t sts)
{
	g_qpos=0;
	upload_pnt=0;
	flag_handle_csv_done = 0;
	uint16_t alarm=0,warn=0;
	if(sts)
	{
		for(uint8_t i=0; i<sizeof(params)/sizeof(params[0]); i++)
		{
			sprintf(g_param_queue[i].code,"%s",params[i]);
			sprintf(g_param_queue[i].value,"%d",DeviceRegs[i]);
			if (strcmp(params[i],"AL")==0) alarm = DeviceRegs[i];
			if (strcmp(params[i],"WN")==0) warn = DeviceRegs[i];
			g_qpos++;
		}
		for(uint8_t i=0; i<sizeof(alarms)/sizeof(alarms[0]); i++)
		{
			if(strcmp(alarms[i],"Not_Used_0")!=0)
			{
				sprintf(g_param_queue[g_qpos].code,"%s",alarms[i]);
				sprintf(g_param_queue[g_qpos].value,"%d",getBit(alarm, i));
				g_qpos++;
			}

		}
		for(uint8_t i=0; i<sizeof(warns)/sizeof(warns[0]); i++)
		{
			if(strcmp(warns[i],"Not_Used_1")!=0 && strcmp(warns[i],"Not_Used_2")!=0 )
			{
				sprintf(g_param_queue[g_qpos].code,"%s",warns[i]);
				sprintf(g_param_queue[g_qpos].value,"%d",getBit(warn, i));
				g_qpos++;
			}

		}


	}
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
	sprintf(g_param_queue[g_qpos].value,"%d",TYPE);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"MFW");
	sprintf(g_param_queue[g_qpos].value,"%d",FW_VER);
	g_qpos++;
	sprintf(g_param_queue[g_qpos].code,"HW");
	sprintf(g_param_queue[g_qpos].value,"%d",HW_VER);
//	g_qpos++;
//	sprintf(g_param_queue[g_qpos].code,"UPR");
//	sprintf(g_param_queue[g_qpos].value,"%d",g_uprate);

	g_paramupdate=1;
}

/*
 * Send Parameters Queue to MQTT server
 */
void ParamQueueToMQTT() //Upload Param to MQTT or Save
{
	if(g_paramupdate==1 && upload_pnt <= g_qpos && g_qpos>0)
	{
		char tmp[4];
		uint32_t pos=0;
		memset(SendParameterstoMqtt,0,sizeof(SendParameterstoMqtt));
		if(flag_sync_time == 1 || flag_sync_time == 0 )
		{
			sprintf(SendParameterstoMqtt,"@>%03d%%%04d%02d%02d%02d%02d%02d\t",pos,utc_time.year,utc_time.month,utc_time.day,utc_time.hour,utc_time.minute,utc_time.second);
		}
		if(flag_sync_time == 2)
		{
			sprintf(SendParameterstoMqtt,"@>%03d%%%04d%02d%02d%02d%02d%02d\t",pos,adjust_time.year,adjust_time.month,adjust_time.day,adjust_time.hour,adjust_time.minute,adjust_time.second);
		}
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

		if(g_forcesend == 1 ||flag_sync_time == 0 || flag_sync_time == 1)
		{
			HAL_UART_Transmit(&huart1,(uint8_t*)SendParameterstoMqtt,strlen(SendParameterstoMqtt),1000);
			g_forcesend =0;
		}
		else
		{
			if(g_isMqttPublished==1)
			{
				if(BSP_SD_Init()==MSD_OK)
				{
					SendData(&g_q,g_NbMessUp);
				}
			}
			else
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
}

