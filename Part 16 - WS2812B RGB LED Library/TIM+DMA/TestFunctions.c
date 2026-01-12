#include "ch32v00x.h"
#include "debug.h"
#include "WS2812B_TIM_DMA.h"
#include "TestFunctions.h"

//Article: https://curiousscientist.tech/blog/ch32v003j4m6-mini-board-ws2812b-rgb-led-tim-pwm-dma

static ws2812b_t *g_strip = 0;
static uint16_t g_led_count = 0;

void Initialize_Tests(ws2812b_t *strip, uint16_t led_count) //Initialize the strip
{
    g_strip = strip;
    g_led_count = led_count;
}

void test_single_led_set(uint16_t pos, uint8_t r, uint8_t g, uint8_t b) //Set a single LED to a custom color
{
    if (pos-1 >= g_led_count) return; //Subtract 1 because the "human numbering" starts at 1

    ws2812b_clear(g_strip); //Clear the previous pixels
    ws2812b_set_pixel(g_strip, pos-1, r, g, b); //Subtract 1 because the "human numbering" starts at 1
    ws2812b_show(g_strip); //Pass the above assigned color to the corresponding LED on the strip
}

void test_dual_bounce_demo(uint16_t steps, uint16_t delay_ms,
                           uint8_t r1, uint8_t g1, uint8_t b1,
                           uint8_t r2, uint8_t g2, uint8_t b2) //Two custom colored LEDs are bouncing against each other
{
    if (!g_strip || g_led_count == 0) return; //Exit if the details are insufficient

    int16_t p1 = 0;
    int16_t p2 = (int16_t)g_led_count - 1;
    int8_t  d1 = +1;
    int8_t  d2 = -1;

    for (uint16_t k = 0; k < steps; k++)
    {
        ws2812b_clear(g_strip);

        //draw the two dots (guarded just in case)
        if (p1 >= 0 && p1 < (int16_t)g_led_count)
            ws2812b_set_pixel(g_strip, (uint16_t)p1, r1, g1, b1);

        if (p2 >= 0 && p2 < (int16_t)g_led_count)
            ws2812b_set_pixel(g_strip, (uint16_t)p2, r2, g2, b2);

        ws2812b_show(g_strip);
        Delay_Ms(delay_ms);

        //advance
        p1 += d1;
        p2 += d2;

        //bounce off ends
        if (p1 >= (int16_t)g_led_count - 1) { p1 = (int16_t)g_led_count - 1; d1 = -1; }
        if (p1 <= 0)                        { p1 = 0;                       d1 = +1; }

        if (p2 >= (int16_t)g_led_count - 1) { p2 = (int16_t)g_led_count - 1; d2 = -1; }
        if (p2 <= 0)                        { p2 = 0;                       d2 = +1; }
    }
}

static uint8_t scale8(uint8_t v, uint8_t scale) //Helper to scale the color to 8 bits
{   
    return (uint8_t)(((uint16_t)v * (uint16_t)scale + 127) / 255);  //scale v (0..255) by scale (0..255) with rounding
}

void test_rotating_red_brightness_gradient(uint16_t rotations, uint16_t frame_delay_ms, uint8_t min_brightness) //Rotating a dimmed portion
{
    if (!g_strip || g_led_count < 2) return; //The strip must exist and must have more than 1 LED

    if (min_brightness == 0) //Limit the lowest brightness to 1 to avoid awkward animation
    {
         min_brightness = 1; 
    }
    const uint8_t max_brightness = 255; //Max brightness is locally capped
    const uint16_t frames = (uint16_t)(rotations * g_led_count); //Number of frames

    for (uint16_t shift = 0; shift < frames; shift++) //Shift the LEDs "frames times"
    {
        for (uint16_t i = 0; i < g_led_count; i++) //Go through all LEDs
        {            
            uint16_t k = (uint16_t)((i + shift) % g_led_count); //position along the ramp, rotated by 'shift'

            uint16_t span = (uint16_t)(max_brightness - min_brightness); //Calculate the span of brightness levels
            uint8_t br = (uint8_t)(min_brightness + (uint32_t)k * span / (g_led_count - 1)); //Calculate the brightness
            
            uint8_t r = scale8(255, br); //red base = 255 scaled by per-LED brightness
            ws2812b_set_pixel(g_strip, i, r, 0, 0); //Apply the brightness
        }
        ws2812b_show(g_strip); //Show the assembled strip
        Delay_Ms(frame_delay_ms); //Wait before showing the next frame
    }
}

void test_walk_one_bit(uint16_t delay_ms) //One LED walking through the strip at a time and getting dimmer and dimmer
{  
    for (int ch = 0; ch < 3; ch++) //Go through each channels (R, G, B)
    {
        for (int bit = 7; bit >= 0; bit--) //Go through all bits of each channels, starting from the highest bit (basically 7 steps of dimming)
        {
            uint8_t g=0, r=0, b=0;
            uint8_t v = (uint8_t)(1u << bit); 

            if (ch == 0) //Assign the bit to the channel according to the outer for loop
            {
                g = v;
            }
            if (ch == 1)
            {
                r = v;
            } 
            if (ch == 2)
            {
                 b = v;
            }

            for (uint16_t pos = 0; pos < g_led_count; pos++) //Move a single lit pixel across the strip
            {
                ws2812b_clear(g_strip); //Clear the strip
                ws2812b_set_pixel(g_strip, pos, r, g, b); //Set the pixel
                ws2812b_show(g_strip); //Apply the pixel value
                Delay_Ms(delay_ms); //Wait between 2 steps
            }
        }
    }
}

void test_all_on_off_hammer(uint16_t delay_ms) //Full power ON for some amount of time
{       
    uint8_t temp_brightness = ws2812b_get_brightness(g_strip); //Get the recent brightness   
    
    ws2812b_set_brightness(g_strip, 255);  //Set global brightness to max brightness
    ws2812b_show(g_strip); //Apply the settings

    ws2812b_clear(g_strip); //all LEDs off
    ws2812b_show(g_strip);
    Delay_Ms(100);
    
    for (uint16_t i = 0; i < g_led_count; i++) //all white (max bits)
    {
        ws2812b_set_pixel(g_strip, i, 255, 255, 255); //R = G = B = 255, full power white
    }

    ws2812b_show(g_strip);
    Delay_Ms(delay_ms);    
    
    ws2812b_set_brightness(g_strip, temp_brightness); //Restore the brightness
    ws2812b_show(g_strip); //Apply the settings
}

void test_alternating_patterns(uint16_t delay_ms) //Alternating "inverse" patterns to check pulse stability
{
    for(int i = 0; i < 10; i++) //Do it 10 times
    {
        //Pattern A: 0xAA = 1010 1010 = 170
        for (uint16_t i = 0; i < g_led_count; i++)
        {
            ws2812b_set_pixel(g_strip, i, 0, 0, 0xAA);
        }        
        ws2812b_show(g_strip);
        Delay_Ms(delay_ms);        

        //Pattern B: 0x55 = 0101 0101 = 85
        for (uint16_t i = 0; i < g_led_count; i++)
        {
            ws2812b_set_pixel(g_strip, i, 0, 0, 0x55); 
        }
        ws2812b_show(g_strip);
        Delay_Ms(delay_ms);
    }    
}

void test_fast_pong(uint8_t r, uint8_t g, uint8_t b, uint16_t delay_ms)
{
    int pos = 0; //Position and direction of the bouncing LED
    int dir = +1;

    for(int i = 0; i < g_led_count * 24; i++) //Do some bounces. With 24, it stops at the last pixel if not cleared
    {
        ws2812b_clear(g_strip);
        ws2812b_set_pixel(g_strip, (uint16_t)pos, r, g, b); //Light the LED up at the actual position
        ws2812b_show(g_strip);

        if (pos >= (g_led_count - 1)) //If we reach the end of the strip, flip the direction
        {
            dir = -1;
        }
        else if(pos <= 0) //Or, if we are back at the beginning, we flip again
        {
            dir = +1;
        } 

        pos += dir; //Increase (dir = +1) or decrease (dir = -1) the position

        Delay_Ms(delay_ms); 
    }    
}

void test_unique_per_led(uint16_t delay_ms) //Assign pseudo-random unique LED color to each LED in a strip
{
    for (uint16_t i = 0; i < g_led_count; i++) //Set some random color to the ith
    {            
        uint8_t r = (uint8_t)(i+1 * 18); //+1 so that the first iteration does not produce a non-lit LED
        uint8_t g = (uint8_t)(i+1 * 22);
        uint8_t b = (uint8_t)(i+1 * 34);
        ws2812b_set_pixel(g_strip, i, r, g, b);
    }
    ws2812b_show(g_strip);
    Delay_Ms(delay_ms);

    //invert all
    for (uint16_t i = 0; i < g_led_count; i++)
    {
        ws2812b_color_t c = ws2812b_get_pixel(g_strip, i);
        ws2812b_set_pixel(g_strip, i, (uint8_t)(255 - c.r), (uint8_t)(255 - c.g), (uint8_t)(255 - c.b));
    }
    ws2812b_show(g_strip);
    Delay_Ms(delay_ms);    
}

static ws2812b_color_t spectrum_color_at(uint16_t idx, uint16_t count) //Helper to assign a spectrum color to a position based on the number of LEDs in a strip
{   
    uint16_t x = (uint16_t)((uint32_t)idx * 255u / (uint32_t)(count - 1));  //Normalize index to 0..255

    ws2812b_color_t c = {0, 0, 0};

    //Segments:
    //0..63   : Blue -> Green
    //64..127 : Green -> Yellow
    //128..191: Yellow -> Orange
    //192..255: Orange -> Red

    if (x <= 63)
    {
        uint8_t t = (uint8_t)(x * 4);
        c.r = 0;
        c.g = t;
        c.b = (uint8_t)(255 - t);
    }
    else if (x <= 127)
    {
        uint8_t t = (uint8_t)((x - 64) * 4);
        c.r = t;
        c.g = 255;
        c.b = 0;
    }
    else if (x <= 191)
    {
        uint8_t t = (uint8_t)((x - 128) * 4);
        c.r = 255;
        c.g = (uint8_t)(255 - t / 2); 
        c.b = 0;
    }
    else
    {
        uint8_t t = (uint8_t)((x - 192) * 4);
        c.r = 255;
        c.g = (uint8_t)(128 - t / 2); 
        c.b = 0;
    }
    return c;
}

void test_static_spectrum(void)
{
    for (uint16_t i = 0; i < g_led_count; i++) //Assign a color to each pixel in the strip
    {
        ws2812b_color_t c = spectrum_color_at(i, g_led_count); //Convert the ith position to color
        ws2812b_set_pixel(g_strip, i, c.r, c.g, c.b); //Assign the above obtained color to the ith pixel
    }

    ws2812b_show(g_strip); //Send out the pulse train
    Delay_Ms(2000); //Rest a bit        
}

void test_rotating_spectrum(uint16_t rotations, uint16_t delay_ms)
{
    if (!g_strip || g_led_count < 2) return; //Return if there is no strip or not enough LEDs

    const uint16_t frames = (uint16_t)(rotations * g_led_count); //Calculate the amount of frames

    for (uint16_t shift = 0; shift < frames; shift++) //Shift through the frames
    {
        for (uint16_t i = 0; i < g_led_count; i++) //Go through each LEDs in the strip
        {            
            uint16_t k = (uint16_t)((i + shift) % g_led_count); //Rotate the spectrum by shifting the sampling index

            ws2812b_color_t c = spectrum_color_at(k, g_led_count); //Assign the color to the LED
            ws2812b_set_pixel(g_strip, i, c.r, c.g, c.b); //Set the assigned color
        }

        ws2812b_show(g_strip); //Show the strip's contents
        Delay_Ms(delay_ms);
    }
}


void test_spectrum_brightness_sweep(void)
{
    uint8_t temp_brightness = ws2812b_get_brightness(g_strip); //Get the recent brightness

    for (uint8_t br = 1; br < 255; br++) //Ramp up brightness to max
    {
        ws2812b_set_brightness(g_strip, br); //Set the new brightness

        for (uint16_t i = 0; i < g_led_count; i++) //Assign colors to each LEDs
        {
            ws2812b_color_t c = spectrum_color_at(i, g_led_count);
            ws2812b_set_pixel(g_strip, i, c.r, c.g, c.b);
        }

        ws2812b_show(g_strip); //Push the pulses to the strip
        Delay_Ms(20); //Wait 20 ms at the current brightness level
    }  
    for (uint8_t br = 255; br > 0; br--) //Ramp down brightness to min
    {
        ws2812b_set_brightness(g_strip, br); //Set the new brightness

        for (uint16_t i = 0; i < g_led_count; i++) //Assign colors to each LEDs
        {
            ws2812b_color_t c = spectrum_color_at(i, g_led_count);
            ws2812b_set_pixel(g_strip, i, c.r, c.g, c.b);
        }

        ws2812b_show(g_strip); //Push the pulses to the strip
        Delay_Ms(20); //Wait 20 ms at the current brightness level
    }

        ws2812b_clear(g_strip); //Set all LEDs to zero       
        ws2812b_show(g_strip);  //Send the pulses to the strip

        ws2812b_set_brightness(g_strip, temp_brightness); //Restore the brightness
}


void test_spectrum_buildup(uint16_t step_ms, uint16_t hold_ms)
{
    for(int i = 0; i < 5; i++) //Run five times
    {
        int target = g_led_count - 1; //The target position of the actual LED

            //Start from empty strip each full cycle ---> delete it
            ws2812b_clear(g_strip);
            ws2812b_show(g_strip);

            while (target >= 0) //Go until we run out of space on the strip
            {                
                for (int pos = 0; pos <= target; pos++) //Move a single LED from 0 up to target
                {
                    //Draw frame: parked LEDs already exist in strip.pixels[]
                    //Temporarily draw mover, show, then remove mover unless it is parking
                    ws2812b_color_t saved = ws2812b_get_pixel(g_strip, (uint16_t)pos);
                    ws2812b_color_t c = spectrum_color_at((uint16_t)pos, g_led_count);

                    ws2812b_set_pixel(g_strip, (uint16_t)pos, c.r, c.g, c.b);
                    ws2812b_show(g_strip);

                    //If not parking, restore what was there (usually off)
                    if (pos != target)
                    {
                        ws2812b_set_pixel(g_strip, (uint16_t)pos, saved.r, saved.g, saved.b);
                    }

                    Delay_Ms(step_ms);
                }               
                target--;  //We just parked at target (it stays as spectrum color there)
            }            
            Delay_Ms(hold_ms); //Full strip lit; hold, then restart
        }    
}

void test_last_bit_toggle(void)
{
    for(int i = 0; i < 10; i++) //Toggle 10 times 
    {
        ws2812b_clear(g_strip);
        ws2812b_set_pixel(g_strip, 0, 0, 0, 0);
        ws2812b_show(g_strip);
        Delay_Ms(200);

        ws2812b_clear(g_strip);
        ws2812b_set_pixel(g_strip, 0, 0, 0, 1);
        ws2812b_show(g_strip);
        Delay_Ms(200); 
    }

    //Toggles the 0000 0000 0000 0000 0000 0000 and 0000 0000 0000 0000 0000 0001 bit series a few times
    //It's used to see if the last bit (B0) spills over and becomes G7. 
}

