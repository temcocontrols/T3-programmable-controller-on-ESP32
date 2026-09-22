#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_i2c.h"
#include "FreeRTOS.h"
#include "stm32f10x.h"
#include "task.h"
#include "queue.h"
#include "key.h"
#include "misc.h"
#include "spi.h"
#include "crc.h"
#include <string.h>
#include "types.h"
#include "inputs.h"
#include "ES51932_Driver.h"

#define I2CSLAVE_ADDR           (0x74 << 1)  // address is 0x74

#define I2C1_CLOCK_FRQ          100000     // I2C-Frq in Hz (100 kHz)
#define I2C1_RAM_SIZE           256        // RAM Size in Byte

#define I2C1_MODE_WAITING       0          // Waiting for commands
#define I2C1_MODE_SLAVE_ADR_WR  1          // Received slave address (writing)
#define I2C1_MODE_ADR_BYTE      2          // Received ADR byte
#define I2C1_MODE_DATA_BYTE_WR  3          // Data byte (writing)
#define I2C1_MODE_SLAVE_ADR_RD  4          // Received slave address (to read)
#define I2C1_MODE_DATA_BYTE_RD  5          // Data byte (to read)

uint8_t i2c1_mode = I2C1_MODE_WAITING;
//uint8_t i2c1_ram_adr = 0;
//uint8_t i2c1_ram[I2C1_RAM_SIZE];
#define TOP_HARDWARE  6
#define TOP_FIRMWARE  2

extern vu16 AD_Value[32];

//uint8_t rcv_byte;
extern u16 test[];

uint8_t i2c_send[200];
uint8_t i2c_rcv[100];
uint8_t i2c_send_index = 0;
uint8_t i2c_rcv_index;
//extern int16 I2C_Sensor_tem_org[3];
//extern int16 I2C_Sensor_hum_org[3];
extern uint8_t hum_exists;
extern uint16_t g_key;

uint8_t Get_I2C1_Ram(uint8_t adr) 
{
    return i2c_send[adr];//i2c1_ram[adr];
}
extern xQueueHandle qKey;
void I2C_TX_buffer(void)
{
	u16 index;
	u16 key_temp;
	vu16 crc;
	vu16 count = 0;
	u8 i2c_tx_temp[200];
	count = 0;
		
//	index = 0;
//	do
//	{		
//			if((index >= 0) && (index < 10))   // NO switch
//			{
//				if(index == 0)		i2c_tx_temp[index] = 0x55;
//				if(index == 1)		i2c_tx_temp[index] = 0xaa;
//				if(index == 2)		i2c_tx_temp[index] = TOP_HARDWARE;
//				if(index == 3)		i2c_tx_temp[index] = TOP_FIRMWARE;
//				//if(xQueueReceive(qKey, &key_temp, 0) == pdTRUE)
//				{
//					if(index == 4)		i2c_tx_temp[index] = g_key >> 8;
//					if(index == 5)		i2c_tx_temp[index] = g_key;
//				}
//				
//			}
//			else if((index >= 24) && (index < 56))   // input value
//			{	
//				if((index - 24) % 2 == 0)
//				{
//					i2c_tx_temp[index] = (u8)(AD_Value[(index - 24) / 2] >> 8);
//				}
//				else 
//				{
//					i2c_tx_temp[index] = (u8)(AD_Value[(index - 24) / 2]);
//				}
//			}			
//			else
//				i2c_tx_temp[index] = 0;
//	}while(index++ < 112);
//	
//	crc = crc16(i2c_tx_temp,112);	
//	i2c_tx_temp[112] = crc / 256;
//	i2c_tx_temp[113] = crc % 256;
//	memcpy(i2c_send,i2c_tx_temp,114);

}


void Set_I2C1_Ram(uint8_t adr, uint8_t val) 
{
    i2c_rcv[adr] = val;//i2c1_ram[adr] = val;
    return;
}


void I2C1_Slave_Init(void) 
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    I2C_InitTypeDef  I2C_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* Configure I2C_EE pins: SCL and SDA */
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure the I2C event priority */
    NVIC_InitStructure.NVIC_IRQChannel                   = I2C1_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Configure I2C error interrupt to have the higher priority */
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&NVIC_InitStructure);

    /* I2C configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = I2CSLAVE_ADDR;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C1_CLOCK_FRQ;

    /* I2C Peripheral Enable */
    I2C_Cmd(I2C1, ENABLE);

    /* Apply I2C configuration after enabling it */
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_ITConfig(I2C1, I2C_IT_EVT, ENABLE); //Part of the STM32 I2C driver
    I2C_ITConfig(I2C1, I2C_IT_BUF, ENABLE);
    I2C_ITConfig(I2C1, I2C_IT_ERR, ENABLE); //Part of the STM32 I2C driver

//    I2C1_Ram_Init();
}

void I2C1_ClearFlag(void) 
{
    /* ADDR Flag clear */
    while((I2C1->SR1 & I2C_SR1_ADDR) == I2C_SR1_ADDR) 
    {
        I2C1->SR1;
        I2C1->SR2;
    }

    /* STOPF Flag clear */
    while((I2C1->SR1&I2C_SR1_STOPF) == I2C_SR1_STOPF) 
    {
        I2C1->SR1;
        I2C1->CR1 |= 0x1;
    }
}


extern u8 range[32];
uint8_t multiChannel;
void output_control(uint8_t * output);

void I2C1_EV_IRQHandler(void) 
{
    uint8_t wert;
    uint32_t event;
		uint8_t i;
		static uint8_t reg_addr = 0; // Variable to store the register address
	
    /* Reading last event */
    event = I2C_GetLastEvent(I2C1);

    /* Event handle */
    if(event == I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED) 
    {
        // Master has sent the slave address to send data to the slave
        i2c1_mode = I2C1_MODE_SLAVE_ADR_WR;
    }
    else if(event == I2C_EVENT_SLAVE_BYTE_RECEIVED) 
    {
        // Master has sent a byte to the slave
        wert = I2C_ReceiveData(I2C1);
        // Check address
        if(i2c1_mode == I2C1_MODE_SLAVE_ADR_WR) 
        {
            i2c1_mode = I2C1_MODE_ADR_BYTE;
            // Set current ram address
 //           i2c1_ram_adr = wert;
            reg_addr = wert; // Store the register address
            i2c_rcv_index = 0;
        }
        else 
        {
            i2c1_mode = I2C1_MODE_DATA_BYTE_WR;
            // Store data in RAM
            Set_I2C1_Ram(i2c_rcv_index, wert);
            // Next ram adress
            // Check if the register address is 10 and data is in the range 1 to 8
            if (reg_addr == 10 && wert >= 1 && wert <= 8) {
                // Ensure that serial data has been received before executing ControlChannels
                //if (receivedSerialData) 
                {
                    ControlChannels(wert); // Call ControlChannels to set the channel
                    multiChannel = wert;
                }
            }
            // Check if the register address is 11, the number is for Mode selection
            if(reg_addr == 11){
                // Check if the data is within the mode range
                if(wert <= MODE_CAPACITANCE_MEASUREMENT_CLAMP){
                    SetMode(wert); // Set the mode
                }
            }
			// Check if the register address is 11, the number is for Mode selection
            if(reg_addr == 12){
                if(wert == SELECT_DC || wert == SELECT_AC){
                    // Add your code here to handle DC/AC selection
                    if(wert == SELECT_DC){
                        // Handle DC selection
                        SetSLACDC(SELECT_DC); // Set to DC mode
                    } else if(wert == SELECT_AC){
                        // Handle AC selection
                        SetSLACDC(SELECT_AC); // Set to AC mode
                    }
                }							
			}
        }
    }
    else if(event == I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED) 
    {
        // Master has sent the slave address to read data from the slave
        i2c1_mode = I2C1_MODE_SLAVE_ADR_RD;
        // Read data from RAM
				i2c_send_index = 0;
        wert = Get_I2C1_Ram(i2c_send_index);			
				
        // Send data to the master
        I2C_SendData(I2C1, wert);
        // Next ram adress
        i2c_send_index++;
				
    }
    else if(event == I2C_EVENT_SLAVE_BYTE_TRANSMITTED) 
    {
        // Master wants to read another byte of data from the slave
        i2c1_mode = I2C1_MODE_DATA_BYTE_RD;
        // Read data from RAM
        wert = Get_I2C1_Ram(i2c_send_index);
        // Send data to the master
        I2C_SendData(I2C1, wert);	
				
        // Next ram adress
        if(i2c_send_index < 114)
					i2c_send_index++;
    }
    else if(event == I2C_EVENT_SLAVE_STOP_DETECTED) 
    {
        // Master has STOP sent
        I2C1_ClearFlag();
        i2c1_mode = I2C1_MODE_WAITING;
    }
}

void I2C1_ER_IRQHandler(void) 
{
    if(I2C_GetITStatus(I2C1, I2C_IT_AF)) 
    {
        I2C_ClearITPendingBit(I2C1, I2C_IT_AF);
    }
}