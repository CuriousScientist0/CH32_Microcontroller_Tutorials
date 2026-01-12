There are two approaches implemented to drive WS2812B RGB LEDs:
- SPI approach
- Timer + DMA approach

The SPI approach is faster and more resourceful. It requires less memory, so it allows the microcontroller to drive more LEDs. But, it (obviously) requires an SPI peripheral.

For example, the CH32V003J4M6 chip does not have an SPI peripheral, so the only way to drive these LEDs with it is the timer + DMA approach.
It is more resource-demanding, so it cannot drive as many LEDs as the SPI approach. However, it might still be a good choice for lightweight projects, for just a handful of LEDs.

SPI article: https://curiousscientist.tech/blog/ch32v003f4p6-ws2812b-rgb-neopixel-driver

TIM+DMA article: https://curiousscientist.tech/blog/ch32v003j4m6-mini-board-ws2812b-rgb-led-tim-pwm-dma
