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
 *CH32V003F4P6 - Receiver
 *https://curiousscientist.tech/blog/ch32v003f4p6-ch32v006k8u6-ch9141-bluetooth-communication
 */

#include "debug.h"

/* Global define */
#define RXBUF_SZ 64 //Receive buffer size

/* Global Variable */
static char rxbuf[RXBUF_SZ]; //Receive buffer
static uint16_t rxi = 0; //i-th item index for the receive buffer

void USART1_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //PD5
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; //PD6
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

void LED_Init(void) //Initializes the GPIO (PD3) for LED blinking
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; //PD3
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //Pin toggling
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

static void send_usart1(const char* s) //Send back the string of characters to the sender
{
    while(*s) //Iterate through the characters passed to the function
    {
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)==RESET){} //Wait until the transmit data register becomes empty
        USART_SendData(USART1, *s++); //Send the data (byte) to the USART1, and increment the pointer
    }
    //When the pointer hits the '\0' NULL-TERMINATE character, the while(0) makes the function to return, the work is done.
}

static void send_usart1_crlf(void)
{
    send_usart1("\r\n"); //Send a line ending according to the requirements of the BT communication protocol (CR-LF)
}

static void processLine(void)
{
    if(rxi == 0) 
    {
        return; //empty line, ignore
    }

    rxbuf[rxi] = 0; //NULL-terminate (useful for text/echo)

    if(rxbuf[0] == '!') //If the first character is "!"
    {        
        if(rxi >= 2)//One-character command expected as rxbuf[1]
        {
            char cmd = rxbuf[1]; //Extract rxbuf[1], (character after "!")

            switch(cmd)
            {
                case 'A': //Toggle PD3
                    GPIO_WriteBit(GPIOD, GPIO_Pin_3, (BitAction)!GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_3));
                    send_usart1("OK: toggled\r\n");
                    break;  

                //Send back a "reading" from our imaginary weather station
                case 'T': 
                    send_usart1("Temperature: 23\r\n"); //Celsius
                break;   

                case 'H':
                    send_usart1("Humidity: 65\r\n"); //RH%
                break;  
                    
                case 'P':
                    send_usart1("Pressure: 1010\r\n"); //hpa
                break; 
                //-----------------------------------------------------------------------------------------

                default:
                    send_usart1("ERR: unknown cmd\r\n");
                    break;
            }
        }
        else
        {
            send_usart1("ERR: missing cmd\r\n");
        }
    }
    else
    {
        //Echo the whole line back
        send_usart1("ECHO: "); //Send the text "ECHO"
        send_usart1(rxbuf); //Send the contents of the received buffer
        send_usart1_crlf(); //Send a CRLF
    }
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
    USART1_CFG(); //Init USART1
    LED_Init(); //Init LED on PD3

    for(int i = 0; i < 10; i++) //Blink the LED upon startup to see that the code is running
    {
        GPIO_WriteBit(GPIOD, GPIO_Pin_3, (BitAction)!GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_3)); //Toggle PD3 10x
        Delay_Ms(100); //Wait 100 ms between toggling 
    }

    while(1)
    {         
        while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
        {
            uint8_t receivedByte = USART_ReceiveData(USART1) & 0xFF; //Keep only the lower byte of the 16-bit data (if it is 16-bit...)

            if(receivedByte == '\r') //Ignore carriage return (CR)
            {                
                continue; //Jump to the next iteration
            }
            else if(receivedByte == '\n') //Check if the received byte is a line feed (LF) character
            {                
                processLine(); //If so, it is the end of the message, so we process the line
                rxi = 0; //reset buffer - next line starts from the beginning of the buffer
            }
            else //If no CR/LF was received
            {                
                if(rxi < RXBUF_SZ - 1) //accumulate, with overflow protection, leaving a room for the null-terminator ('\0')
                {
                    rxbuf[rxi++] = (char)receivedByte; //Cast the received byte to char and add it to the buffer. Also, advance the pointer
                }
                else //Reached the buffer's maximum capacity
                {                    
                    rxi = 0; //Reset the buffer
                    send_usart1("ERR: overflow\r\n"); //Let the user know that the line received was larger than the buffer
                }
            }
        }
    }
}
