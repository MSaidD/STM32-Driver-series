/*
 * DS1302.h
 *
 *  Created on: Aug 20, 2022
 *      Author: mdarici
 */

#ifndef INC_DS1302_H_
#define INC_DS1302_H_

#include "stm32f3xx_hal.h"
#include "main.h"

typedef enum
{
	SUNDAY = 1,
	MONDAY = 2,
	TUESDAY = 3,
	WEDNESDAY = 4,
	THURSDAY = 5,
	FRIDAY = 6,
	SATURDAY = 7
} days;

/* Initialization */
/* GPIO and DWT */
void DS1302_Init(void);

uint8_t DEC2BCD(uint8_t dec);

uint8_t BCD2DEC(uint8_t bcd);

/* Reads time byte by byte to 'buf' */
void DS1302_ReadTime(uint8_t *buf);

/* Writes time byte by byte from 'buf' */
void DS1302_WriteTime(uint8_t *buf);

/* Writes 'val' to ram address 'addr' */
/* Ram addresses range from 0 to 30 */
void DS1302_WriteRam(uint8_t addr, uint8_t val);

/* Reads ram address 'addr' */
uint8_t DS1302_ReadRam(uint8_t addr);

/* Clears the entire ram writing 0 */
void DS1302_ClearRam(void);

/* Reads time in burst mode, includes control byte */
void DS1302_ReadTimeBurst(uint8_t * temp);

/* Writes time in burst mode, includes control byte */
void DS1302_WriteTimeBurst(uint8_t * buf);

/* Reads ram in burst mode 'len' bytes into 'buf' */
void DS1302_ReadRamBurst(uint8_t len, uint8_t * buf);

/* Writes ram in burst mode 'len' bytes from 'buf' */
void DS1302_WriteRamBurst(uint8_t len, uint8_t * buf);

#endif /* INC_DS1302_H_ */
