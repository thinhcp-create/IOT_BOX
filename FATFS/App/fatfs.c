/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
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
#include "fatfs.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */
uint8_t retUSER;    /* Return value for USER */
char USERPath[4];   /* USER logical drive path */
FATFS USERFatFS;    /* File system object for USER logical drive */
FIL USERFile;       /* File object for USER */

/* USER CODE BEGIN Variables */
extern rtc_time g_time;
/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);
  /*## FatFS: Link the USER driver ###########################*/
//  retUSER = FATFS_LinkDriver(&USER_Driver, USERPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
	convert_to_fattime(g_time.year, g_time.month,g_time.date, g_time.hour,g_time.min, g_time.sec);
//  return 0;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */
void SD_FATFS_Init(void)
{
	/*## FatFS: Link the USER driver ###########################*/
	  retUSER = FATFS_UnLinkDriver(USERPath);

	  /* USER CODE BEGIN Init */
	  retSD = FATFS_LinkDriver(&SD_Driver,SDPath);
	  /* additional user code for init */
	  /* USER CODE END Init */
}
void RAM_FATFS_Init(void)
{
	/*## FatFS: Link the USER driver ###########################*/
		retSD = FATFS_UnLinkDriver(SDPath);

	  /* USER CODE BEGIN Init */
	  retUSER = FATFS_LinkDriver(&USER_Driver, USERPath);
	  /* additional user code for init */
	  /* USER CODE END Init */
}
DWORD convert_to_fattime(uint16_t year, uint8_t month, uint8_t day,
                         uint8_t hour, uint8_t minute, uint8_t second) {
    // Kiểm tra năm phải >= 1980 (FATFS chỉ hỗ trợ từ năm 1980 trở đi)
    if (year < 1980) year = 1980;

    // Chuyển đổi sang định dạng FATFS
    return ((DWORD)(year - 1980) << 25) | // Năm tính từ 1980
           ((DWORD)month << 21) |        // Tháng (1–12)
           ((DWORD)day << 16) |          // Ngày (1–31)
           ((DWORD)hour << 11) |         // Gi�? (0–23)
           ((DWORD)minute << 5) |        // Phút (0–59)
           ((DWORD)(second / 2));        // Giây chia 2 (0–29)
}
/* USER CODE END Application */
