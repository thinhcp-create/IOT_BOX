/*
 * storage.h
 *
 *  Created on: Dec 17, 2024
 *      Author: Salmon1611
 */

#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_
#include "stdlib.h"
#include "main.h"



void ReadData(LIFO_inst *q);
void SendData(LIFO_inst *q, uint8_t nmb_send_element);
void SaveData(LIFO_inst *q, char * data);
void SavePointer(LIFO_inst* q);
void LoadPointer(LIFO_inst* q);
uint8_t QueueIsFull(LIFO_inst *q);
uint8_t QueueIsEmpty(LIFO_inst *q);


#endif /* INC_STORAGE_H_ */
