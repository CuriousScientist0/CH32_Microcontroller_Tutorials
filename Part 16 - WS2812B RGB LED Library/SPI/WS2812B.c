#include "ch32v00x.h"
#include "ws2812b.h"

#define WS2812_MAX_LEDS     32 //Maximum number of LEDs
#define WS_RESET_US         80 //RET code signal with a margin (reset)
#define WS_RESET_BYTES      30 //1 bit = 333 ns. 1 byte = 2.67 us. 30*2.67 = 80.1 us
#define CODE0  0b1000
#define CODE1  0b1110

static uint8_t spiBuf[WS2812_MAX_LEDS * 12]; //1 RGB LED = 24 bit. 1 LED = 3 color. 1 color = 8 bit. 1 bit color = 4 SPI bit (Code 0/1)
//1 color is 8 * 4 = 32 SPI bits. There are 3 colors => 96 total SPI bits. 96/8 = 12 SPI bytes, hence the 12x multiplier.
static const uint8_t ws_reset_buf[WS_RESET_BYTES] = {0}; //30x zero bytes for creating 80 us long MOSI pulse.

static void InitializeSPI1(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef  SPI_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_SPI1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_5; //PC6 = MOSI (AF push-pull), PC5 = SCK (AF push-pull)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    //Note. No need to define the rest of the pins as we won't use them! I defined SCK, so I can use a logical analyzer to measure the pulses.
    
    GPIO_Init(GPIOC, &GPIO_InitStructure);    

    GPIO_ResetBits(GPIOC, GPIO_Pin_6); //Keep MOSI low when idle (helps reset)

    SPI_I2S_DeInit(SPI1);

    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx; //Only MOSI needed
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; //48/16 = 3MHz (333.3 ns)
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    //Note. Many parameters are unimportant, but they must be initialized. They don't affect the MOSI signals' timings anyway.

    SPI_Cmd(SPI1, ENABLE);
}

static void SPI_TransferBytes(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) 
    {
        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {} //Wait until the transfer register becomes empty

        SPI_I2S_SendData(SPI1, data[i]); //Push out the data
    }
    
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) {} //Wait for the SPI until it is not busy
}

static uint16_t encode_grb_to_spi(const ws2812b_t *strip) //MSB first, GRB order
{    
    uint32_t out = 0; //Return value of the function

    for (uint16_t i = 0; i < strip->count; i++)  //Go until we have LEDs in the strip
    {
        //Read the color of the LED's subpixels
        uint8_t g = strip->pixels[i].g;
        uint8_t r = strip->pixels[i].r;
        uint8_t b = strip->pixels[i].b;

        if (strip->brightness != 255) //Scale the GRB components with the defined global brightness level ("master dimming")
        {
            uint16_t br = strip->brightness;
            g = (uint8_t)((g * br + 127) / 255); 
            r = (uint8_t)((r * br + 127) / 255);
            b = (uint8_t)((b * br + 127) / 255);
        }

        uint8_t bytes[3] = { g, r, b }; //order GRB - put each bytes where they belong

        
        for (int bi = 0; bi < 3; bi++) //Go through each color channel bytes
        {
            uint8_t v = bytes[bi]; //Actual byte we are encoding (bi = 0 - v = g, 1 - r, 2 - b)
            //e.g. g = 1010 0101
            for (int bit = 0; bit < 8; bit += 2) //Run 4 loops 
            {
                uint8_t b1 = (v & 0b10000000) ? CODE1 : CODE0; //Check the top bit. If 1: use CODE1, otherwise use CODE0
                v <<= 1; //Shift left so the next bit becomes the new MSB

                uint8_t b2 = (v & 0b10000000) ? CODE1 : CODE0; //Check the top bit again...
                v <<= 1; //Shift again

                spiBuf[out++] = (uint8_t)((b1 << 4) | (b2 & 0x0F)); //Pack the nibbles into a single byte
            }
            /*
                bit = 0:
                v = 1010 0101
                b1 = CODE1 = 1110
                v <<= 1 ---> v = 0100 1010
                b1 = CODE0 = 1000
                v << 1 ---> 1001 0100
                then shift b1 up by 4 and 1111 b2 ---> 1110 1000, put this in the first item of the spiBuf()
            */
            
        }
    }

    return (uint16_t)out; //return the number of bytes filled in spiBuf[] - it will be used later when sending out SPI MOSI
}

void ws2812b_init(ws2812b_t *strip, ws2812b_color_t *pixel_buf, uint16_t count)
{
    InitializeSPI1(); //Initialize SPI

    strip->count = count; //Access the 'count' member of the 'strip' struct through a pointer and pass the 'count' parameter to it
    strip->pixels = pixel_buf; //Access the 'pixels' member of the 'strip' struct through a pointer and pass the 'pixel_buf' parameter to it
    strip->brightness = 64; //Access the 'brightness' member of the 'strip' struct through a pointer and set the value to 64. (initial value)
    ws2812b_clear(strip);
}

void ws2812b_show(const ws2812b_t *strip)
{
    if (strip->count == 0) return; //Return if there's no LED count
    if (strip->count > WS2812_MAX_LEDS) return; //Return if there's invalid LED count

    uint16_t n = encode_grb_to_spi(strip); //Fill in spiBuf[] and return the number of bytes filled into it

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) {} //Wait for SPI to become available

    SPI_TransferBytes(ws_reset_buf, WS_RESET_BYTES); //Send reset signal (80 us delay)

    SPI_TransferBytes(spiBuf, n); //Transfer the pixels
   
    SPI_TransferBytes(ws_reset_buf, WS_RESET_BYTES); //Send reset signal (80 us delay)
}


void ws2812b_clear(ws2812b_t *strip)
{
    if (!strip || !strip->pixels) return;

    for (uint16_t i = 0; i < strip->count; i++) //Assign zeroes to each LEDs RGB pixel values (brightness)
    {
        strip->pixels[i].g = 0;
        strip->pixels[i].r = 0;
        strip->pixels[i].b = 0;
    }
}

void ws2812b_set_pixel(ws2812b_t *strip, uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (!strip || !strip->pixels) return;
    if (index >= strip->count) return;

    //Store as GRB (matches WS2812B wire order)
    strip->pixels[index].g = g;
    strip->pixels[index].r = r;
    strip->pixels[index].b = b;
}

ws2812b_color_t ws2812b_get_pixel(const ws2812b_t *strip, uint16_t index)
{
    if (!strip || !strip->pixels) return (ws2812b_color_t){0,0,0};
    if (index >= strip->count)    return (ws2812b_color_t){0,0,0};

    return strip->pixels[index]; //Return the color struct of the i-th LED
}

void ws2812b_set_brightness(ws2812b_t *strip, uint8_t brightness)
{
    if (!strip) return;
    strip->brightness = brightness; //Pass the global brightness to the strip 
}

void ws2812b_reset()
{
    SPI_TransferBytes(ws_reset_buf, WS_RESET_BYTES); //Send the 80 us long MOSI signal
}

