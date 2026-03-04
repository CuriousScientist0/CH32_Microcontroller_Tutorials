#include "i2c.h"        
#include "bme280.h"
#include <ch32v00X_conf.h>  

uint8_t rxbuf[10]; //Buffer for the received bytes. 

void IIC_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PC1 - SDA
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD; //OD because of the pullup resistor
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PC2 - SCL
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD; //OD because of the pullup resistor
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    I2C_InitStructure.I2C_ClockSpeed = 400000; //typical speed, also used for Arduinos
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9; 
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; 
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);
    I2C_AcknowledgeConfig( I2C1, ENABLE );//host mode
}

void I2C_SendByte(uint8_t deviceAddress, uint8_t *Buffer, uint8_t numberOfBytes)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET); //Wait for the flag to change

    I2C_GenerateSTART(I2C1, ENABLE); //Generate start

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)); //Wait for confirmation

    I2C_Send7bitAddress(I2C1, deviceAddress << 1, I2C_Direction_Transmitter); //Pay attention to the shifting due to the 8-bit variable but 7-bit address

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); //Set MCU as transmitter

    for(uint8_t n=0;n<numberOfBytes;n++)
    {
       if( I2C_GetFlagStatus( I2C1, I2C_FLAG_TXE ) !=  RESET )
        {
           I2C_SendData( I2C1, Buffer[n]);
            while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED ) );
        }
    }

    I2C_GenerateSTOP(I2C1, ENABLE); //Generate stop
}

void I2C_ReceiveByte(uint8_t address, uint8_t numberOfBytes)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Receiver);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    for(uint8_t n=0;n<numberOfBytes;n++)
     {
        while( !I2C_GetFlagStatus( I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED | I2C_FLAG_BTF));
        Delay_Us(1);
        if(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) != RESET)
        {
            rxbuf[n] = I2C_ReceiveData(I2C1);
        }
     }

    I2C_GenerateSTOP(I2C1, ENABLE);
}

void I2C_WriteRegister(uint8_t devAddr7, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    I2C_SendByte(devAddr7, buf, 2);
}

uint8_t I2C_WriteRegisters(uint8_t devAddr7, uint8_t reg, const uint8_t *data, uint8_t len)
{
    //Build [reg][data...]
    uint8_t buf[1 + 32];
    if (len > 32) return FAIL;

    buf[0] = reg;
    for (uint8_t i = 0; i < len; i++) buf[1 + i] = data[i];

    I2C_SendByte(devAddr7, buf, (uint8_t)(1 + len));
    return OK;
}

uint8_t I2C_ReadRegisters(uint8_t devAddr7, uint8_t reg, uint8_t *data, uint8_t len)
{
    //Write register address
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, devAddr7 << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET);
    I2C_SendData(I2C1, reg);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    //Repeated start, then read
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, devAddr7 << 1, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    for(uint8_t i = 0; i < len; i++)
    {
        while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
        data[i] = I2C_ReceiveData(I2C1);
    }

    I2C_GenerateSTOP(I2C1, ENABLE);
    return OK;
}
