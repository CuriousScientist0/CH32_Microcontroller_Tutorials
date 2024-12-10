/*********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *CH32V003F4P6 - Setup and blinky, introduction to GPIOs
 *https://curiousscientist.tech/blog/ch32v003f4p6-setup-and-gpios
 */

#include "debug.h"


/* Global define */


/* Global Variable */
int16_t buttonValue = 0; //Counter for button clicks

/*********************************************************************
 * @fn      USARTx_CFG
 *
 * @brief   Initializes the USART2 & USART3 peripheral.
 *
 * @return  none
 */
void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    /* USART1 TX-->D.5   RX-->D.6 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //Alternate function (TX), push-pull
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Floating input pin, state is controlled by the programmer 
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200; //Default baud rate - Make sure you set the same on the terminal!
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void initializeLEDGPIOs()
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3; //Pin 0, 2 and 3
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //Push-pull output: they can be set to high (SET) or low (RESET)
    GPIO_Init(GPIOD, &GPIO_InitStructure); //Pass the above settings to port D

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure); //Pass the above settings to port C

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Floating input (pin is driven by an external source; here - pullup resistor)
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void blinkLEDs()
{
    for(int i = 0; i < 20; i++) //Run the toggling 2 times
    {
    GPIO_WriteBit(GPIOD, GPIO_Pin_0, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_0) == Bit_SET) ? Bit_RESET : Bit_SET);
    //Modify the state of PD0 pin based on the last set databit.
    //Read the databit on PD0, and invert its current status. This inverted status is then written on the pin.
    Delay_Ms(50); //Wait 50 ms (Blocking)

    GPIO_WriteBit(GPIOD, GPIO_Pin_2, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_2) == Bit_SET) ? Bit_RESET : Bit_SET);
    Delay_Ms(50);

    GPIO_WriteBit(GPIOD, GPIO_Pin_3, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_3) == Bit_SET) ? Bit_RESET : Bit_SET);
    Delay_Ms(50);

    GPIO_WriteBit(GPIOC, GPIO_Pin_1, (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_1) == Bit_SET) ? Bit_RESET : Bit_SET);
    Delay_Ms(50);
    }
}

void readButtonPolling_Blink()
{
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == Bit_RESET) //PC0 goes LOW
    {
        blinkLEDs(); //Run the function
    }
    else
    {

    }
    Delay_Ms(100); //Poor man's debounce - Adjust the delay to achieve the desired behavior
}

void readButtonPolling_USART()
{
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == Bit_RESET) //PC2 goes low (reset)
    {
        buttonValue++; //Increase the value of the buttonValue variable by 1
        printf("Button value: %d\n", buttonValue); //Print it on the serial terminal
    }
    else
    {
		//The rising edge (release of button) is not treated here
    }

    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == Bit_RESET) //PC3 goes low (reset)
    {
        buttonValue--; //Decrease the value of the buttonValue variable by 1
        printf("Button value: %d\n", buttonValue); //Print it on the serial terminal
    }
    else
    {
		//The rising edge (release of button) is not treated here
    }
}


/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void) //"Setup()" part in an Arduino code
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    USARTx_CFG();
    Delay_Ms(5000); //Some initial delay so after uploading, you have time to open the terminal and see the message on it
    printf("CH32V003F4P6 DEMO PART 1 - GPIO"); //Print some message on the serial terminal

    initializeLEDGPIOs();
    blinkLEDs();
	
	// End of "Setup()"

    while(1) //"Loop()" part in an Arduino code
    {
        readButtonPolling_Blink();
        readButtonPolling_USART();
    }
}
