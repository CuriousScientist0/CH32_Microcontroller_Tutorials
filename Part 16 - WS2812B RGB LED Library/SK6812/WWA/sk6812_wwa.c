#include "ch32v00x.h"
#include "sk6812_wwa.h"

#define SK_CODE0           0x08 //1000 -___
#define SK_CODE1           0x0C //1100 --__
#define SK_BYTES_PER_LED   12 //12 byte = 96 bit. 
//Code 0 and 1 are 4-4 SPI bits. 1 colour needs 8 bits, so 32 SPI bits/color
//3x32 = 96 -> 96 / 8 = 12.

#define SK_RESET_BYTES     48 //1 bit = 333 ns (1/3 MHz). 1 byte = 2.67 us. 48*2.67 = 128 us

static uint8_t spi_buf[(size_t)SK6812_WWA_MAX_LEDS * SK_BYTES_PER_LED];

static const uint8_t reset_buf[SK_RESET_BYTES] = {0}; //48x zero bytes to create 128 us long MOSI pulse

static int valid_strip(const sk6812_wwa_t *strip)
{
    return strip != NULL &&
           strip->pixels != NULL &&
           strip->count > 0u &&
           strip->count <= SK6812_WWA_MAX_LEDS;
}

static void initialize_spi1(void)
{
    GPIO_InitTypeDef gpio = {0};
    SPI_InitTypeDef spi = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC | RCC_PB2Periph_SPI1, ENABLE);

    SPI_I2S_DeInit(SPI1);
   
    GPIO_ResetBits(GPIOC, GPIO_Pin_6 | GPIO_Pin_5); //Technically only pin 6, MOSI is used. SCK (5) is not driving anything

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_5;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &gpio);

    spi.SPI_Direction = SPI_Direction_1Line_Tx; //MOSI only
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; //48MHz / 16 = 3 MHz (333.3 ns)
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &spi);

    SPI_Cmd(SPI1, ENABLE);
}

static void spi_transfer_bytes(const uint8_t *data, size_t len)
{    
    for (size_t i = 0; i < len; ++i)
    {
        while ((SPI1->STATR & SPI_I2S_FLAG_TXE) == 0) //Wait until the TX register becomes empty
        {
        }

        SPI1->DATAR = data[i]; //Push out the data
    }

    //Wait for both the transmit buffer and shifter to finish.
    while ((SPI1->STATR & SPI_I2S_FLAG_TXE) == 0)
    {
    }

    while ((SPI1->STATR & SPI_I2S_FLAG_BSY) != 0)
    {
    }
}

static uint8_t scale_channel(uint8_t value, uint8_t brightness)
{
    return (uint8_t)(((uint16_t)value * brightness + 127) / 255);
}

static size_t encode_pixels(const sk6812_wwa_t *strip)
{
    size_t out = 0;

    for (uint16_t i = 0; i < strip->count; ++i) //Go until we have LEDs in the strip
    {
        const sk6812_wwa_color_t *pixel = &strip->pixels[i];

        uint8_t channels[3] = 
        {
            scale_channel(pixel->white, strip->brightness),
            scale_channel(pixel->amber, strip->brightness),
            scale_channel(pixel->warm_white, strip->brightness)
        };

        for (uint8_t channel = 0; channel < 3; ++channel) //Go through each color channel bytes
        {
            uint8_t value = channels[channel]; //Actual encoded byte
           
            for (uint8_t pair = 0; pair < 4; ++pair) //Run 4 loops
            {
                uint8_t first = (value & 0b10000000) ? SK_CODE1 : SK_CODE0; //Check the top bit, if 1 use CODE 1, otherwise CODE 0
                value = (uint8_t)(value << 1); //Shift left so the next bit becomes the new MSB

                uint8_t second = (value & 0b10000000) ? SK_CODE1 : SK_CODE0; //Check the top bit again...
                value = (uint8_t)(value << 1);

                spi_buf[out++] = (uint8_t)((first << 4) | second); //Pack the nibbles into a single byte
            }
        }
    }

    return out;
}

int sk6812_wwa_init(sk6812_wwa_t *strip, sk6812_wwa_color_t *pixel_buf, uint16_t count)
{
    if (strip == NULL)
    {
        return 0; //Error handling
    }

    //Fill in default values
    strip->count = 0;
    strip->pixels = NULL; 
    strip->brightness = 64; //Assign 64 to the brightness member of the strip struct through a pointer. (initial value)

    if (pixel_buf == NULL || count == 0 || count > SK6812_WWA_MAX_LEDS)
    {
        return 0;
    }

    strip->count = count; //Access the 'count' member of the 'strip' struct through a pointer and pass the 'count' parameter to it
    strip->pixels = pixel_buf; //Access the 'pixels' member of the 'strip' struct through a pointer and pass the 'pixel_buf' parameter to it

    initialize_spi1();
    sk6812_wwa_clear(strip);
    sk6812_wwa_reset();

    return 1;
}

void sk6812_wwa_show(const sk6812_wwa_t *strip)
{
    if (!valid_strip(strip)) //Error handling
    {
        return;
    }

    size_t length = encode_pixels(strip); //Get the length

    while ((SPI1->STATR & SPI_I2S_FLAG_BSY) != 0) //Wait for the SPI to become available
    {
    }

    spi_transfer_bytes(reset_buf, SK_RESET_BYTES); //Send reset signal
    spi_transfer_bytes(spi_buf, length); //Transfer pixels
    spi_transfer_bytes(reset_buf, SK_RESET_BYTES); //Send reset signal
}

void sk6812_wwa_clear(sk6812_wwa_t *strip)
{
    if (!valid_strip(strip)) //Error handling
    {
        return;
    }

    for (uint16_t i = 0; i < strip->count; ++i) //Assign zeroes to each LEDs RGB pixel values (brightness)
    {
        strip->pixels[i] = sk6812_wwa_color(0, 0, 0);
    }
}

void sk6812_wwa_set_pixel(sk6812_wwa_t *strip, uint16_t index, uint8_t white, uint8_t warm_white, uint8_t amber)
{
    if (!valid_strip(strip) || index >= strip->count) //Error handling
    {
        return;
    }

    strip->pixels[index] = sk6812_wwa_color(white, warm_white, amber); //Store pixel values
}

sk6812_wwa_color_t sk6812_wwa_get_pixel(const sk6812_wwa_t *strip, uint16_t index)
{
    if (!valid_strip(strip) || index >= strip->count)
    {
        return sk6812_wwa_color(0, 0, 0);
    }

    return strip->pixels[index]; //Return the color struct of the i-th LED
}

void sk6812_wwa_set_brightness(sk6812_wwa_t *strip, uint8_t brightness)
{
    if (strip != NULL)
    {
        strip->brightness = brightness;//Pass the global brightness to the strip 
    }
}

void sk6812_wwa_reset(void)
{
    spi_transfer_bytes(reset_buf, SK_RESET_BYTES); //Send the reset signal
}
