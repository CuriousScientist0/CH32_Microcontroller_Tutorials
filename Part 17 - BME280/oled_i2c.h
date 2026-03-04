#ifndef OLED_I2C_H
#define OLED_I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t oled_i2caddress; //OLED Display address

//Forward declaration of font table (ASCII 0x20–0x7F)
extern const uint8_t font[97][5];

void I2C_Write_Command(uint8_t command);
void I2C_Write_Data(uint8_t data);

//Initialize the SSD1306 OLED (128×32) display
void initializeOLED(void);

//Initialize the SSD1306 OLED (128×32) display
void initializeOLED042(void);

//Clear the entire display (pages 0–3)
void eraseDisplay(void);

//Draw a single character from font at given page (0–3) and segment (column in bytes)
//'letter' is the index into font (0 = space)
void printCharacter(uint8_t page, uint8_t segment, uint8_t letter);  //page = line, column = column, letter = letter from the font[]
void printCharacter042(uint8_t page, uint8_t column, uint8_t letter);

//Draw a null-terminated ASCII string starting at pageStart and columnStart (in pixels)
//Characters wrap to next page if they exceed width
void printText(const uint8_t *receivedBuffer, uint8_t pageStart, uint8_t columnStart);
void printText042(const uint8_t *receivedBuffer, uint8_t pageStart, uint8_t columnStart);

void oledSetPageCol(uint8_t page, uint8_t col);

#ifdef __cplusplus
}
#endif

#endif // OLED_I2C_H
