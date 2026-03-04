#include "bme280.h"
#include "i2c.h"
#include "debug.h"  

//Registers - Datasheet chapter 5.4
#define REG_ID        0xD0 //Chip ID
#define REG_RESET     0xE0 //Soft reset work
#define REG_CTRL_HUM  0xF2 //Humidity acquisition option
#define REG_STATUS    0xF3 //Status (measuring or im_update)
#define REG_CTRL_MEAS 0xF4 //P and T data acquisition option
#define REG_CONFIG    0xF5 //Rate, filter and interface options
#define REG_DATA      0xF7 //Raw measurement output data
#define CHIP_ID       0x60 //Chip ID from 0xD0
#define RESET_CMD     0xB6 //Reset code for 0xE0

static uint8_t wr8(bme280_t *b, uint8_t reg, uint8_t v) 
{
    return I2C_WriteRegisters(b->addr, reg, &v, 1);
}

static uint8_t rd8(bme280_t *b, uint8_t reg, uint8_t *v) 
{
    return I2C_ReadRegisters(b->addr, reg, v, 1);
}

static uint8_t rdn(bme280_t *b, uint8_t reg, uint8_t *buf, uint8_t n) 
{
    return I2C_ReadRegisters(b->addr, reg, buf, n);
}

static uint16_t u16le(const uint8_t *p)
{
     return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int16_t s16le(const uint8_t *p)
{
     return (int16_t)u16le(p); 
}

static uint8_t read_cal(bme280_t *b)
{
    uint8_t c1[26];
    uint8_t c2[7];

    if (!rdn(b, 0x88, c1, 26)) return FAIL;
    if (!rdn(b, 0xE1, c2, 7))  return FAIL;

    b->dig_T1 = u16le(&c1[0]);
    b->dig_T2 = s16le(&c1[2]);
    b->dig_T3 = s16le(&c1[4]);

    b->dig_P1 = u16le(&c1[6]);
    b->dig_P2 = s16le(&c1[8]);
    b->dig_P3 = s16le(&c1[10]);
    b->dig_P4 = s16le(&c1[12]);
    b->dig_P5 = s16le(&c1[14]);
    b->dig_P6 = s16le(&c1[16]);
    b->dig_P7 = s16le(&c1[18]);
    b->dig_P8 = s16le(&c1[20]);
    b->dig_P9 = s16le(&c1[22]);

    b->dig_H1 = c1[25];
    b->dig_H2 = s16le(&c2[0]);
    b->dig_H3 = c2[2];
    b->dig_H4 = (int16_t)((((int16_t)c2[3]) << 4) | (c2[4] & 0x0F)); //Pay attention to H4 and H5 as their registers are a bit special
    b->dig_H5 = (int16_t)((((int16_t)c2[5]) << 4) | (c2[4] >> 4));
    b->dig_H6 = (int8_t)c2[6];

    return OK;
}

uint8_t bme280_init(bme280_t *bme, uint8_t addr)
{
    if (!bme) return FAIL;
    bme->addr = addr;
    bme->t_fine = 0;

    uint8_t id = 0;
    if (!rd8(bme, REG_ID, &id)) return FAIL; //check ID
    if (id != CHIP_ID) return FAIL;
    
    if (!wr8(bme, REG_RESET, RESET_CMD)) return FAIL; //reset
    Delay_Ms(5);
    
    for (uint8_t i = 0; i < 50; i++) //wait NVM copy
    {
        uint8_t st;
        if (!rd8(bme, REG_STATUS, &st)) return FAIL;
        if ((st & 0x01) == 0) break;
        Delay_Ms(2);
    }

    if (!read_cal(bme)) return FAIL;
    
    return bme280_configure(bme, 1, 1, 1, 3, 0, 5); //default config: normal mode, small oversampling
}

uint8_t bme280_configure(bme280_t *bme,
                      uint8_t osrs_t, uint8_t osrs_p, uint8_t osrs_h,
                      uint8_t mode,
                      uint8_t filter,
                      uint8_t standby)
{
    osrs_t &= 7; osrs_p &= 7; osrs_h &= 7;
    mode &= 3; filter &= 7; standby &= 7;

    //write humidity first (datasheet requirement)
    if (!wr8(bme, REG_CTRL_HUM, osrs_h)) return FAIL;

    uint8_t cfg = (uint8_t)((standby << 5) | (filter << 2));
    if (!wr8(bme, REG_CONFIG, cfg)) return FAIL;

    uint8_t meas = (uint8_t)((osrs_t << 5) | (osrs_p << 2) | mode);
    if (!wr8(bme, REG_CTRL_MEAS, meas)) return FAIL;

    return OK;
}

/********** Compensation (integer, float-free) **********/
//Datasheet 4.2.3. 

static int32_t comp_T(bme280_t *b, int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)b->dig_T1 << 1))) * ((int32_t)b->dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)b->dig_T1)) * ((adc_T >> 4) - ((int32_t)b->dig_T1))) >> 12) *
                    ((int32_t)b->dig_T3)) >> 14;
    b->t_fine = var1 + var2; //Access the t_fine member of the bme280_t struct where the *b points to.
    return (b->t_fine * 5 + 128) >> 8; //°C * 100
}

static uint32_t comp_P(bme280_t *b, int32_t adc_P)
{
    int64_t var1 = ((int64_t)b->t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)b->dig_P6;
    var2 = var2 + ((var1 * (int64_t)b->dig_P5) << 17);
    var2 = var2 + (((int64_t)b->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)b->dig_P3) >> 8) + ((var1 * (int64_t)b->dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * (int64_t)b->dig_P1) >> 33;

    if (var1 == 0) return 0;

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)b->dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)b->dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)b->dig_P7) << 4);

    return (uint32_t)(p >> 8); //Pa
}

static uint32_t comp_H(bme280_t *b, int32_t adc_H)
{
    int32_t v = (b->t_fine - ((int32_t)76800));
    v = (((((adc_H << 14) - (((int32_t)b->dig_H4) << 20) - (((int32_t)b->dig_H5) * v)) + ((int32_t)16384)) >> 15) *
         (((((((v * ((int32_t)b->dig_H6)) >> 10) * (((v * ((int32_t)b->dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
             ((int32_t)2097152)) * ((int32_t)b->dig_H2) + 8192) >> 14));

    v = (v - (((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)b->dig_H1)) >> 4));
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;

    // v is %RH * 1024 (after >>12)
    uint32_t hum_x1024 = (uint32_t)(v >> 12);
    return (hum_x1024 * 1000u + 512u) / 1024u; //%RH * 1000
}

uint8_t bme280_read(bme280_t *bme, int32_t *t_x100, uint32_t *p_pa, uint32_t *h_x1000)
{
    uint8_t d[8];
    if (!rdn(bme, REG_DATA, d, 8)) return FAIL;

    int32_t adc_P = (int32_t)((((uint32_t)d[0] << 12) | ((uint32_t)d[1] << 4) | (d[2] >> 4))); //Get the raw ADC readings
    int32_t adc_T = (int32_t)((((uint32_t)d[3] << 12) | ((uint32_t)d[4] << 4) | (d[5] >> 4)));
    int32_t adc_H = (int32_t)((((uint32_t)d[6] << 8)  | (uint32_t)d[7]));

    int32_t T = comp_T(bme, adc_T); //Get the actual values by applying the corresponding trimming parameters and formulas
    uint32_t P = comp_P(bme, adc_P);
    uint32_t H = comp_H(bme, adc_H);

    if (t_x100)  *t_x100 = T;
    if (p_pa)    *p_pa = P;
    if (h_x1000) *h_x1000 = H;
    return OK;
}

int32_t altitude_m_approx(uint32_t p_pa) //Very rudimentary conversion, just to print something
{
    const int32_t P0 = 101325; //sea-level standard (Pa)    
    return (int32_t)(P0 - (int32_t)p_pa) / 12; //~12 Pa per meter near sea level
}
