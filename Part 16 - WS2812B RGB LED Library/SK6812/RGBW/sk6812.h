#ifndef SK6812_H
#define SK6812_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SK6812_MAX_LEDS 256

#define SK6812_WIRE_GRBW 1 //0: RGBW, 1:GRBW order

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
} sk6812_color_t;

typedef struct
{
    uint16_t count;
    sk6812_color_t *pixels;
    uint8_t brightness;
} sk6812_t;

int sk6812_init(sk6812_t *strip, sk6812_color_t *pixel_buf, uint16_t count);

void sk6812_clear(sk6812_t *strip);

void sk6812_set_pixel(sk6812_t *strip, uint16_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w);

sk6812_color_t sk6812_get_pixel(const sk6812_t *strip, uint16_t index);

void sk6812_set_brightness(sk6812_t *strip, uint8_t brightness);

void sk6812_show(const sk6812_t *strip);

void sk6812_reset(void);

static inline sk6812_color_t sk6812_rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    sk6812_color_t c = {r, g, b, w};
    return c;
}

#ifdef __cplusplus
}
#endif

#endif
