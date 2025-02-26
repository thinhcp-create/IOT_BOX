
/**
  ***************************************************************************************************************
  ***************************************************************************************************************
  ***************************************************************************************************************
  File:	      FLASH_PAGE_F1.c
  Modifier:   ControllersTech.com
  Updated:    27th MAY 2021
  ***************************************************************************************************************
  Copyright (C) 2017 ControllersTech.com
  This is a free software under the GNU license, you can redistribute it and/or modify it under the terms
  of the GNU General Public License version 3 as published by the Free Software Foundation.
  This software library is shared with public for educational purposes, without WARRANTY and Author is not liable for any damages caused directly
  or indirectly by this software, read more about this on the GNU General Public License.
  ***************************************************************************************************************
*/

#include "FLASH_PAGE_F1.h"
#include "string.h"
#include "stdio.h"


/* STM32F103 have 128 PAGES (Page 0 to Page 127) of 1 KB each. This makes up 128 KB Flash Memory
 * Some STM32F103C8 have 64 KB FLASH Memory, so I guess they have Page 0 to Page 63 only.
 */

/* FLASH_PAGE_SIZE should be able to get the size of the Page according to the controller */
static uint32_t GetPage(uint64_t Address)
{
  for (int indx=0; indx<128; indx++)
  {
	  if((Address < (FLASH_BASE + (FLASH_PAGE_SIZE *(indx+1))) ) && (Address >= (FLASH_BASE + FLASH_PAGE_SIZE*indx)))
	  {
		  return (FLASH_BASE + FLASH_PAGE_SIZE*indx);
	  }
  }

  return 0;
}


uint32_t Flash_Write_Data (uint64_t StartPageAddress, uint32_t *Data, uint16_t numberofwords)
{

	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PAGEError;
	int sofar=0;

	  /* Unlock the Flash to enable the flash control register access *************/
	   HAL_FLASH_Unlock();

	   /* Erase the user Flash area*/

	  uint32_t StartPage = GetPage(StartPageAddress);
	  uint64_t EndPageAdress = StartPageAddress + numberofwords*4;
	  uint32_t EndPage = GetPage(EndPageAdress);

	   /* Fill EraseInit structure*/
	   EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	   EraseInitStruct.PageAddress = StartPage;
	   EraseInitStruct.NbPages     = ((EndPage - StartPage)/FLASH_PAGE_SIZE) +1;

	   if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
	   {
	     /*Error occurred while page erase.*/
		  return HAL_FLASH_GetError ();
	   }

	   /* Program the user Flash area word by word*/

	   while (sofar<numberofwords)
	   {
	     if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, StartPageAddress, Data[sofar]) == HAL_OK)
	     {
	    	 StartPageAddress += 4;  // use StartPageAddress += 2 for half word and 8 for double word
	    	 sofar++;
	     }
	     else
	     {
	       /* Error occurred while writing data in Flash memory*/
	    	 return HAL_FLASH_GetError ();
	     }
	   }

	   /* Lock the Flash to disable the flash control register access (recommended
	      to protect the FLASH memory against possible unwanted operation) *********/
	   HAL_FLASH_Lock();

	   return 0;
}


uint32_t Flash_Write_Array(uint32_t StartAddress, uint8_t *data, uint16_t len)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError;
    uint32_t CurrentAddress = StartAddress;
    uint32_t tempData = 0;
    uint16_t i = 0;

    // Mở khóa Flash
    HAL_FLASH_Unlock();

    // Xóa Flash từ trang bắt đầu
    uint32_t StartPage = StartAddress & ~(FLASH_PAGE_SIZE - 1); // Căn chỉnh về đầu trang
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = StartPage;
    EraseInitStruct.NbPages = ((len + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE); // Tính số trang cần xóa

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
    {
        // Xử lý lỗi xóa
        HAL_FLASH_Lock();
        return HAL_FLASH_GetError();
    }

    // Ghi dữ liệu
    while (i < len)
    {
        // Ghép 4 byte (hoặc ít hơn nếu còn lại < 4 byte)
        tempData = 0;
        for (uint8_t j = 0; j < 4; j++)
        {
            if (i < len)
            {
                tempData |= (data[i] << (8 * j));
                i++;
            }
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, CurrentAddress, tempData) != HAL_OK)
        {
            // Xử lý lỗi ghi
            HAL_FLASH_Lock();
            return HAL_FLASH_GetError();
        }

        CurrentAddress += 4; // Tiến đến địa chỉ tiếp theo (ghi 4 byte mỗi lần)
    }

    // Khóa Flash sau khi ghi xong
    HAL_FLASH_Lock();

    return 0; // Thành công
}
