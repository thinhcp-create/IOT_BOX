/*
 * storage.c
 *
 *  Created on: Dec 17, 2024
 *      Author: Salmon1611
 */
#include "storage.h"
#include "string.h"
#include "stdio.h"
#include "espcomm.h"
#include "main.h"
#include "fatfs.h"
#include "FLASH_PAGE_F1.h"

extern uint32_t g_NbSector;
extern SD_HandleTypeDef hsd;
char tmp[MQTT_BUFF_SIZE];
extern uint8_t countdb;

uint8_t QueueIsEmpty(LIFO_inst *q)
{
	if (q->pnt_front == 0) return 1;
	return 0;
}

uint8_t QueueIsFull(LIFO_inst *q)
{
	  if ((q->pnt_front == q->pnt_rear + 1)
			  || (q->pnt_front == SD_DATA_SECTOR_BEGIN
					  && q->pnt_rear == SD_DATA_SECTOR_END))
		  return 1;
	  return 0;
}

void SavePointer(LIFO_inst* q)
{
	//Save pointer to internal Flash
	Flash_Write_Data(IFLASH_ADD_PNT_FRONT,(uint32_t *)q,2);
}
void LoadPointer(LIFO_inst* q)
{
	Flash_Read_Data(IFLASH_ADD_PNT_FRONT,(uint32_t *)q,2);


}
void SaveData(LIFO_inst *q, char * data)
{
	if(BSP_SD_Init()==MSD_OK)
	{
		if(QueueIsFull(q))
		{
			 if(q->pnt_front == SD_DATA_SECTOR_BEGIN && q->pnt_rear == SD_DATA_SECTOR_END )
			 {
				 q->pnt_front += 1;
				 q->pnt_rear = SD_DATA_SECTOR_BEGIN;
			 }
			 else if (q->pnt_front == q->pnt_rear + 1)
			 {

				 if(q->pnt_front!= SD_DATA_SECTOR_END) //q->pnt_front is not in the last sector
				 {
					 q->pnt_front+=1;
				 }
				 else
				 {
					 q->pnt_front = SD_DATA_SECTOR_BEGIN;
				 }
				 q->pnt_rear += 1;

			 }
		}
		else
		{
			if(q->pnt_front==0)//first element
			{
				q->pnt_front = SD_DATA_SECTOR_BEGIN;
				q->pnt_rear = SD_DATA_SECTOR_BEGIN;
			}
			else
			{
				if(q->pnt_rear< SD_DATA_SECTOR_END)
				{
					q->pnt_rear += 1;
				}
			}
		}
//		USER_SPI_write(0, (uint8_t*)data, q->pnt_rear, 1);
		HAL_SD_WriteBlocks(&hsd, (uint8_t*)data, q->pnt_rear,1, 100);
//		memset(tmp,0,sizeof(tmp));
//		USER_SPI_read(0, (uint8_t*)tmp, q->pnt_rear, 1);
//		mqtt_debug_send(tmp);

		SavePointer(q);
//		SaveDeviceInfo();
		debugPrint("M[%d] Saved data, front = %d rear = %d",HAL_GetTick()/1000,q->pnt_front,q->pnt_rear);
	}
	else
	{
		debugPrint("M[%d] SaveData - SDcard Failed",HAL_GetTick()/1000);
	}
}

void ReadData(LIFO_inst *q)
{
	if(!QueueIsEmpty(q))
	{
		debugPrint("M[%d] Read Data - front = %d rear = %d",HAL_GetTick()/1000,q->pnt_front,q->pnt_rear);
		debugPrint("M[%d] Last Data at Rear Pointer:",HAL_GetTick()/1000);
		memset(tmp,0,sizeof(tmp));
		HAL_SD_ReadBlocks(&hsd, (uint8_t*)tmp, q->pnt_rear, 1,100);
		mqtt_debug_send(tmp);
	}
	else
	{
		debugPrint("M[%d] Read Data - Queue Empty: front = %d rear = %d",HAL_GetTick()/1000,q->pnt_front,q->pnt_rear);
	}
}

void SendData(LIFO_inst *q, uint8_t nmb_send_element)
{
	if(BSP_SD_Init()==MSD_OK)
	{
		if(!QueueIsEmpty(q))
		{
			debugPrint("M[%d] Send Saved Data",HAL_GetTick()/1000);
			for(uint8_t i =0;i<nmb_send_element;i++)
			{
				countdb++;
				if(QueueIsEmpty(q))
				{

				}
				else
				{
					if(q->pnt_rear> q->pnt_front)
					{
						memset(tmp,0,sizeof(tmp));
//						USER_SPI_read(0, (uint8_t*)tmp, q->pnt_rear, 1);
						HAL_SD_ReadBlocks(&hsd, (uint8_t*)tmp, q->pnt_rear, 1,100);
//						mqtt_saved_data_send(tmp);
						mqtt_debug_send((char *)tmp);

						q->pnt_rear -= 1;
					}
					else if(q->pnt_rear == q->pnt_front)
					{
						memset(tmp,0,sizeof(tmp));
//						USER_SPI_read(0, (uint8_t*)tmp, q->pnt_rear, 1);
						HAL_SD_ReadBlocks(&hsd, (uint8_t*)tmp, q->pnt_rear, 1,100);
//						mqtt_saved_data_send(tmp);
						mqtt_debug_send(tmp);

						q->pnt_front=0;

						debugPrint("M[%d] Queue is Empty",HAL_GetTick()/1000);
						break;

					}
					else if (q->pnt_rear < q->pnt_front)
					{
						if(q->pnt_rear>1)
						{
							memset(tmp,0,sizeof(tmp));
//							USER_SPI_read(0, (uint8_t*)tmp, q->pnt_rear, 1);
							HAL_SD_ReadBlocks(&hsd, (uint8_t*)tmp, q->pnt_rear, 1,100);
//							mqtt_saved_data_send(tmp);
							mqtt_debug_send(tmp);

							q->pnt_rear -= 1;
						}
						else
						{
							memset(tmp,0,sizeof(tmp));
//							USER_SPI_read(0, (uint8_t*)tmp, q->pnt_rear, 1);
							HAL_SD_ReadBlocks(&hsd, (uint8_t*)tmp, q->pnt_rear, 1,100);
//							mqtt_saved_data_send(tmp);
							mqtt_debug_send(tmp);

							q->pnt_rear = g_NbSector;
						}
					}
				}
				debugPrint("M[%d] Send Data - front = %d rear = %d",HAL_GetTick()/1000,q->pnt_front,q->pnt_rear);
			}
		}
		SavePointer(q);
//		SaveDeviceInfo();
	}
	else
	{
		debugPrint("M[%d] SendData - SDcard Failed",HAL_GetTick()/1000);
	}
}
