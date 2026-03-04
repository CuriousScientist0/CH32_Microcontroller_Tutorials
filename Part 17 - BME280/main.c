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
 *CH32V003J4M6 - BME280 lightweight driver with OLED display
 *Article: https://curiousscientist.tech/blog/ch32v003j4m6-bme280-weather-station
 */

#include "debug.h"
#include "i2c.h"
#include "oled_i2c.h"
#include "bme280.h"

/* Global typedef */

/* Global define */

/* Global Variable */

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */

static void print_values_on_oled(int32_t t_x100, uint32_t p_pa, uint32_t h_x1000)
{
    char line[24];

    int32_t t_int  = t_x100 / 100;
    int32_t t_frac = (t_x100 < 0) ? (-(t_x100 % 100) / 10) : ((t_x100 % 100) / 10);

    uint32_t p_hpa_x10 = (p_pa + 5) / 10; // Pa/10 = hPa*10
    uint32_t p_int = p_hpa_x10 / 10;
    uint32_t p_frac = p_hpa_x10 % 10;

    uint32_t h_int  = h_x1000 / 1000;
    uint32_t h_frac = (h_x1000 % 1000) / 100;

    int32_t alt_m = altitude_m_approx(p_pa);
  
    snprintf(line, sizeof(line), "T: %ld.%01ld C", (long)t_int, (long)t_frac);
    printText042((uint8_t*)line, 0, 0);

    snprintf(line, sizeof(line), "P: %lu.%01lu hPa", (unsigned long)p_int, (unsigned long)p_frac);
    printText042((uint8_t*)line, 1, 0);

    snprintf(line, sizeof(line), "H: %lu.%01lu %%", (unsigned long)h_int, (unsigned long)h_frac);
    printText042((uint8_t*)line, 2, 0);

    snprintf(line, sizeof(line), "A: %ld m", (long)alt_m);
    printText042((uint8_t*)line, 3, 0);
}
    

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    IIC_Init();
    initializeOLED042();    

    eraseDisplay();
    Delay_Ms(500);

    printText042("CH32V003J4M6", 0, 0); //Print some welcome text
    printText042(" --BME280--", 1, 0);
    printText042("  CS 2026 ", 2, 0);
    Delay_Ms(2000);
    
    eraseDisplay();

    bme280_t bme; //Create a bme object

    printText042("BME init...", 0, 0); //Text will stay on the OLED if the functions below fail
    Delay_Ms(200);

    if (!bme280_init(&bme, 0x76)) //I2C address for BME280 is 0x77 or 0x76
    {
        if (!bme280_init(&bme, 0x77)) 
        { 
          while(1);
        }
    }    

    bme280_configure(&bme, 1, 1, 1, 0, 0, 0); //forced mode   

    while(1)
    {
        int32_t t;
        uint32_t p, h;
       
        I2C_WriteRegister(bme.addr, 0xF4, (1 << 5) | (1 << 2) | 1); //forced mode

        Delay_Ms(10); //enough for osrs=1

        bme280_read(&bme, &t, &p, &h); //Get the parameters
        print_values_on_oled(t, p, h); //Print the parameters on the oled display        

        Delay_Ms(500);
    }
}
