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
 *CH32V006K8U6 - SPI-based SK6812 RGBW LED  driver
 *Article: https://www.curiousscientist.tech/blog/sk6812-rgbw-nw-a-potential-neopixel-dethroner
*/

#include "debug.h"
#include "sk6812_wwa.h"

#define LED_COUNT 1

#if LED_COUNT < 1 || LED_COUNT > SK6812_WWA_MAX_LEDS
#error "LED_COUNT must be in 1..SK6812_WWA_MAX_LEDS"
#endif

static sk6812_wwa_t strip;
static sk6812_wwa_color_t leds[LED_COUNT];

static void show_color(sk6812_wwa_color_t color)
{
    uint16_t i;
    for (i = 0; i < strip.count; ++i)
    {
        sk6812_wwa_set_pixel(&strip, i, color.white, color.warm_white, color.amber);
    }
    sk6812_wwa_show(&strip);
}

void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD | RCC_PB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

static uint8_t apply_color_command(const char *text)
{
    uint8_t channels[3];

    if (text == NULL)
    {
        return 0;
    }

    for (uint8_t channel = 0; channel < 3; ++channel)
    {
        uint16_t value = 0;

        while (*text == ' ' || *text == '\t')
        {
            ++text;
        }

        if (*text < '0' || *text > '9')
        {
            return 0;
        }

        while (*text >= '0' && *text <= '9')
        {
            value = (uint16_t)(value * 10 + (uint16_t)(*text - '0'));

            if (value > 255)
            {
                return 0;
            }

            ++text;
        }

        channels[channel] = (uint8_t)value;

        while (*text == ' ' || *text == '\t')
        {
            ++text;
        }

        if (channel < 2)
        {
            if (*text != ',')
            {
                return 0;
            }

            ++text;
        }
        else
        {
            if (*text != '\0')
            {
                return 0;
            }
        }
    }

    show_color(sk6812_wwa_color(channels[0], channels[1], channels[2]));

    return 1;
}

static void color_usart_receive_byte(uint8_t byte)
{
    static char line[48];
    static uint8_t length = 0;
    static uint8_t discard = 0;

    if (byte == '\r' || byte == '\n')
    {
        if (!discard && length > 0)
        {
            line[length] = '\0';
            (void)apply_color_command(line);
        }

        length = 0;
        discard = 0;
        return;
    }

    if (discard)
    {
        return;
    }

    if ((byte < 32 && byte != '\t') || byte > 126)
    {
        discard = 1;
        return;
    }

    if (length >= sizeof(line) - 1)
    {
        discard = 1;
        return;
    }

    line[length++] = (char)byte;
}



int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate(); //Note: Don't forget to set the clock to 48 MHz!
    Delay_Init();
    USARTx_CFG();
    printf("CH32V006K8U6 - SK6812 WWA Demo\n"); //Print some text on the serial port
    
    if (SystemCoreClock != 48000000u || !sk6812_wwa_init(&strip, leds, LED_COUNT))
    {
        while (1) 
        {            
        }
    }
    
    show_color(sk6812_wwa_color(0, 0, 0));
    sk6812_wwa_set_brightness(&strip, 255);

    while (1)
    {
        
        if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
        {
            uint8_t byte = (uint8_t)USART_ReceiveData(USART1);
            color_usart_receive_byte(byte);
        }
        //Note: Simply send the required white brightness values to the MCU via serial.
        //Example: 0,255,0 lights up the warm white LED at full brightness         
    }
}
