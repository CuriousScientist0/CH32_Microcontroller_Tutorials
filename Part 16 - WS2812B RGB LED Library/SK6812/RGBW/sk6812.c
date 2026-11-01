#include "ch32v00x.h"
#include "sk6812.h"


#define SK_CODE0           0x08 //1000 -___
#define SK_CODE1           0x0C //1100 --__
#define SK_BYTES_PER_LED   16 //16 byte = 128 bit
//Code 0 and 1 are 4-4 SPI bits. 1 colour needs 8 bits, so 32 SPI bits/color
//4x32 = 128 -> 128 / 8 = 16

#define SK_RESET_BYTES     48  //1 bit = 333 ns (1/3 MHz). 1 byte = 2.67 us. 48*2.67 = 128 us

static uint8_t spi_buf[SK6812_MAX_LEDS * SK_BYTES_PER_LED];

static const uint8_t reset_buf[SK_RESET_BYTES] = {0};

static int valid_strip(const sk6812_t *strip)
{
    return strip != NULL &&
           strip->pixels != NULL &&
           strip->count > 0u &&
           strip->count <= SK6812_MAX_LEDS;
}

static void initialize_spi1(void)
{
    GPIO_InitTypeDef gpio = {0};
    SPI_InitTypeDef spi = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC | RCC_PB2Periph_SPI1, ENABLE);

    SPI_I2S_DeInit(SPI1);

    GPIO_ResetBits(GPIOC, GPIO_Pin_6 | GPIO_Pin_5); //Only PC6, MOSI, is relevant. SCK (PC5) can be omitted.

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_5;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &gpio);

    spi.SPI_Direction = SPI_Direction_1Line_Tx;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; //48 MHz / 16 = 3 MHz (333.3 ns)
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &spi);

    SPI_Cmd(SPI1, ENABLE);
}

static void spi_transfer_bytes(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i)
    {
        while ((SPI1->STATR & SPI_I2S_FLAG_TXE) == 0) //Wait until ready
        {
        }

        SPI1->DATAR = data[i]; //Drive out data
    }

    while ((SPI1->STATR & SPI_I2S_FLAG_TXE) == 0)
    {
    }

    while ((SPI1->STATR & SPI_I2S_FLAG_BSY) != 0)
    {
    }
}

static uint8_t scale_channel(uint8_t value, uint8_t brightness)
{
    return (uint8_t)(((uint16_t)value * brightness + 127u) / 255u);
}

static uint16_t encode_pixels(const sk6812_t *strip)
{
    uint16_t out = 0;

    for (uint16_t i = 0; i < strip->count; ++i)
    {
        const sk6812_color_t *pixel = &strip->pixels[i];

        uint8_t r = scale_channel(pixel->r, strip->brightness);
        uint8_t g = scale_channel(pixel->g, strip->brightness);
        uint8_t b = scale_channel(pixel->b, strip->brightness);
        uint8_t w = scale_channel(pixel->w, strip->brightness);

#if SK6812_WIRE_GRBW
        uint8_t channels[4] = {g, r, b, w};
#else
        uint8_t channels[4] = {r, g, b, w};
#endif

        for (uint8_t channel = 0; channel < 4; ++channel)
        {
            uint8_t value = channels[channel];

            for (uint8_t pair = 0; pair < 4; ++pair)
            {
                uint8_t first = (value & 0b10000000) ? SK_CODE1 : SK_CODE0;
                value = (uint8_t)(value << 1);

                uint8_t second = (value & 0b10000000) ? SK_CODE1 : SK_CODE0;
                value = (uint8_t)(value << 1);

                spi_buf[out++] =
                    (uint8_t)((first << 4) | second);
            }
        }
    }

    return out;
}

int sk6812_init(sk6812_t *strip, sk6812_color_t *pixel_buf, uint16_t count)
{
    if (strip == NULL)
        return 0;

    strip->count = 0;
    strip->pixels = NULL;
    strip->brightness = 64;

    if (pixel_buf == NULL || count == 0 || count > SK6812_MAX_LEDS)
    {
        return 0;
    }

    strip->count = count;
    strip->pixels = pixel_buf;

    initialize_spi1();
    sk6812_clear(strip);
    sk6812_reset();

    return 1;
}

void sk6812_show(const sk6812_t *strip)
{
    if (!valid_strip(strip))
    {
        return;
    }

    uint16_t length = encode_pixels(strip);

    while ((SPI1->STATR & SPI_I2S_FLAG_BSY) != 0u)
    {
    }

    spi_transfer_bytes(reset_buf, SK_RESET_BYTES);
    spi_transfer_bytes(spi_buf, length);
    spi_transfer_bytes(reset_buf, SK_RESET_BYTES);
}

void sk6812_clear(sk6812_t *strip)
{
    if (!valid_strip(strip))
    {
        return;
    }

    for (uint16_t i = 0; i < strip->count; ++i)
    {
        strip->pixels[i] = sk6812_rgbw(0, 0, 0, 0);
    }
}

void sk6812_set_pixel(sk6812_t *strip, uint16_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    if (!valid_strip(strip) || index >= strip->count)
    {
        return;
    }

    strip->pixels[index] = sk6812_rgbw(r, g, b, w);
}

sk6812_color_t sk6812_get_pixel(const sk6812_t *strip, uint16_t index)
{
    if (!valid_strip(strip) || index >= strip->count)
    {
        return sk6812_rgbw(0, 0, 0, 0);
    }

    return strip->pixels[index];
}

void sk6812_set_brightness(sk6812_t *strip, uint8_t brightness)
{
    if (strip != NULL)
    {
        strip->brightness = brightness;
    }
}

void sk6812_reset(void)
{
    spi_transfer_bytes(reset_buf, SK_RESET_BYTES);
}
