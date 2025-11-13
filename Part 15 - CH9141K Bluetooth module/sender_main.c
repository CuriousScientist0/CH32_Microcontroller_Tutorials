/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2024/01/01
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *CH32V006K8U6 - Sender
 *https://curiousscientist.tech/blog/ch32v003f4p6-ch32v006k8u6-ch9141-bluetooth-communication
 */

#include "debug.h"
#include "string.h"

/* Global define */
#define U2_RX_MAX   256 //Max size of RX buffer

/* Global Variable */
static volatile uint8_t  U2_RxBuf[U2_RX_MAX]; //RX buffer (volatile due to ISR)
static volatile uint16_t U2_RxCnt = 0; //RX buffer position counter
static volatile uint8_t  U2_RxDone = 0; //RX done flag
char line[256]; //Line char array


/*********************************************************************
 * @fn      USARTx_CFG
 *
 * @brief   Initializes the USART1 peripheral.
 *
 * @return  none
 */
void USART1_CFG(void) //Onboard USART (USB communication)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD | RCC_PB2Periph_USART1, ENABLE);

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

void GPIOPins_CFG(void) //AT pin is controlled by the microcontroller so we can switch between transmission and AT command modes
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; //PD4 - AT
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //Push-pull due to toggling
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void USART2_CFG(void) //BT module USART
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    NVIC_InitTypeDef  NVIC_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD | RCC_PB2Periph_AFIO, ENABLE);
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_USART2, ENABLE);

    GPIO_PinRemapConfig(GPIO_PartialRemap3_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PD2 - TX
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; //PD3 - RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //IPU or floating both works
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART2, &USART_InitStructure);
    //We add interrupt to USART2 (BT receive from Receiver) instead of polling
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);
}

void UART_SendString(USART_TypeDef* USARTx, const char *s)
{
    while(*s) //Iterate through the characters passed to the function
    {
        while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET); //Wait until the transmit data register becomes empty
        USART_SendData(USARTx, *s++); //Send the data (byte) to the USART1, and increment the pointer
    }
}

void USART2_RxArm(void)
{
    for (uint16_t i = 0; i < U2_RX_MAX; i++) //Erase the contents of the buffer
    {
        U2_RxBuf[i] = 0;
    }
    U2_RxCnt = 0; //Reset the counter
    U2_RxDone = 0; //Reset the flag
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); //Restart the interrupt
}

static uint16_t copyUSART2Buffer(char *destination, uint16_t destination_size)
{
    if (!U2_RxDone) //Return if the code is still receiving data
    { 
        return 0;
    }
    
    uint16_t n = (U2_RxCnt < (destination_size - 1)) ? U2_RxCnt : (destination_size - 1); //Get the number of bytes to copy

    memcpy(destination, U2_RxBuf, n * sizeof(U2_RxBuf[0]));

    /*
    for (uint16_t i = 0; i < n; i++) 
    {
        destination[i] = (char)U2_RxBuf[i]; //Copy the bytes from the buffer to the destination
    }
    */

    destination[n] = '\0';  //NULL-terminate at the last position in the array

    //Strip CR/LF tail (optional)
    while (n && (destination[n-1] == '\r' || destination[n-1] == '\n'))
    {
         destination[--n] = '\0';
    }

    return n; //Return the final length of the string after trimming
}

static void quickFlushLeftover(uint32_t quiet_ms) //Flush the garbage from the USART2 (sometimes an "OK" arrives after the response that messes with the rest of the logic)
{
    uint32_t elapsedMillis = 0; //millis counter

    while (!U2_RxDone && elapsedMillis < quiet_ms)  //wait until the buffer is full and enough time elapses
    { 
        Delay_Ms(1);
        elapsedMillis++; //Waste time
    }

    if (U2_RxDone) //Rearm USART2 when the code is done with the above stuff
    {        
        USART2_RxArm();
    }
}

void waitForBufferAndPrint()
{   
    while(!U2_RxDone){} //Wait for the buffer to be full
    printf("%s\r\n", U2_RxBuf); //Print all of it + linebreaks    
    quickFlushLeftover(120); //Wait 120 ms for garbage on USART2 and flush (dispose) it
    USART2_RxArm(); //Rearm the receiver
}

void initialize_Bluetooth()
{
    GPIO_WriteBit(GPIOD, GPIO_Pin_4, Bit_RESET); //pull AT LOW. -- AT Command Mode

    Delay_Ms(500); //Wait so that the line settles
    USART2_RxArm();
    UART_SendString(USART2, "AT+HELLO?\r\n"); //Ask for a response   
    waitForBufferAndPrint();

    printf("\nBLEMODE: ");
    UART_SendString(USART2, "AT+BLEMODE?\r\n"); //Ask the BLE mode, 0: broadcast, 1: host, 2: device
    waitForBufferAndPrint();
    
    printf("\nName: ");
    UART_SendString(USART2, "AT+NAME?\r\n"); //Ask the name of the device
    waitForBufferAndPrint();

    printf("\nConnecting to receiver...");
    UART_SendString(USART2, "AT+CONN=7B:49:69:7B:54:50\r\n"); //Connect to a known MAC address
    waitForBufferAndPrint();

    printf("\nBLESTA: ");
    UART_SendString(USART2, "AT+BLESTA?\r\n"); //Get the bluetooth connection status, 03: successfully connected
    waitForBufferAndPrint();
    
    printf("\nMAC: ");
    UART_SendString(USART2, "AT+MAC?\r\n"); //Get the MAC of THIS device
    waitForBufferAndPrint();

    GPIO_WriteBit(GPIOD, GPIO_Pin_4, Bit_SET); //pull AT HIGH. -- Data mode (exit AT mode)    
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

    USART1_CFG(); //USB to USART chip for PC terminal
    USART2_CFG(); //USART towards the BT module
    GPIOPins_CFG(); //Configure AT as output

    /*
    //Test code for demonstrating applying settings separately
    GPIO_WriteBit(GPIOD, GPIO_Pin_4, Bit_RESET);
    Delay_Ms(500); //Wait so that the line settles
    USART2_RxArm();
    UART_SendString(USART2, "AT+NAME=DemoDevice\r\n"); 
    waitForBufferAndPrint();
    printf("\nNAME: ");
    UART_SendString(USART2, "AT+NAME?\r\n"); 
    waitForBufferAndPrint();
    //----
    printf("\nBLEMODE: "); 
    UART_SendString(USART2, "AT+BLEMODE?\r\n");
    waitForBufferAndPrint();
    GPIO_WriteBit(GPIOD, GPIO_Pin_4, Bit_SET);   
    while(1){} //Hang the code here
    */ 

    initialize_Bluetooth(); //Set up connection and connect to the receiver
    
    printf("CH32V006K8U6 - Bluetooth with CH9141K.\n");   

    while(1)
    {     
        //Line from USART2 to PC (ISR() origin)
        uint16_t n = copyUSART2Buffer(line, sizeof(line));   
        if (n) //If the line that was copied is not empty (n>0)
        {
            printf("%s\r\n", line); //Print the line on the serial terminal (USART1)
            USART2_RxArm(); //ready for the next line once we printed the line
        }  

        //PC terminal to USART2 (BT module) - Polling
        if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) //Check if USART1 has a byte waiting
        {
            uint8_t data = USART_ReceiveData(USART1) & 0xFF; //If yes, copy the byte from USART1 to data

            while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET); //Wait until the end of transmission
            USART_SendData(USART2, data); //Send the data to USART2 (BT module)
        }     
    }
}

void USART2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*********************************************************************
 * @fn      USART2_IRQHandler
 *
 * @brief   This function handles USART2 global interrupt request.
 *
 * @return  none
 */
void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //If there is something to read
    {
        uint8_t receivedByte = (uint8_t)(USART2->DATAR & 0xFF); //Read the line directly from the data register (this also clears RXNE flag)

        if (!U2_RxDone) //If we haven't yet gotten a complete line
        {
            if (U2_RxCnt < U2_RX_MAX) //If we are within the capacity of the buffer
            {
                U2_RxBuf[U2_RxCnt++] = receivedByte; //Add the recent byte to the buffer and advance the counter
            }            

            if (receivedByte == '\n' || U2_RxCnt >= U2_RX_MAX) //If we're at the end of the line or reached the buffer's capacity
            {
                U2_RxDone = 1; //Raise the flag
                USART_ITConfig(USART2, USART_IT_RXNE, DISABLE); //Disable (disarm!) the interrupt for now, so no new bytes are written into the buffer
            }
        }
    }
}
