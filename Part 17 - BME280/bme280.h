#pragma once
#include <stdint.h>

typedef struct 
{
    uint8_t addr;   //0x76 or 0x77
    int32_t t_fine; //fine resolution temperature value
    //T1-T3, P1-P9, H1-H6 ---> Trimming (calibration) parameters stored in the chip's non-volatile memory
    uint16_t dig_T1; int16_t dig_T2, dig_T3; //short
    uint16_t dig_P1; int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9; //short
    uint8_t  dig_H1, dig_H3; //char
    int16_t  dig_H2, dig_H4, dig_H5; //short
    int8_t   dig_H6; //char
} bme280_t; //Define a type, called bme280_t - Create a variable in the memory that stores the above data

uint8_t bme280_init(bme280_t *bme, uint8_t addr);

uint8_t bme280_configure(bme280_t *bme,
                      uint8_t osrs_t, uint8_t osrs_p, uint8_t osrs_h,
                      uint8_t mode,
                      uint8_t filter,
                      uint8_t standby);

uint8_t bme280_read(bme280_t *bme, int32_t *temp_c_x100, uint32_t *press_pa, uint32_t *hum_x1000);

int32_t altitude_m_approx(uint32_t p_pa);
