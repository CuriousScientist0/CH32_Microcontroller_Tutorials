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
    WS2812B with CH32V003J4M6 using TIM1 PWM + DMA

    Article: https://curiousscientist.tech/blog/ch32v003j4m6-mini-board-ws2812b-rgb-led-tim-pwm-dma

    PINOUT
    1 - PA1 / PD6
    2 - VSS (GND)
    3 - PA2
    4 - VDD (5 or 3.3V)
    5 - PC1 (SDA - i2c)
    6 - PC2 (SCL - i2c)
    7 - PC4
    8 - PD1 - SWIO --- DONT TOUCH ---    
    -----> Set clock source to 48 MHz HSI in system_ch32v00x.c 
 */

#include "debug.h"
#include "WS2812B_TIM_DMA.h"
#include "TestFunctions.h"

/* Global typedef */

/* Global define */
#define LED_COUNT 24 //Number of LEDs to be controlled (Maximum is 34 on J4M6 chip)

/* Global Variable */
static ws2812b_t strip; //Create an object of the ws2812b_t called strip
static ws2812b_color_t leds[LED_COUNT]; //Array of 24 elements where each item holds G, R and B values for one LED


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

    ws2812b_init(&strip, leds, LED_COUNT);  //Initialize the strip    
    ws2812b_set_brightness(&strip, 255);  //Set global brightness to half brightness
    ws2812b_show(&strip); //Apply the settings
    ws2812b_clear(&strip); //Clear the LEDs (nothing is ON)
    ws2812b_show(&strip); //Apply the settings
    Delay_Ms(5000);
    
    Initialize_Tests(&strip, LED_COUNT); //Initialize the test series
   
    //-----
    test_rotating_red_brightness_gradient(20, 3, 1); //Spinning brightness gradient
    
    test_dual_bounce_demo(240, 20, 128, 0, 0, 0, 0, 128); //Mimic dual bounce of 2 colours (red and blue)

    test_last_bit_toggle(); //Toggle the last bit (B0) to test for spill-over (if it becomes G7) 
    //The user should see a blinking dim blue LED. If the blinking LED is bright green, there's something wrong with the pulses.

    test_alternating_patterns(100); //Alternate the blue color's bits between 10101010 and 01010101 ten times with 100 us delay

    test_walk_one_bit(5); //Go through the strip for G, R and B and gradually decrease their brightness

    test_all_on_off_hammer(10000); //Show ALL LEDs in the whole strip for 1 second. Be careful about the global brightness and current draw!

    test_fast_pong(128,0,128,10); //Bounce a color between the two ends of the strip. 

    test_unique_per_led(1000); //Assign custom color to each position in the strip. The colors are random-ish, but close to white
    
    test_static_spectrum(); //Assign custom color to each position in the strip by distributing the visible spectrum's colors to each LED
    
    test_spectrum_brightness_sweep(); //Do a brightness sweep on the full-spectrum rainbow   

    test_rotating_spectrum(10,10); //Do 10 rotations of the full spectrum

    test_spectrum_buildup(10,1000); //Build up the full-spectrum rainbow by stacking the LEDs in the strip

    //Clear the strip, finish the demo
    ws2812b_clear(&strip);
    ws2812b_show(&strip);
    
    while (1)
    {      
            //Not implemented
    }    
}

