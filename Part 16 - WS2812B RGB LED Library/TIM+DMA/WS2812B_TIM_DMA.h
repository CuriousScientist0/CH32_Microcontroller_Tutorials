//WS2812B_TIM_DMA.h
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//Article: https://curiousscientist.tech/blog/ch32v003j4m6-mini-board-ws2812b-rgb-led-tim-pwm-dma

//WS2812B expects color order GRB on the wire
typedef struct 
{
    uint8_t g; //Green
    uint8_t r; //Red
    uint8_t b; //Blue
} ws2812b_color_t;

typedef struct 
{
    uint16_t count;            //number of LEDs
    ws2812b_color_t *pixels;   //pixel buffer: count entries (GRB fields)
    uint8_t brightness;        //Brightness
} ws2812b_t;

//Initialize a WS2812B instance and its pixel buffer.
void ws2812b_init(ws2812b_t *strip, ws2812b_color_t *pixel_buf, uint16_t count);

//Clear all pixels to 0 (off). Does not transmit.
void ws2812b_clear(ws2812b_t *strip);

//Set one pixel (0..count-1). Does not transmit.
void ws2812b_set_pixel(ws2812b_t *strip, uint16_t index, uint8_t r, uint8_t g, uint8_t b);

//Get one pixel.
ws2812b_color_t ws2812b_get_pixel(const ws2812b_t *strip, uint16_t index);

//Set global brightness (0..255). 255 = full.
void ws2812b_set_brightness(ws2812b_t *strip, uint8_t brightness);

//Get global brightness (0..255). 255 = full.
uint8_t ws2812b_get_brightness(const ws2812b_t *strip);

//Transmit the whole frame to the LED chain (blocking).
void ws2812b_show(const ws2812b_t *strip);

//Convenience: pack RGB into a color object.
static inline ws2812b_color_t ws2812b_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    ws2812b_color_t c = { .g = g, .r = r, .b = b };
    return c;
}
//Reset the LED
void ws2812b_reset();

#ifdef __cplusplus
}
#endif
