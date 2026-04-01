#include "tm1622.h"
#include "debug.h"

/* ---------- timing ---------- */
#define TM1622_SETUP_US     1
#define TM1622_HOLD_US      1
#define TM1622_CLK_US       2

/* ---------- local helpers ---------- */
#define LSB_FIRST           1
#define MSB_FIRST           0

//Shorthand functions for setting pins LOW or HIGH
static inline void CS_H(void)   { GPIO_SetBits(TM1622_CS_PORT, TM1622_CS_PIN); }
static inline void CS_L(void)   { GPIO_ResetBits(TM1622_CS_PORT, TM1622_CS_PIN); }
static inline void WR_H(void)   { GPIO_SetBits(TM1622_WR_PORT, TM1622_WR_PIN); }
static inline void WR_L(void)   { GPIO_ResetBits(TM1622_WR_PORT, TM1622_WR_PIN); }
static inline void DAT_H(void)  { GPIO_SetBits(TM1622_DATA_PORT, TM1622_DATA_PIN); }
static inline void DAT_L(void)  { GPIO_ResetBits(TM1622_DATA_PORT, TM1622_DATA_PIN); }

extern const uint16_t fonts[]; //Refers to the fonts[] that is placed in main.c

static void TM1622_SendBits(uint16_t value, uint8_t bitCount, uint8_t order)
{
    for(uint8_t i = 0; i < bitCount; i++) //Count up to the number of bits
    {
        WR_L(); //Set the WR line LOW

        if(order == LSB_FIRST) //LSB first -> Data needs LSB-first handling
        {
            if(value & 0x0001U) //Check if the lowest bit is 1
            {
                DAT_H(); //If it is 1, set the DATA line high
            }
            else
            {  
                DAT_L(); //Otherwise, set the DATA line LOW
            } 

            value >>= 1; //Shift the value to right by one bit to prepare it for the next loop iteration
        }
        else //MSB first -> Address needs MSB-first handling
        {
            if(value & (1U << (bitCount - 1)))  //Check if the highest bit is 1
            {
                DAT_H(); //If it is 1, set the DATA line high
            } 
            else
            { 
                DAT_L(); //Otherwise, set the DATA line LOW
            }

            value <<= 1; //Shift the value to the left by one bit to prepare it for the next loop iteration
        }

        Delay_Us(TM1622_CLK_US); //Clock pulse delay
        WR_H(); //Set the WR line HIGH -> Data gets latched at this rising edge
        Delay_Us(TM1622_HOLD_US); //Delay
    }
}

static void TM1622_SendCommand(uint8_t cmd)
{
    Delay_Us(TM1622_SETUP_US);
    CS_L();
    Delay_Us(TM1622_SETUP_US);

    /* command mode = 100; see page 8, section 4 table */
    TM1622_SendBits(0b100, 3, MSB_FIRST); //Set command mode
    TM1622_SendBits(cmd, 8, MSB_FIRST); //Send command
    TM1622_SendBits(0, 1, MSB_FIRST);  //Send don't care bit

    Delay_Us(TM1622_SETUP_US);
    CS_H();
}

void TM1622_WriteNibble(uint8_t addr, uint8_t data) //Datasheet Page 9 Chart 3 Write 101
{
    if(addr >= TM1622_RAM_NIBBLES) return; //Check for valid RAM range and if it is invalid, jump out

    Delay_Us(TM1622_SETUP_US); //Wait to make timing stable
    CS_L(); //Pull chip select low to select the chip
    Delay_Us(TM1622_SETUP_US); //Wait to make timing stable

    /* write mode = 101 */
    TM1622_SendBits(0b101, 3, MSB_FIRST); //Send 101 (WRITE) to the chip
    TM1622_SendBits(addr, 6, MSB_FIRST); //Send the RAM address (MSB -> A5, A4, A3, A2, A1, A0)
    TM1622_SendBits(data & 0x0F, 4, LSB_FIRST); //Send the DATA nibble. Mask for the lower 4 bit (nibble)  (LSB -> D0, D1, D2, D3)

    Delay_Us(TM1622_SETUP_US); //Final delay to let things settle
    CS_H(); //Pull CS high to prepare for the next transfer    
    Delay_Us(TM1622_SETUP_US); //Final delay to let things settle
}

void TM1622_SetDecimalAtPosition(uint8_t logicalPos) //Light up the corresponding decimal point
{
    uint8_t group; //Decimals are arranged into 3 groups (addresses)
    uint8_t index; //Each group has 3 positions (indices)

    if(logicalPos >= 9) return; //Treat out-of-range values by ignoring them

    group = logicalPos / 3; //Determine group
    index = logicalPos % 3; //Determine position within group

    TM1622_SetDecimalInGroup(group, index); //Set decimal point
}

void TM1622_ClearAllDecimals(void) //Direct deleting of all decimals
{
    TM1622_WriteNibble(0x29, 0x01);
    TM1622_WriteNibble(0x2B, 0x01);
    TM1622_WriteNibble(0x2D, 0x01);
}

void TM1622_SetDecimalInGroup(uint8_t group, uint8_t index)
{
    uint8_t addr; //Address of the group
    uint8_t data; //nibble of the corresponding decimal point

    switch(group) //Fetch the address of the group where the decimal point is located
    {
        case 0: addr = 0x29; break;
        case 1: addr = 0x2B; break;
        case 2: addr = 0x2D; break;
        default: return;
    }

    if(index > 2) return; //Treat invalid group by neglecting it

    data = (1U << (index + 1)); //Determine the nibble value
    TM1622_WriteNibble(addr, data); //Light up the corresponding decimal point
}

void TM1622_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    gpio.GPIO_Pin   = TM1622_CS_PIN | TM1622_WR_PIN | TM1622_DATA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP; //All pins are push-pull, because we toggle them with the MCU
    gpio.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(TM1622_CS_PORT, &gpio);

    CS_H(); //Set all pins to high - default state
    WR_H();
    DAT_H();

    Delay_Ms(20); //"Safety delay"

    //Initialize the display
    TM1622_SendCommand(TM1622_CMD_SYS_EN); //Turn on the oscillator
    TM1622_SendCommand(TM1622_CMD_RC_32K); //Use the internal oscillator as clock source
    TM1622_SendCommand(TM1622_CMD_LCD_OFF); //Turn OFF LCD
    TM1622_ClearAll();
    TM1622_SendCommand(TM1622_CMD_LCD_ON); //Turn ON LCD
}

static const uint8_t digitAddr[10] = 
{
    0x00, //0-3
    0x04, //4-7
    0x08, //8-11
    0x0C, //12-15
    0x10, //16-19
    0x14, //20-23
    0x18, //24-27
    0x1C, //28-31
    0x20, //32-35
    0x24  //36-39
}; //Each nibble has 4 segments, so we can use the start address of each nibble as a starting point. Every new nibble is 4 units apart from the previous


void TM1622_WriteDigitRaw(uint8_t digit, uint16_t segData)
{
    uint8_t base; //Position-address of the printed digit

    if(digit >= 10) return; //Check if we are printing within the display's limit 

    base = digitAddr[digit]; //Fetch the position-address of the selected digit

    //One number consists of 4 nibbles, we need to send all nibbles and their active segments

    TM1622_WriteNibble(base + 0, (segData >> 12) & 0x0F); //Take the position-address - bits 15-12; keep the upper 4 bits in the nibble with the shifting and mask the junk.
    TM1622_WriteNibble(base + 1, (segData >> 8)  & 0x0F); //Take the position-address - bits 11-8; keep the next 4 bits in the nibble and mask the rest
    TM1622_WriteNibble(base + 2, (segData >> 4)  & 0x0F); //Take the position-address - bits 7-4; keep the next 4 bits in the nibble and mask the rest
    TM1622_WriteNibble(base + 3, (segData >> 0)  & 0x0F); //Take the position-address - bits 3-0; Don't shift

    //Example
    //SegData = 0x9C59 = 1001 1100 0101 1001
    //segData >> 12 = 0x9C59 >> 12 = 0x0009 (Hex bit shifting! The 9 next to x stayed in. 12/4 = 3 -> push the 9 from left to right by 3 steps) then & 0x0F = 0x9
    //segData >> 8 = 0x9C, then & 0x0F = 0xC
    //segData >> 4 = 0x9C5, then & 0x0F = 0x5
    //segData & 0x0f = 0x9
}

void TM1622_ClearAll(void)
{
    for(uint8_t addr = 0; addr < TM1622_RAM_NIBBLES; addr++) //Fill up all the segments with zeroes
    {
        TM1622_WriteNibble(addr, 0x00);
    }
}

static uint16_t TM1622_GetFontForChar(char c)
{ 
    if((uint8_t)c < 0x20 || (uint8_t)c > 0x5D) //Check for "out of range" characters
    {
        return 0x0000; //unsupported -> blank (space)
    }

    uint8_t index = (uint8_t)c - 0x20; //Convert the char's value to an ASCII value
    return fonts[index];
}

void TM1622_PrintChar(uint8_t pos, char c)
{
    uint16_t pattern = TM1622_GetFontForChar(c);

    //Send the 16-bit pattern to the RAM addresses that belong to digit "pos"
    TM1622_WriteDigitRaw(pos, pattern);
}


void TM1622_PrintString(uint8_t startDigit, const char *str)
{
    int8_t d = (int8_t)startDigit;

    while((*str != '\0') && (d >= 0))
    {
        TM1622_PrintChar((uint8_t)d, *str);
        d--; //move left on the display
        str++; //move forward in the string
    }
}

void TM1622_TestSegment(uint8_t digit) //Test all segments within a character
{
    for(int j = 0; j < 10; j++)
    {
        for(uint8_t bit = 0; bit < 16; bit++)
        {
            TM1622_WriteDigitRaw(j, (1U << bit)); //Take a digit (position), and turn on the "bit-th" segment of it at a time
            //1U << bit. It pushes in the "1" by bit steps from the right in a 16-bit number. Rest is zero.
            //1U << 5 == 0000 0000 0010 0000
            Delay_Ms(500);
        }
        TM1622_ClearAll();
    }    
}

void TM1622_PrintCharAtNumericPosition(uint8_t logicalPos, char c)
{
    uint16_t segData = 0x0000; //Clear segment data

    if(logicalPos >= 9) return; //Jump out if the data is invalid    
    
    uint8_t digit = 8-logicalPos; //9 digits (0-8 items), 10th digit (leftmost) is reserved for the minus symbol

    if((uint8_t)c >= 0x20 && (uint8_t)c <= 0x5D) //If the character is within the valid ASCII range
    {
        segData = fonts[(uint8_t)c - 0x20]; //Fetch the font position from the lookup table
    }

    TM1622_WriteDigitRaw(digit, segData); //Print the font
}

void TM1622_PrintNumberString(const char *str)
{
    uint8_t logicalPos = 0; //Digit position
    int8_t lastPrintedPos = -1; //Last printed digit position

    while(*str != '\0' && logicalPos < 9) //Go until max displayable digits or null-terminate
    {
        if(*str == '-') //If there's a minus sign
        {
            TM1622_WriteDigitRaw(9, fonts['-' - 0x20]); //Print the minus sign at the 10th block (0-9 logic)
        }
        else if(*str == '.') //If there is a decimal dot
        {
            if(lastPrintedPos >= 0) //If it makes sense to print decimal dot... (".234" is not valid!)
            {
                TM1622_SetDecimalAtPosition((uint8_t)lastPrintedPos+1); //Print the dot at the corresponding position
            }
        }
        else if(*str >= '0' && *str <= '9') //If digits are being printed
        {
            TM1622_PrintCharAtNumericPosition(logicalPos, *str); //Print the actual digit
            lastPrintedPos = logicalPos; //Update the last printed position
            logicalPos++; //Increment the digit position
        }
        str++; //Increment the string position for the next digit
    }
}
