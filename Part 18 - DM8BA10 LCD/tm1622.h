#ifndef TM1622_H
#define TM1622_H

#include "ch32v00x.h"
#include <stdint.h>

/* Pin assignment */
#define TM1622_CS_PORT      GPIOD
#define TM1622_CS_PIN       GPIO_Pin_2 //PD2
#define TM1622_WR_PORT      GPIOD
#define TM1622_WR_PIN       GPIO_Pin_3 //PD3
#define TM1622_DATA_PORT    GPIOD
#define TM1622_DATA_PIN     GPIO_Pin_4 //PD4

/* Commands */
#define TM1622_CMD_SYS_DIS  0x00
#define TM1622_CMD_SYS_EN   0x01
#define TM1622_CMD_LCD_OFF  0x02
#define TM1622_CMD_LCD_ON   0x03
#define TM1622_CMD_RC_32K   0x18

#define TM1622_NUM_DIGITS   10
#define TM1622_RAM_NIBBLES  48   

void TM1622_Init(void); //Initialize the display
void TM1622_ClearAll(void); //Clear all characters (does not clear decimals)
void TM1622_ClearAllDecimals(void); //Clear all decimals (does not clear characters)
void TM1622_SetDecimalAtPosition(uint8_t logicalPos); //Set the decimal point to the desired position
void TM1622_SetDecimalInGroup(uint8_t group, uint8_t index); //Set decimal point within the address group

void TM1622_WriteNibble(uint8_t addr, uint8_t data); //Write a nibble (4-bit chunk) on the display
void TM1622_WriteDigitRaw(uint8_t digit, uint16_t segData); //Write a raw digit (4 nibbles) on the display
void TM1622_PrintChar(uint8_t pos, char c); //Print a character based on the font[] lookup table
void TM1622_PrintString(uint8_t startPos, const char *str); //Print a string of characters
void TM1622_PrintNumberString(const char *str); //Print a string of numbers
void TM1622_TestSegment(uint8_t digit); //Test all segments of the display sequentially
