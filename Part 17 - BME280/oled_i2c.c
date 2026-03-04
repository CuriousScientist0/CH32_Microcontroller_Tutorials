#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "i2c.h"
#include "oled_i2c.h"

//uint8_t oled_i2caddress = 0x3C << 1; //096 OLED Display address
uint8_t oled_i2caddress = 0x78 << 1; //042 OLED Display address

//Font table
const uint8_t font[97][5] =
{
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, // " " 0x20
    { 0x00, 0x00, 0x4f, 0x00, 0x00 }, // !   0x21
    { 0x00, 0x07, 0x00, 0x07, 0x00 }, // "   0x22
    { 0x14, 0x7f, 0x14, 0x7f, 0x14 }, // #   0x23
    { 0x24, 0x2a, 0x7f, 0x2a, 0x12 }, // $   0x24
    { 0x23, 0x13, 0x08, 0x64, 0x62 }, // %   0x25
    { 0x36, 0x49, 0x55, 0x22, 0x50 }, // &   0x26
    { 0x00, 0x05, 0x03, 0x00, 0x00 }, // '   0x27
    { 0x00, 0x1c, 0x22, 0x41, 0x00 }, // (   0x28
    { 0x00, 0x41, 0x22, 0x1c, 0x00 }, // )   0x29
    { 0x14, 0x08, 0x3e, 0x08, 0x14 }, // *   0x2A
    { 0x08, 0x08, 0x3e, 0x08, 0x08 }, // +   0x2B
    { 0x00, 0x50, 0x30, 0x00, 0x00 }, // ,   0x2C
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, // -   0x2D
    { 0x00, 0x60, 0x60, 0x00, 0x00 }, // .   0x2E
    { 0x20, 0x10, 0x08, 0x04, 0x02 }, // /   0x2F
    { 0x3e, 0x51, 0x49, 0x45, 0x3e }, // 0   0x30
    { 0x00, 0x42, 0x7f, 0x40, 0x00 }, // 1   0x31
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 2   0x32
    { 0x21, 0x41, 0x45, 0x4b, 0x31 }, // 3   0x33
    { 0x18, 0x14, 0x12, 0x7f, 0x10 }, // 4   0x34
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 5   0x35
    { 0x3c, 0x4a, 0x49, 0x49, 0x30 }, // 6   0x36
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 7   0x37
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 8   0x38
    { 0x06, 0x49, 0x49, 0x29, 0x1e }, // 9   0x39
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, // :   0x3A
    { 0x00, 0x56, 0x36, 0x00, 0x00 }, // ;   0x3B
    { 0x08, 0x14, 0x22, 0x41, 0x00 }, // <   0x3C
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, // =   0x3D
    { 0x00, 0x41, 0x22, 0x14, 0x08 }, // >   0x3E
    { 0x02, 0x01, 0x51, 0x09, 0x06 }, // ?   0x3F
    { 0x32, 0x49, 0x79, 0x41, 0x3e }, // @   0x40
    { 0x7e, 0x11, 0x11, 0x11, 0x7e }, // A   0x41
    { 0x7f, 0x49, 0x49, 0x49, 0x36 }, // B   0x42
    { 0x3e, 0x41, 0x41, 0x41, 0x22 }, // C   0x43
    { 0x7f, 0x41, 0x41, 0x22, 0x1c }, // D   0x44
    { 0x7f, 0x49, 0x49, 0x49, 0x41 }, // E   0x45
    { 0x7f, 0x09, 0x09, 0x09, 0x01 }, // F   0x46
    { 0x3e, 0x41, 0x49, 0x49, 0x7a }, // G   0x47
    { 0x7f, 0x08, 0x08, 0x08, 0x7f }, // H   0x48
    { 0x00, 0x41, 0x7f, 0x41, 0x00 }, // I   0x49
    { 0x20, 0x40, 0x41, 0x3f, 0x01 }, // J   0x4A
    { 0x7f, 0x08, 0x14, 0x22, 0x41 }, // K   0x4B
    { 0x7f, 0x40, 0x40, 0x40, 0x40 }, // L   0x4C
    { 0x7f, 0x02, 0x0c, 0x02, 0x7f }, // M   0x4D
    { 0x7f, 0x04, 0x08, 0x10, 0x7f }, // N   0x4E
    { 0x3e, 0x41, 0x41, 0x41, 0x3e }, // O   0x4F
    { 0x7f, 0x09, 0x09, 0x09, 0x06 }, // P   0X50
    { 0x3e, 0x41, 0x51, 0x21, 0x5e }, // Q   0X51
    { 0x7f, 0x09, 0x19, 0x29, 0x46 }, // R   0X52
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, // S   0X53
    { 0x01, 0x01, 0x7f, 0x01, 0x01 }, // T   0X54
    { 0x3f, 0x40, 0x40, 0x40, 0x3f }, // U   0X55
    { 0x1f, 0x20, 0x40, 0x20, 0x1f }, // V   0X56
    { 0x3f, 0x40, 0x38, 0x40, 0x3f }, // W   0X57
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, // X   0X58
    { 0x07, 0x08, 0x70, 0x08, 0x07 }, // Y   0X59
    { 0x61, 0x51, 0x49, 0x45, 0x43 }, // Z   0X5A
    { 0x00, 0x7f, 0x41, 0x41, 0x00 }, // [   0X5B
    { 0x02, 0x04, 0x08, 0x10, 0x20 }, // "\" 0X5C
    { 0x00, 0x41, 0x41, 0x7f, 0x00 }, // ]   0X5D
    { 0x04, 0x02, 0x01, 0x02, 0x04 }, // ^   0X5E
    { 0x40, 0x40, 0x40, 0x40, 0x40 }, // _   0X5F
    { 0x00, 0x01, 0x02, 0x04, 0x00 }, // `   0X60
    { 0x20, 0x54, 0x54, 0x54, 0x78 }, // a   0X61
    { 0x7f, 0x48, 0x44, 0x44, 0x38 }, // b   0X62
    { 0x38, 0x44, 0x44, 0x44, 0x20 }, // c   0X63
    { 0x38, 0x44, 0x44, 0x48, 0x7f }, // d   0X64
    { 0x38, 0x54, 0x54, 0x54, 0x18 }, // e   0X65
    { 0x08, 0x7e, 0x09, 0x01, 0x02 }, // f   0X66
    { 0x0c, 0x52, 0x52, 0x52, 0x3e }, // g   0X67
    { 0x7f, 0x08, 0x04, 0x04, 0x78 }, // h   0X68
    { 0x00, 0x44, 0x7d, 0x40, 0x00 }, // i   0X69
    { 0x20, 0x40, 0x44, 0x3d, 0x00 }, // j   0X6A
    { 0x7f, 0x10, 0x28, 0x44, 0x00 }, // k   0X6B
    { 0x00, 0x41, 0x7f, 0x40, 0x00 }, // l   0X6C
    { 0x7c, 0x04, 0x18, 0x04, 0x78 }, // m   0X6D
    { 0x7c, 0x08, 0x04, 0x04, 0x78 }, // n   0X6E
    { 0x38, 0x44, 0x44, 0x44, 0x38 }, // o   0X6F
    { 0x7c, 0x14, 0x14, 0x14, 0x08 }, // p   0X70
    { 0x08, 0x14, 0x14, 0x18, 0x7c }, // q   0X71
    { 0x7c, 0x08, 0x04, 0x04, 0x08 }, // r   0X72
    { 0x48, 0x54, 0x54, 0x54, 0x20 }, // s   0X73
    { 0x04, 0x3f, 0x44, 0x40, 0x20 }, // t   0X74
    { 0x3c, 0x40, 0x40, 0x20, 0x7c }, // u   0X75
    { 0x1c, 0x20, 0x40, 0x20, 0x1c }, // v   0X76
    { 0x3c, 0x40, 0x30, 0x40, 0x3c }, // w   0X77
    { 0x44, 0x28, 0x10, 0x28, 0x44 }, // x   0X78
    { 0x0c, 0x50, 0x50, 0x50, 0x3c }, // y   0X79
    { 0x44, 0x64, 0x54, 0x4c, 0x44 }, // z   0X7A
    { 0x00, 0x08, 0x36, 0x41, 0x00 }, // {   0X7B
    { 0x00, 0x00, 0x7f, 0x00, 0x00 }, // |   0X7C
    { 0x00, 0x41, 0x36, 0x08, 0x00 }, // }   0X7D
    { 0x08, 0x08, 0x2a, 0x1c, 0x08 }, // ->  0X7E
    { 0x08, 0x1c, 0x2a, 0x08, 0x08 }, // <-  0X7F
};

void I2C_Write_Command(uint8_t command)
{ 
    uint8_t buf[2] = {0x00,command};
    I2C_SendByte(0x3C, buf, 2);
}

void I2C_Write_Data(uint8_t data)
{
    uint8_t buf[2] = {0x40,data};
    I2C_SendByte(0x3C, buf, 2);
}

void initializeOLED(void) //Initialize SSD1306 OLED (128x32) over I2C
{
    I2C_Write_Command(0xAE); //Display OFF
    I2C_Write_Command(0xD5); //Set display clock divide ratio/oscillator frequency
    I2C_Write_Command(0x80);
    I2C_Write_Command(0xA8); //Set multiplex ratio
    I2C_Write_Command(0x1F); //32 rows
    I2C_Write_Command(0xD3); //Set display offset
    I2C_Write_Command(0x00);
    I2C_Write_Command(0x40); //Set start line
    I2C_Write_Command(0x8D); //Enable charge pump
    I2C_Write_Command(0x14);
    I2C_Write_Command(0xA1); //Segment re-map
    I2C_Write_Command(0xC8); //COM output scan direction
    I2C_Write_Command(0xDA); //COM pins config
    I2C_Write_Command(0x02);
    I2C_Write_Command(0x81); //Contrast
    I2C_Write_Command(0x8F);
    I2C_Write_Command(0xD9); //Pre-charge period
    I2C_Write_Command(0xF1);
    I2C_Write_Command(0xDB); //VCOMH deselect level
    I2C_Write_Command(0x40);
    I2C_Write_Command(0xA4); //Entire display ON
    I2C_Write_Command(0xA6); //Normal display
    I2C_Write_Command(0xAF); //Display ON
}

void initializeOLED042(void) //0.42 OLED
{
    I2C_Write_Command(0xAE); //Display OFF
    //--------------------- (0xD5 and 0x80 belong together)
    I2C_Write_Command(0xA8); //Mux ratio - Not set
    I2C_Write_Command(0x27); //00100111 - 40 (N = 39 + 1 = 40)
    //---------------------
    I2C_Write_Command(0xD3); //Vertical shift by com from 0d-63d
    I2C_Write_Command(0x00); //Reset (no offset)
    //---------------------
    I2C_Write_Command(0x40); //Display RAM display start line register
    //--------------------
    I2C_Write_Command(0xA1); //Segment remap - A1: Column address 127 is mapped to SEG0
    //---------------------
    I2C_Write_Command(0xC8); //Port scan direction, C8: Remapped mode, Scan from COM[N-1] to COM[0]
    //---------------------
    I2C_Write_Command(0xDA); //Set COM pins
    I2C_Write_Command(0x12); //00010010 -> A5 = 1: Enable COM left/right remap, A4 = 0: Sequential COM pin config
    //---------------------

    //---------------------
    I2C_Write_Command(0x81);//Contrast
    I2C_Write_Command(0x80); //128
    //--------------------
    I2C_Write_Command(0xA4); //Entire display ON according to the GDDRAM contents
    //--------------------
    I2C_Write_Command(0xA6); //Normal display (A7 - inverse)
    //--------------------
    I2C_Write_Command(0xD5); //Set display code divide ratio/oscillator frequency
    I2C_Write_Command(0x80); //0b10000000 -> A[3:0]: 0000: ratio = 1, A[7:4]: reset.
    //---------------------
    I2C_Write_Command(0x8D); //Charge pump enable
    I2C_Write_Command(0x14); //0001 0100 -> A2 = 1, enabled. See table 2.1

    I2C_Write_Command(0xAD); //IREF
    I2C_Write_Command(0x30);
    //---------------------
    I2C_Write_Command(0xAF); //ON - Electrical ON (AE - electrical OFF)
}

//Clear the display by zeroing pages 0–3
void eraseDisplay(void)
{
    for(uint8_t page = 0; page < 5; page++)
    {
        I2C_Write_Command(0x22);
        I2C_Write_Command(0xb0+page);
        I2C_Write_Command(0x04);
        I2C_Write_Command(0x0C);
        I2C_Write_Command(0x11);

        for(uint8_t segment = 0; segment < 72; segment++) //All 128 segments (columns)
        {
            I2C_Write_Data(0x00); //Set bits to zero (pixel = OFF)
        }
    }
}

//Draw a single 5x8 character at given page and column
void printCharacter(uint8_t page, uint8_t segment, uint8_t letter)
{
    I2C_Write_Command(0x22);
    I2C_Write_Command(0xb0+page);

    I2C_Write_Command(0x40);

    uint8_t newLower = segment & 0x0F;       //Lower nibble of column start
    uint8_t newHigher = (segment >> 4) & 0x0F; //Higher nibble of column start

    I2C_Write_Command(0x00 | newLower);     //Set lower column start address
    I2C_Write_Command(0x10 | newHigher);    //Set higher column start address

    for(uint8_t i = 0; i < 5 ; i ++)
    {
        I2C_Write_Data(font[letter][i]);
        //{ 0x3c, 0x40, 0x30, 0x40, 0x3c }, //w
    }
}

void printCharacter042(uint8_t page, uint8_t column, uint8_t letter) //page = line, column = column, letter = letter from the font[]
{

    I2C_Write_Command(0x22); //Set page address command
    I2C_Write_Command(0xb0+page); //Actual page adress, iterated by the for() - This is the line number. Page 0 - first line
    //0xb0 = 176.

    I2C_Write_Command(0x04); //Set the right border, end of the screen

    uint16_t address = (0x11 << 4) | 0x0C; //Combine the default lower and higher address bits together
    address += column; //Increment the address by the number of columns amount (unit - pixels (segment))
    uint8_t newLower = address & 0x0F; //Create a new lower bit address by masking out the unnecessary bits
    uint8_t newHigher = (address >> 4) & 0x0F;  //Shift and mask to get the upper 4 bits

    I2C_Write_Command(newLower); //Set lower 4-bits of column address
    I2C_Write_Command(0x10 | newHigher); //Set higher 4-bits of column address

   //Write character data (5 bytes wide)
   for (uint8_t i = 0; i < 5; i++)
   {
       I2C_Write_Data(font[letter][i]);
   }

   //Add a space (1 column) after the character for readability
   I2C_Write_Data(0x00);
}

//Draw a null-terminated string, wrapping to next page if needed
void printText(const uint8_t *receivedBuffer, uint8_t pageStart, uint8_t columnStart)
{
    uint8_t column = columnStart;

    for(uint8_t i = 0; receivedBuffer[i] != '\0'; i++)
    {
        char currentCharacter = receivedBuffer[i];

        if(currentCharacter < 0x20 || currentCharacter > 0x7E)
        {
            printf("Invalid character was received\n");
            continue;
        }

        uint8_t fontPosition = (int)currentCharacter - 0x20;

        printCharacter(pageStart, column, fontPosition);

        column += 6;

        if(column > 128-6)
        {
            column = columnStart;
            pageStart += 1;
        }

        if(pageStart > 4)
        {
            break;
        }
    }
}

void printText042(const uint8_t *receivedBuffer, uint8_t pageStart, uint8_t columnStart)
{
    uint8_t column = columnStart; //Initial column position

       for (uint8_t i = 0; receivedBuffer[i] != '\0'; i++) //Loop until end of the array (assuming '\0' as the end marker)
       {
          char currentChar = receivedBuffer[i]; //Get the ASCII value of the i-th character
          //printf("Char received: %d\n", currentChar); //Check if the parsing is OK, it should print the ASCII value

          if (currentChar < 0x20 || currentChar > 0x7E)
          {
              //printf("Skipping invalid character: 0x%02x\n", currentChar);
              continue; // Skip invalid characters
          }

          uint8_t fontPosition = (int)currentChar - 0x20; //Convert the ASCII to the lookup table position
          //printf("Char operation: %d \n", fontPosition); //Print, just to make sure

          //Call printCharacter with the current page, column, and character data (font position)
          printCharacter042(pageStart, column, fontPosition);

          //Increment column by 6 (5 for the character width + 1 for space) for the next character
          column += 6;

          //If column exceeds the width of the display (72 pixels), move to the next page
          if (column > 72 - 6)
          {
              column = columnStart;  //Reset to the first column
              pageStart += 1;        //Move to the next page
          }

          //Stop if we run out of pages (5 pages for a 40px height display)
          if (pageStart > 4)
          {
              break; //Exit if we run out of space
          }
       }
}

void oledSetPageCol(uint8_t page, uint8_t col)
{
    I2C_Write_Command(0xB0 | page);               //set page address
    I2C_Write_Command(0x00 | (col & 0x0F));       //lower 4 bits of column
    I2C_Write_Command(0x10 | ((col >> 4) & 0x0F));//upper 4 bits of column
}
