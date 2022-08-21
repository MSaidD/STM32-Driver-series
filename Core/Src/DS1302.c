/*
 * DS1302.c
 *
 *  Created on: Aug 20, 2022
 *      Author: mdarici
 */

#include "DS1302.h"

// Write Register Address
#define DS1302_SEC				0x80
#define DS1302_MIN				0x82
#define DS1302_HOUR				0x84
#define DS1302_DATE				0x86
#define DS1302_MONTH			0x88
#define DS1302_DAY				0x8a
#define DS1302_YEAR				0x8c
#define DS1302_CONTROL		0x8e
#define DS1302_CHARGER		0x90
#define DS1302_CLKBURST		0xbe
#define DS1302_RAMBURST 	0xfe

#define RAMSIZE 					0x31	// Ram Size in bytes
#define DS1302_RAMSTART		0xc0 	// First Address

// GPIO Pins
#define DS1302_GPIO	GPIOC
#define DS1302_SCLK	GPIO_PIN_0
#define DS1302_SDA	GPIO_PIN_1
#define DS1302_RST	GPIO_PIN_2

uint8_t BCD2DEC(uint8_t bcd) {
  return (10 * ((bcd & 0xF0) >> 4) + (bcd & 0x0F));
}

uint8_t DEC2BCD(uint8_t dec) {
  const uint8_t tens = dec / 10;
  const uint8_t ones = dec % 10;
  return (tens << 4) | ones;
}

void pulse_clk(void){
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK, GPIO_PIN_RESET);
}

// SDA Write(output) Mode
void writeSDA(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = DS1302_SDA;
	GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(DS1302_GPIO, &GPIO_InitStructure);
}

// SDA Read(input) Mode
void readSDA(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = DS1302_SDA;
	GPIO_InitStructure.Mode =  GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull = GPIO_PULLDOWN;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(DS1302_GPIO, &GPIO_InitStructure);
}

/* Sends an address or command */
void DS1302_SendCmd(uint8_t cmd) {
	uint8_t i;
	for (i = 0; i < 8; i ++)
	{
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA, (cmd & 1) ?  GPIO_PIN_SET :  GPIO_PIN_RESET);
		pulse_clk();
		cmd >>= 1;
	}
}


/* Writes a byte to 'addr' */
void DS1302_WriteByte(uint8_t addr, uint8_t d)
{
	uint8_t i;

	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);
	DS1302_SendCmd(addr);	// Sends address

	for (i = 0; i < 8; i ++)
	{
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA, (d & 1) ?  GPIO_PIN_SET :  GPIO_PIN_RESET);
		pulse_clk();
		d >>= 1;
	}

	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA,  GPIO_PIN_RESET);
}


/* Sends 'cmd' command and writes in burst mode 'len' bytes from 'temp' */
void DS1302_WriteBurst(uint8_t cmd, uint8_t len, uint8_t * temp)
{
	uint8_t i, j;

	DS1302_WriteByte(DS1302_CONTROL,0x00);			// Disable write protection
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);
	DS1302_SendCmd(cmd);	// Sends burst write command

	for(j = 0; j < len; j++) {
		for (i = 0; i < 8; i ++)
		{
			HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA, (temp[j] & 1) ?  GPIO_PIN_SET :  GPIO_PIN_RESET);
			pulse_clk();
			temp[j] >>= 1;
		}
	}

	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA,  GPIO_PIN_RESET);
	DS1302_WriteByte(DS1302_CONTROL,0x80);			// Enable write protection
}


/* Reads a byte from addr */
uint8_t DS1302_ReadByte(uint8_t addr)
{
	uint8_t i;
	uint8_t temp = 0;

	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);
	addr = addr | 0x01; // Generate Read Address
	DS1302_SendCmd(addr);	// Sends address
	readSDA();
	for (i = 0; i < 8; i ++)
	{
		temp >>= 1;

		if(HAL_GPIO_ReadPin(DS1302_GPIO, DS1302_SDA))
			temp |= 0x80;
		pulse_clk();
	}
	writeSDA();
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA,  GPIO_PIN_RESET);
	return temp;
}


/* Sends 'cmd' command and reads in burst mode 'len' bytes into 'temp' */
void DS1302_ReadBurst(uint8_t cmd, uint8_t len, uint8_t * temp)
{
	uint8_t i, j;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);
	cmd = cmd | 0x01; // Generate read command
	DS1302_SendCmd(cmd);	// Sends burst read command
	readSDA();
	for (j = 0; j < len; j ++) {
		temp[j] = 0;
		for (i = 0; i < 8; i ++)
		{
			temp[j] >>= 1;
			if(HAL_GPIO_ReadPin(DS1302_GPIO, DS1302_SDA))
				temp[j] |= 0x80;
			pulse_clk();
		}
	}
	writeSDA();
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA,  GPIO_PIN_RESET);
}


/* Writes time byte by byte from 'buf' */
void DS1302_WriteTime(uint8_t *buf)
{
	DS1302_WriteByte(DS1302_CONTROL,0x00);			// Disable write protection

	DS1302_WriteByte(DS1302_SEC,0x80);
	DS1302_WriteByte(DS1302_YEAR,DEC2BCD(buf[1]));
	DS1302_WriteByte(DS1302_MONTH,DEC2BCD(buf[2]));
	DS1302_WriteByte(DS1302_DATE,DEC2BCD(buf[3]));
	DS1302_WriteByte(DS1302_HOUR,DEC2BCD(buf[4]));
	DS1302_WriteByte(DS1302_MIN,DEC2BCD(buf[5]));
	DS1302_WriteByte(DS1302_SEC,DEC2BCD(buf[6]));
	DS1302_WriteByte(DS1302_DAY,DEC2BCD(buf[7]));
	DS1302_WriteByte(DS1302_CONTROL,0x80);			// Enable write protection

}

/* Reads time byte by byte to 'buf' */
void DS1302_ReadTime(uint8_t *buf)
{
   	uint8_t tmp;

	tmp = DS1302_ReadByte(DS1302_YEAR);
	buf[1] = BCD2DEC(tmp);
	tmp = DS1302_ReadByte(DS1302_MONTH);
	buf[2] = BCD2DEC(tmp);
	tmp = DS1302_ReadByte(DS1302_DATE);
	buf[3] = BCD2DEC(tmp);
	tmp = DS1302_ReadByte(DS1302_HOUR);
	buf[4] = BCD2DEC(tmp);
	tmp = DS1302_ReadByte(DS1302_MIN);
	buf[5] = BCD2DEC(tmp);
	tmp = DS1302_ReadByte((DS1302_SEC))&0x7F;
	buf[6] = BCD2DEC(tmp);
	tmp = DS1302_ReadByte(DS1302_DAY);
	buf[7] = BCD2DEC(tmp);
}

/* Initialization */
void DS1302_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = DS1302_SCLK | DS1302_SDA | DS1302_RST;
	GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(DS1302_GPIO, &GPIO_InitStructure);

	DS1302_WriteByte(DS1302_CHARGER,0x00);			// Disable Trickle Charger

	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);

}

/* Writes 'val' to ram address 'addr' */
/* Ram addresses range from 0 to 30 */
void DS1302_WriteRam(uint8_t addr, uint8_t val) {
	DS1302_WriteByte(DS1302_CONTROL,0x00);	// Disable write protection
	if (addr >= RAMSIZE) {
		return;
	}

	DS1302_WriteByte(DS1302_RAMSTART + (2 * addr), val);
	DS1302_WriteByte(DS1302_CONTROL,0x80);			// Enable write protection
}

/* Reads ram address 'addr' */
uint8_t DS1302_ReadRam(uint8_t addr) {
	if (addr >= RAMSIZE) {
		return 0;
	}

	return DS1302_ReadByte(DS1302_RAMSTART + (2 * addr));
}

/* Clears the entire ram writing 0 */
void DS1302_ClearRam(void) {
	uint8_t i;
	for(i=0; i< RAMSIZE; i++){
		DS1302_WriteRam(i,0x00);
	}
}

/* Reads time in burst mode, includes control byte */
void DS1302_ReadTimeBurst(uint8_t * buf) {
	uint8_t temp[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	DS1302_ReadBurst(DS1302_CLKBURST, 8, temp);

	buf[1] = BCD2DEC(temp[6]);	// Year
	buf[2] = BCD2DEC(temp[4]);	// Month
	buf[3] = BCD2DEC(temp[3]);	// Date
	buf[4] = BCD2DEC(temp[2]);	// Hour
	buf[5] = BCD2DEC(temp[1]);	// Min
	buf[6] = BCD2DEC(temp[0]);	// Sec
	buf[7] = BCD2DEC(temp[5]);	// Day
	buf[0] = temp[7]; 					// Control
}

/* Writes time in burst mode, includes control byte */
void DS1302_WriteTimeBurst(uint8_t * buf) {
	uint8_t temp[8];

	temp[0]=DEC2BCD(buf[6]);	// Sec
	temp[1]=DEC2BCD(buf[5]);	// Min
	temp[2]=DEC2BCD(buf[4]);	// Hour
	temp[3]=DEC2BCD(buf[3]);	// Date
	temp[4]=DEC2BCD(buf[2]);	// Month
	temp[5]=DEC2BCD(buf[7]);	// Day
	temp[6]=DEC2BCD(buf[1]);	// Year
	temp[7]=buf[0];				// Control

	DS1302_WriteBurst(DS1302_CLKBURST, 8, temp);
}

/* Reads ram in burst mode 'len' bytes into 'buf' */
void DS1302_ReadRamBurst(uint8_t len, uint8_t * buf) {
	uint8_t i;
	if(len <= 0) {
		return;
	}
	if (len > RAMSIZE) {
		len = RAMSIZE;
	}
	for(i = 0; i < len; i++) {
		buf[i] = 0;
	}
	DS1302_ReadBurst(DS1302_RAMBURST, len, buf);
}

/* Writes ram in burst mode 'len' bytes from 'buf' */
void DS1302_WriteRamBurst(uint8_t len, uint8_t * buf) {
	if(len <= 0) {
		return;
	}
	if (len > RAMSIZE) {
		len = RAMSIZE;
	}
	DS1302_WriteBurst(DS1302_RAMBURST, len, buf);
}


