#include "mmI2c.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_i2c.h"
#include "misc.h"
#include <string.h>

// Global variables
uint8_t i2c1_mode = I2C1_MODE_WAITING;
uint8_t i2c_rcv[I2C1_RAM_SIZE]; // I2C RAM buffer

void Set_I2C1_Ram(uint8_t adr, uint8_t val) 
{
    i2c_rcv[adr] = val;
}

void I2C1_Slave_Init(void) 
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    I2C_InitTypeDef  I2C_InitStructure;

    // Enable I2C1 and GPIOB clocks
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    // Configure I2C1 pins: SCL and SDA
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure the I2C event priority
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Configure the I2C error priority
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // I2C1 configuration
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = I2CSLAVE_ADDR;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C1_CLOCK_FRQ;
    I2C_Init(I2C1, &I2C_InitStructure);

    // Enable I2C1
    I2C_Cmd(I2C1, ENABLE);
}

void I2C1_EV_IRQHandler(void) 
{
    static uint8_t reg_addr = 0; // Variable to store the register address

    // Handle I2C1 event interrupt
    switch (I2C_GetLastEvent(I2C1)) {
        case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED:
            i2c1_mode = I2C1_MODE_SLAVE_ADR_WR;
            break;
        case I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED:
            i2c1_mode = I2C1_MODE_SLAVE_ADR_RD;
            break;
        case I2C_EVENT_SLAVE_BYTE_RECEIVED:
            if (i2c1_mode == I2C1_MODE_SLAVE_ADR_WR) {
                uint8_t data = I2C_ReceiveData(I2C1);
                if (i2c1_mode == I2C1_MODE_SLAVE_ADR_WR) {
                    reg_addr = data; // Store the register address
                    i2c1_mode = I2C1_MODE_ADR_BYTE;
                } else if (i2c1_mode == I2C1_MODE_ADR_BYTE) {
                    Set_I2C1_Ram(reg_addr, data); // Write data to the register
                    i2c1_mode = I2C1_MODE_DATA_BYTE_WR;
                }
            }
            break;
        case I2C_EVENT_SLAVE_BYTE_TRANSMITTED:
            if (i2c1_mode == I2C1_MODE_SLAVE_ADR_RD) {
                I2C_SendData(I2C1, i2c_rcv[reg_addr]); // Read data from the register
                i2c1_mode = I2C1_MODE_DATA_BYTE_RD;
            }
            break;
        default:
            break;
    }
}

void I2C1_ER_IRQHandler(void) 
{
    // Handle I2C1 error interrupt
    if (I2C_GetITStatus(I2C1, I2C_IT_AF)) {
        I2C_ClearITPendingBit(I2C1, I2C_IT_AF);
    }
}

void Update_I2C_Buffer(uint8_t *data, uint16_t length) 
{
    if (length > I2C1_RAM_SIZE) {
        length = I2C1_RAM_SIZE; // Ensure we don't overflow the buffer
    }
    memcpy(i2c_rcv, data, length);
}