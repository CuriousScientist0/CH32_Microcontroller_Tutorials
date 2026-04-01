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
 *@Note
 *CH32V003F4P6 - DM8BA10 - 10-digit, 16-segment LCD 
 *https://curiousscientist.tech/blog/ch32v003f4p6-with-dm8ba10-alphanumerical-display
 */

#include "debug.h"
#include "tm1622.h"
#include <string.h>

char receivedData; //The data which we exchange via the USART, it is a single character
char textBuffer[11]; //Buffer that contains the incoming text
uint8_t textReceived = 0; //Flag indicator for received text

const uint16_t fonts[] =
{
    0x0000, // space
    0x0000, // !
    0x4020, // "
    0x0000, // #
    0x0000, // $
    0xAE73, // %
    0x0000, // &
    0x4000, // '
    0x0000, // (
    0x0000, // )
    0xA3A3, // *
    0x8221, // +
    0x000C, // ,
    0x8001, // -
    0x0000, // .
    0x0180, // /
    0x7DDE, // 0
    0x0044, // 1
    0x9C59, // 2
    0x985D, // 3
    0xC045, // 4
    0xD81D, // 5
    0xDC1D, // 6
    0x1054, // 7
    0xDC5D, // 8
    0xD85D, // 9
    0x0000, // :
    0x0000, // ;
    0x0082, // <
    0x8809, // =
    0x2100, // >
    0x1251, // ?
    0x0000, // @
    0xD455, // A
    0xCC0D, // B
    0x5C18, // C
    0x8C4D, // D
    0xDC19, // E
    0xD410, // F
    0x5C1D, // G
    0xC445, // H
    0x0220, // I
    0x0C4C, // J
    0x02A2, // K
    0x4C08, // L
    0x64C4, // M
    0x6446, // N
    0x5C5C, // O
    0xD451, // P
    0x5D5C, // Q
    0xD453, // R
    0xD81D, // S
    0x1230, // T
    0x4C4C, // U
    0x2046, // V
    0x4E4C, // W
    0x2182, // X
    0xC241, // Y
    0x1998, // Z
    0x5C00, // [
    0x2002, // \ //
    0x1A20, // ]
}; 
//Note - Symbols that are hard to render are replaced with space. 0-9 and A-Z are usable, plus a few simple symbols.

void USARTx_CFG(void) //For explanation, refer to my USART tutorial
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    /* USART1 TX-->D.5   RX-->D.6 */
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

void receiveCommandUSART(char *textBuffer)
{
    uint8_t index = 0; //Index for keeping track of the position inside the buffer
    char receivedCharacter; //Received character from USART

    memset(textBuffer, 0, 11); //Clear the buffer. Copy zeroes (0) to the first 10 items (length)

    while (index < (12 - 1)) //12-1 because the last character must be the null-terminator we add manually
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET); //Wait for the flag

        receivedCharacter = USART_ReceiveData(USART1); //Store the received character

        if (receivedCharacter == '\n' || receivedCharacter == '\r') //Abort if the character is an endline character
        {
            break;
            //Note: Some terminals need to be configured to send an LF to terminate the message!
        }

        textBuffer[index++] = receivedCharacter; //Otherwise add the character to the buffer
    }

    textBuffer[index] = '\0'; //Null-terminate the string after all the characters are received (when endline breaks the while)
    textReceived = 1; //Indicator flag ON
}

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();    
    USART_Printf_Init(115200);
    USARTx_CFG();

    Delay_Ms(2000);
    printf("CH32V003F4P6 Demo Part 18 - DM8BA10 16-segment display\n");

    TM1622_Init();

    //Below, there is a bunch of different exercises that can help to understand the working principles of the display and the code

    //Light up 4 individual nibbles within a character
    TM1622_WriteDigitRaw(4, 0x000F);
    Delay_Ms(5000);
    TM1622_WriteDigitRaw(4, 0x00F0);
    Delay_Ms(5000);
    TM1622_WriteDigitRaw(4, 0x0F00);
    Delay_Ms(5000);
    TM1622_WriteDigitRaw(4, 0xF000);
    Delay_Ms(5000);
    TM1622_ClearAll();
    
    TM1622_ClearAll();
    //Walk through numbers
    for(int i = 16; i < 26; i++)
    {
        TM1622_WriteDigitRaw(5, fonts[i]);
        Delay_Ms(500);
        TM1622_ClearAll();
    }
    
    //Walk through letters
    for(int i = 33; i < 59; i++)
    {
        TM1622_WriteDigitRaw(5, fonts[i]);
        Delay_Ms(500);
        TM1622_ClearAll();
    }
  
    //Print some string
    TM1622_ClearAll();
    Delay_Ms(3000);
    TM1622_PrintString(8, "CURIOUS");
    Delay_Ms(2000);
    TM1622_PrintString(9, "SCIENTIST");
    Delay_Ms(3000);
  
    TM1622_ClearAll();
    //Loop decimal points from left to right
    for(int k = 0; k<3; k++)
    {
        for(int i = 0; i<3; i++)
        {
            for(int j = 0; j<3; j++)
            {
            TM1622_SetDecimalInGroup(i, j);
            Delay_Ms(500);
            }
            TM1622_ClearAllDecimals();
        }
    }

    //Show groups (they are only "related" visually)
    //Vertical group
    TM1622_ClearAll();
    TM1622_WriteDigitRaw(4, 0x4664);
    Delay_Ms(10000);

    //Horizontal group
    TM1622_ClearAll();
    TM1622_WriteDigitRaw(5, 0x9819);
    Delay_Ms(10000);

    //Diagonal group
    TM1622_ClearAll();
    TM1622_WriteDigitRaw(6, 0x2182);
    Delay_Ms(10000);

    //All together
    TM1622_WriteDigitRaw(7, 0xFFFF);
    Delay_Ms(30000);
    TM1622_ClearAll();

    //Walk through the whole display, starting from the first digit and light up all segments
    TM1622_TestSegment(0);
    
    //Testing decimal numbers in different formats. Decimal separator is a dot!    
    TM1622_ClearAll();
    TM1622_ClearAllDecimals();
    TM1622_PrintNumberString("3.1415");
    Delay_Ms(5000);    
    TM1622_ClearAll();
    TM1622_ClearAllDecimals();
    TM1622_PrintNumberString("-273.15");
    Delay_Ms(5000);    
    TM1622_ClearAll();
    TM1622_ClearAllDecimals();
    TM1622_PrintNumberString("3476.21");
    Delay_Ms(5000);    
    TM1622_ClearAll();
    TM1622_ClearAllDecimals();
    TM1622_PrintNumberString("-0.2261");
    Delay_Ms(5000);    
    TM1622_ClearAll();
    TM1622_ClearAllDecimals();
    TM1622_PrintNumberString("327.441");
    Delay_Ms(5000);    
    TM1622_ClearAll();
    TM1622_ClearAllDecimals();
    TM1622_PrintNumberString("10688912.7");
    Delay_Ms(5000); 
    TM1622_ClearAll();
    TM1622_ClearAllDecimals();   
    
    while(1)
    {        
        receiveCommandUSART(textBuffer);

        if(textReceived == 1) //If a text was received, let the code print it
        {
            TM1622_ClearAll();
            TM1622_ClearAllDecimals();
            //TM1622_PrintString(9, textBuffer); //Only CAPITAL letters are supported due to the display's style
            TM1622_PrintNumberString(textBuffer); //In case you want to send floating point numbers from the PC
            //Note: Use the string OR numberstring, but not both at the same time

            printf("Text received: %s\r\n", textBuffer);
            textReceived = 0; //Reset flag
        }

    }
}
