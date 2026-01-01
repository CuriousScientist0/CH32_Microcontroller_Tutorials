/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/25
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *CH32V003F4P6 - SPI-based WS2812B RGB LED (NeoPixel) driver
 *Article: www.curiousscientist.tech/blog/ch32v003f4p6-ws2812b-rgb-neopixel-driver
 */

#include "debug.h"
#include "ws2812b.h"


/* Global define */
#define LED_COUNT 24 //Number of LEDs to be controlled

/* Global Variable */

static ws2812b_color_t leds[LED_COUNT]; //Create an array of 24 elements where each element holds G, R, B values for one LED

static inline void GPIO_USART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //GPIO_Pin_5 is a GPIO pin
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz; //30 MHz max toggling speed
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //Pin has an alternate function (AF), in this case the UART
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; //GPIO_Pin_6 is a GPIO pin
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Floating = High-impedance mode. Since this is RX, its state will be determined by the programmer's TX pin
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    //----------------------------------------------------------------------------------------------------------
    //Config of the USART, no further explanation is needed really
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}


/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    GPIO_USART_Init(); //Can be skipped, it is only for debugging

    ws2812b_t strip; //Create an object of the ws2812b_t called strip

    ws2812b_init(&strip, leds, LED_COUNT); //Initialize the strip

    ws2812b_reset(&strip); //Reset the strip to clear off glitching frames
    ws2812b_clear(&strip); //Clear the strip (in the code)
    ws2812b_show(&strip); //APPLY the clearing (send the settings to the LED strip)
    ws2812b_set_brightness(&strip, 50); //Set global brightness to a quarter level
    ws2812b_clear(&strip); //Clear the strip
    ws2812b_show(&strip); //APPLY the brightness level

    Delay_Ms(2000);

    for(int i = 0; i < LED_COUNT-2; i++)
    {
        ws2812b_set_pixel(&strip, i, 255, 0, 0);   //LED0 red
        ws2812b_set_pixel(&strip, i+1, 0, 255, 0);   //LED1 green
        ws2812b_set_pixel(&strip, i+2, 0, 0, 255);   //LED2 blue
        ws2812b_show(&strip); //APPLY the pixels
        Delay_Ms(200);
        ws2812b_clear(&strip); //Clear the strip
        ws2812b_show(&strip); //APPLY the clearing
    } //Show RGB and then shift it until B reaches the top


    Delay_Ms(2000);

    uint8_t pos = 0; //Position of the lit LED
    int8_t dir = +1; //Direction of sweeping

    printf("CH32V003F4P6 - WS2812B \"NeoPixel\" Tutorial - Part 16\n");

    uint8_t red = 255; //Color brightness 
    uint8_t green = 0;
    uint8_t blue = 0; 
    uint8_t bounce = 0; //Bounce counter for when the LED reaches the end of the strip

while (1)
{
    ws2812b_clear(&strip); //Clear the previously shown RGB LEDs
    
    ws2812b_set_pixel(&strip, pos, red, green, blue); //Draw the first red LED

    //trailing pixel (one behind in direction of travel)
    int16_t tail = (int16_t)pos - dir;          //tail is behind by 1 unit
    int16_t tail2 = (int16_t)pos - (dir*2);     //tail2 is behind by 2 units
    if (tail >= 0 && tail < LED_COUNT) 
    {
        ws2812b_set_pixel(&strip, (uint8_t)tail, (uint8_t)(red/2), (uint8_t)(green/2), (uint8_t)(blue/2)); //Same color but with a lower intensity
        ws2812b_set_pixel(&strip, (uint8_t)tail2, (uint8_t)(red/4), (uint8_t)(green/4), (uint8_t)(blue/4)); //Same color but with a much lower intensity
    }

    ws2812b_show(&strip);

    if (pos == (LED_COUNT - 1)) //Flip the direction at the top and the bottom
    {
        dir = -1;
        bounce++;

    }
    else if (pos == 0)
    {
        dir = +1;
        bounce++;
    }    

    //A little fun: whenever the strip reaches the end and bounces back, we change color
    if(bounce % 2 == 0) //bounce became 2
    {
        red = 0;
        green = 255;
        blue = 0;
    }
    else if (bounce %3 ==0) //bounce became 3
    {
        red = 0;
        green = 0;
        blue = 255;

    }
    else //bounce is 1 
    {
        red = 255;
        green = 0;
        blue = 0;        
    }

    if(bounce == 4)
    {
        bounce = 1; //wrap
    }

    pos = (uint8_t)(pos + dir); //Recalculate the position for the next iteration

    Delay_Ms(110); //wait 110 ms
}

}
