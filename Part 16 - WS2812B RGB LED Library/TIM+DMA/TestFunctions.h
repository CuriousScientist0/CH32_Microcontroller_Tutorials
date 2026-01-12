#pragma once
#include <stdint.h>
#include "WS2812B_TIM_DMA.h"   // for ws2812b_t / ws2812b_color_t

#ifdef __cplusplus
extern "C" {
#endif

//Article: https://curiousscientist.tech/blog/ch32v003j4m6-mini-board-ws2812b-rgb-led-tim-pwm-dma

void Initialize_Tests(ws2812b_t *strip, uint16_t led_count);

void test_single_led_set(uint16_t pos, uint8_t r, uint8_t g, uint8_t b);
void test_walk_one_bit(uint16_t delay_ms);
void test_all_on_off_hammer(uint16_t delay_ms);
void test_alternating_patterns(uint16_t delay_ms);
void test_fast_pong(uint8_t r, uint8_t g, uint8_t b, uint16_t delay_ms);
void test_unique_per_led(uint16_t delay_ms);
void test_static_spectrum(void);
void test_rotating_spectrum(uint16_t rotations, uint16_t delay_ms);
void test_spectrum_brightness_sweep(void);
void test_spectrum_buildup(uint16_t step_ms, uint16_t hold_ms);
void test_last_bit_toggle(void);
void test_dual_bounce_demo(uint16_t steps, uint16_t delay_ms, uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2);
void test_rotating_red_brightness_gradient(uint16_t rotations, uint16_t frame_delay_ms, uint8_t min_brightness);

#ifdef __cplusplus
}
#endif
