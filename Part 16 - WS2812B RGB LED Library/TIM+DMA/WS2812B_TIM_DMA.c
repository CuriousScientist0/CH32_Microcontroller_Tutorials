#include "ch32v00x.h"
#include "debug.h"
#include "WS2812B_TIM_DMA.h"

/*
    TIM+DMA backend for WS2812B on CH32V003(J4M6)
    //Article: https://curiousscientist.tech/blog/ch32v003j4m6-mini-board-ws2812b-rgb-led-tim-pwm-dma
    
    Default mapping
      - Output pin: PC4 (TIM1_CH4)
      - Timer: TIM1 CH4 PWM
      - DMA: DMA1_Channel5 writing TIM1->CH4CVR (CCR4) (RM: Table 8-2 DMA1 peripheral mapping table)
      - 48 MHz system clock assumed (HSI used for testing)

    Good to know: 
    TIM RCC might be different on different MCUs. CH32V006K8U6 uses RCC_PB2Periph_TIM1
    DMA RCC might also be different. CH32V006K8U6 uses RCC_HBPeriph_DMA1
*/

//------------------------User-tunable backend config------------------------
#define WS2812_MAX_LEDS          34

//Timer/pin selection
#define WS_GPIO_PORT             GPIOC
#define WS_GPIO_PIN              GPIO_Pin_4

#define WS_TIM                   TIM1
#define WS_TIM_RCC               RCC_APB2Periph_TIM1
#define WS_GPIO_RCC              RCC_APB2Periph_GPIOC

//CH4 on TIM1
#define WS_TIM_CCR_REG           (WS_TIM->CH4CVR)

//DMA mapping for TIM1_UP on CH32V003
#define WS_DMA_RCC               RCC_AHBPeriph_DMA1
#define WS_DMA_CH                DMA1_Channel5
#define WS_DMA_TC_FLAG           DMA1_FLAG_TC5
#define WS_DMA_GL_FLAG           DMA1_FLAG_GL5

//WS2812 timing encoded as PWM high-time counts (at 48 MHz, tick=20.83ns)
#define WS_CCR_0                 19   //~400ns high
#define WS_CCR_1                 41   //~850ns high

//Bit period: ARR+1 ticks. ARR = 60-1 => 60 ticks => 1.25us
#define WS_TIM_PRESCALER         0
#define WS_TIM_ARR               (60 - 1)

//Reset/latch low time
#define WS_RESET_US              80

//------------------------ Internal state ------------------------
static uint16_t ccrBuf[WS2812_MAX_LEDS * 24 + 2];  // 24 bits per LED, 2 extra slots for the helper pulses
static ws2812b_t *s_strip = 0;

static inline uint16_t bit_to_ccr(uint8_t bit_set) //Convert a bit to code 1 or code 0 CCR pulses
{
    return bit_set ? WS_CCR_1 : WS_CCR_0;
}

static void ws_timer_init(void)
{
    GPIO_InitTypeDef  gpio = {0};
    TIM_OCInitTypeDef oc   = {0};
    TIM_TimeBaseInitTypeDef tb = {0};

    RCC_APB2PeriphClockCmd(WS_GPIO_RCC | WS_TIM_RCC, ENABLE);

    //PC4 = TIM1_CH4 AF_PP
    gpio.GPIO_Pin   = WS_GPIO_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(WS_GPIO_PORT, &gpio);

    //Timer base
    TIM_Cmd(WS_TIM, DISABLE);

    tb.TIM_Period        = WS_TIM_ARR;
    tb.TIM_Prescaler     = WS_TIM_PRESCALER;
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(WS_TIM, &tb);

    //PWM on CH4
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = WS_CCR_0;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC4Init(WS_TIM, &oc);

    //Immediate CCR updates (DMA writes should take effect right away)
    TIM_OC4PreloadConfig(WS_TIM, TIM_OCPreload_Disable); //Disable -> changes to CCR are applied immediately (no waiting for update event)
    TIM_ARRPreloadConfig(WS_TIM, ENABLE); //Enable -> ARR is only updated upon the next update event

    //Keep disabled until actually sending the pulses out
    TIM_CtrlPWMOutputs(WS_TIM, DISABLE);
}

static void ws_dma_init(uint16_t *buf, uint16_t len)
{
    DMA_InitTypeDef dma = {0};

    RCC_AHBPeriphClockCmd(WS_DMA_RCC, ENABLE);

    DMA_DeInit(WS_DMA_CH);

    dma.DMA_PeripheralBaseAddr = (uint32_t)&WS_TIM_CCR_REG;
    dma.DMA_MemoryBaseAddr     = (uint32_t)buf;
    dma.DMA_DIR                = DMA_DIR_PeripheralDST; //memory to peripheral (buffer to timer)
    dma.DMA_BufferSize         = len;

    dma.DMA_PeripheralInc      = DMA_PeripheralInc_Disable; //Peripheral address is NOT incremented
    dma.DMA_MemoryInc          = DMA_MemoryInc_Enable; //Memory address IS incremented (next item in the buffer...)

    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //16 bits
    dma.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;

    dma.DMA_Mode               = DMA_Mode_Normal;
    dma.DMA_Priority           = DMA_Priority_VeryHigh;
    dma.DMA_M2M                = DMA_M2M_Disable;

    DMA_Init(WS_DMA_CH, &dma);
}

static void ws_reset_latch(void)
{
    GPIO_InitTypeDef gpio = {0};

    //Stop timer + outputs
    TIM_Cmd(WS_TIM, DISABLE);
    TIM_CtrlPWMOutputs(WS_TIM, DISABLE);

    //Force pin low as GPIO
    gpio.GPIO_Pin   = WS_GPIO_PIN;
    gpio.GPIO_Speed = GPIO_Speed_30MHz;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP; //Use it as a normal GPIO
    GPIO_Init(WS_GPIO_PORT, &gpio);
    GPIO_ResetBits(WS_GPIO_PORT, WS_GPIO_PIN);

    Delay_Us(WS_RESET_US); //Wait for a reset signal length
    
    gpio.GPIO_Mode = GPIO_Mode_AF_PP; //Restore AF_PP -> timer-related IO
    GPIO_Init(WS_GPIO_PORT, &gpio);
}

static uint16_t ws_build_ccr_from_strip(const ws2812b_t *strip) //build the pulse train for the DMA
{
    uint16_t idx = 0; //index for the buffer items

    ccrBuf[idx++] = WS_CCR_0; //Extra pulse in the beginning

    for (uint16_t led = 0; led < strip->count; led++) //Iterate over all the LEDs in the strip
    {
        uint8_t g = strip->pixels[led].g; //Fetch the LED's base colors and store them locally
        uint8_t r = strip->pixels[led].r;
        uint8_t b = strip->pixels[led].b;

        //global brightness scaling based on the fetched R, G and B values
        if (strip->brightness != 255)
        {
            uint16_t br = strip->brightness;
            g = (uint8_t)((g * br + 127) / 255);
            r = (uint8_t)((r * br + 127) / 255);
            b = (uint8_t)((b * br + 127) / 255);
        }

        //GRB, MSB first
        for (int bit = 7; bit >= 0; bit--) //Create the green bits
        {
            ccrBuf[idx++] = bit_to_ccr(g & (1 << bit));
        } 
        for (int bit = 7; bit >= 0; bit--) //Create the red bits
        {
            ccrBuf[idx++] = bit_to_ccr(r & (1 << bit));
        } 
        for (int bit = 7; bit >= 0; bit--) //Create the blue bits
        {
            ccrBuf[idx++] = bit_to_ccr(b & (1 << bit));
        } 

        /*Example
        b = 1011 0010

        bit = 7 (first for loop iteration)
        (1 << bit) == 1000 0000 (number 1 was shifted left 7 times). This is a mask.
        b & 1000 0000 == 1011 0010 & 1000 0000 ----> first number becomes 1 - CCR becomes WS_CCR_1

        bit = 6 (second for loop iteration)
        (1 << bit) == 0100 0000 (number 1 was shifted left 6 times).
        b & 0100 0000 == 1011 0010 & 0100 0000 ----> second number becomes 0 - CCR becomes WS_CCR_0

        bit = 5 (third for loop iteration)
        (1 << bit) = 0010 0000 (number 1 was shifted left 5 times)
        b & 0010 0000 == 1011 0010 & 0010 000 ----> third number becomes 1 - CCR becomes WS_CCR_1

        ...now you should know the pattern :)
        */
    }

    ccrBuf[idx++] = WS_CCR_0; //Extra pulse in the end

    return idx; 
}


static void ws_send_ccr_slots(uint16_t *buf, uint16_t slotCount)
{
    //Clean frame start + latch time
    ws_reset_latch();

    //Timer+DMA are fully stopped/flags are cleaned
    TIM_Cmd(WS_TIM, DISABLE);
    TIM_CtrlPWMOutputs(WS_TIM, DISABLE);
    TIM_DMACmd(WS_TIM, TIM_DMA_Update, DISABLE);

    DMA_Cmd(WS_DMA_CH, DISABLE);
    DMA_DeInit(WS_DMA_CH);

    DMA_ClearFlag(WS_DMA_GL_FLAG);
    DMA_ClearFlag(WS_DMA_TC_FLAG);

    //Setup DMA for full transfer
    ws_dma_init(buf, slotCount);

    //Sync timer
    TIM_SetCounter(WS_TIM, 0);
    TIM_ClearFlag(WS_TIM, TIM_FLAG_Update);

    //Enable DMA requests on timer update
    TIM_DMACmd(WS_TIM, TIM_DMA_Update, ENABLE);

    //Start DMA first, then timer
    DMA_Cmd(WS_DMA_CH, ENABLE);
    TIM_GenerateEvent(WS_TIM, TIM_EventSource_Update);

    //Clear update flag so the first real PWM bit starts cleanly
    TIM_ClearFlag(WS_TIM, TIM_FLAG_Update);

    //Enable output and timer
    TIM_CtrlPWMOutputs(WS_TIM, ENABLE);
    TIM_Cmd(WS_TIM, ENABLE);

    //Wait for DMA completion
    while (DMA_GetFlagStatus(WS_DMA_TC_FLAG) == RESET) {}

    TIM_ClearFlag(WS_TIM, TIM_FLAG_Update);
    while (TIM_GetFlagStatus(WS_TIM, TIM_FLAG_Update) == RESET) {}

    //Stop timer, output and DMA
    TIM_Cmd(WS_TIM, DISABLE);
    TIM_CtrlPWMOutputs(WS_TIM, DISABLE);
    TIM_DMACmd(WS_TIM, TIM_DMA_Update, DISABLE);
    DMA_Cmd(WS_DMA_CH, DISABLE);

    //Latch end
    ws_reset_latch();
}

//------------------------ Public API------------------------
void ws2812b_init(ws2812b_t *strip, ws2812b_color_t *pixel_buf, uint16_t count)
{
    if (!strip || !pixel_buf) return; //Return if there's no strip or buffer declared
    if (count == 0) return; //Return if there's no LED count
    if (count > WS2812_MAX_LEDS) return; //Return if there's invalid number of LEDs

    ws_timer_init(); //Initialize timer

    strip->count = count; //Access the 'count' member of the 'strip' struct through a pointer and pass the 'count' parameter to it
    strip->pixels = pixel_buf; //Access the 'pixels' member of the 'strip' struct through a pointer and pass the 'pixel_buf' parameter to it
    strip->brightness = 64; //Access the 'brightness' member of the 'strip' struct through a pointer and set the value to 64. (initial value)
    ws2812b_clear(strip);

    s_strip = strip; //Cache strip pointer so internal functions can access the active WS2812B instance

    //ensure known low state before first show
    ws_reset_latch();
}

void ws2812b_show(const ws2812b_t *strip)
{
    if (!strip || !strip->pixels) return; //Return if there's no LED count
    if (strip->count == 0) return;  //Return if there's 0 LED count
    if (strip->count > WS2812_MAX_LEDS) return; //Return if there's invalid LED count

    uint16_t slots = ws_build_ccr_from_strip(strip); //Get the number of slots
    ws_send_ccr_slots(ccrBuf, slots); //Send out the train pulse to the LEDs
}

void ws2812b_clear(ws2812b_t *strip) //Clear the whole strip
{
    if (!strip || !strip->pixels) return;

    for (uint16_t i = 0; i < strip->count; i++) //Assign zeroes to each LEDs RGB pixel values (brightness)
    {
        strip->pixels[i].g = 0;
        strip->pixels[i].r = 0;
        strip->pixels[i].b = 0;
    }
}

void ws2812b_set_pixel(ws2812b_t *strip, uint16_t index, uint8_t r, uint8_t g, uint8_t b) //Set an individual pixel
{
    if (!strip || !strip->pixels) return; //Return if there is nothing to access
    if (index >= strip->count) return; //Return if the user tries to access an LED outside of the number of LEDs 

    strip->pixels[index].g = g; //Assign the colors
    strip->pixels[index].r = r;
    strip->pixels[index].b = b;
}

ws2812b_color_t ws2812b_get_pixel(const ws2812b_t *strip, uint16_t index) //Fetch the color of a selected pixel
{
    if (!strip || !strip->pixels) return (ws2812b_color_t){0,0,0}; //Return 0 if there's nothing to access
    if (index >= strip->count)    return (ws2812b_color_t){0,0,0}; //Return 0 if the user is accessing an LED outside of bounds
    return strip->pixels[index]; //Return the actual pixel
}

void ws2812b_set_brightness(ws2812b_t *strip, uint8_t brightness) //Set the whole strip's (global) brightness
{
    if (!strip) return; //If there's nothing to access, return
    strip->brightness = brightness; //Set the brightness
}

uint8_t ws2812b_get_brightness(const ws2812b_t *strip) //Get the whole strip's (global) brightness
{
    if (!strip) return 0; //If there's nothing to access, return
    return strip->brightness; //Return the strip's brightness
}


void ws2812b_reset(void)
{    
    ws_reset_latch(); //Call reset
}
