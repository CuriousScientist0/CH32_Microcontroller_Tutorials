#ifndef I2C_H
#define I2C_H
#define OK    1
#define FAIL  0

#include <stdint.h>

extern uint8_t rxbuf[10];

//Native IIC
void IIC_Init();
void I2C_ReceiveByte(uint8_t address, uint8_t numberOfBytes);
void I2C_SendByte(uint8_t deviceAddress, uint8_t *Buffer, uint8_t numberOfBytes);

void I2C_WriteRegister(uint8_t devAddr7, uint8_t reg, uint8_t value);
uint8_t I2C_WriteRegisters(uint8_t devAddr7, uint8_t reg, const uint8_t *data, uint8_t len);
uint8_t I2C_ReadRegisters(uint8_t devAddr7, uint8_t reg, uint8_t *data, uint8_t len);

#endif //I2C_H
