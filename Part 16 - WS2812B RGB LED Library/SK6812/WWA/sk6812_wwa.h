#ifndef SK6812_WWA_H
#define SK6812_WWA_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SK6812_WWA_MAX_LEDS
#define SK6812_WWA_MAX_LEDS 256 //Maximum number of LEDs (RAM can also limit it!)
#endif
#if SK6812_WWA_MAX_LEDS < 1 || SK6812_WWA_MAX_LEDS > 65535
#error "SK6812_WWA_MAX_LEDS must be in 1..65535 and fit device RAM"
#endif


typedef struct 
{
    uint8_t white;
    uint8_t warm_white;
    uint8_t amber;
} sk6812_wwa_color_t; //Color struct

typedef struct
{
    uint16_t count;
    sk6812_wwa_color_t *pixels;
    uint8_t brightness;
} sk6812_wwa_t; //Pixel struct


int sk6812_wwa_init(sk6812_wwa_t *strip, sk6812_wwa_color_t *pixel_buf, uint16_t count);

void sk6812_wwa_clear(sk6812_wwa_t *strip);

void sk6812_wwa_set_pixel(sk6812_wwa_t *strip, uint16_t index, uint8_t white, uint8_t warm_white, uint8_t amber);

sk6812_wwa_color_t sk6812_wwa_get_pixel(const sk6812_wwa_t *strip, uint16_t index);

void sk6812_wwa_set_brightness(sk6812_wwa_t *strip, uint8_t brightness);

void sk6812_wwa_show(const sk6812_wwa_t *strip);

void sk6812_wwa_reset(void);

static inline sk6812_wwa_color_t sk6812_wwa_color(uint8_t white, uint8_t warm_white, uint8_t amber)
{
    sk6812_wwa_color_t c = {white, warm_white, amber};
    return c;
}

#ifdef __cplusplus
}
#endif
#endif
