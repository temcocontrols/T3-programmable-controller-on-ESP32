#ifndef INA228_H
#define INA228_H

#include "driver/i2c.h"

#if 0
// INA228 I2C address
#define INA228_I2C_ADDRESS 0x40

// Register addresses
#define INA228_REG_CONFIG 0x00
#define INA228_REG_SHUNT_VOLTAGE 0x01
#define INA228_REG_VOLTAGE 0x02
#define INA228_REG_POWER 0x03
#define INA228_REG_CURRENT 0x04

// Function to initialize the INA228
esp_err_t ina228_init(i2c_port_t i2c_num);

// Function to read the shunt voltage
esp_err_t ina228_read_shunt_voltage(i2c_port_t i2c_num, float *voltage);

// Function to read the bus voltage
esp_err_t ina228_read_bus_voltage(i2c_port_t i2c_num, float *voltage);

// Function to read the power
esp_err_t ina228_read_power(i2c_port_t i2c_num, float *power);

// Function to read the current
esp_err_t ina228_read_current(i2c_port_t i2c_num, float *current);
#endif

#ifndef MAIN_INA228_H_
#define MAIN_INA228_H_

#define INA228_SLAVE_ADDRESS		0x40
#define INA228_SLAVE_OUTPUT_ADDRESS	0x45


#define I2C_MASTER_NUM I2C_NUM_0

#define INA228_CONFIG			0x00
#define INA228_ADC_CONFIG		0x01
#define INA228_SHUNT_CAL		0x02
#define INA228_SHUNT_TEMPCO		0x03
#define INA228_VSHUNT			0x04
#define INA228_VBUS			0x05
#define INA228_DIETEMP			0x06
#define INA228_CURRENT			0x07
#define INA228_POWER			0x08
#define INA228_ENERGY			0x09
#define INA228_CHARGE			0x0A
#define INA228_DIAG_ALRT		0x0B
#define INA228_SOVL			0x0C
#define INA228_SUVL			0x0D
#define INA228_BOVL			0x0E
#define INA228_BUVL			0x0F
#define INA228_TEMP_LIMIT		0x10
#define INA228_PWR_LIMIT		0x11
#define INA228_MANUFACTURER_ID	0x3E
#define INA228_DEVICE_ID		0x3F

#define I2C_CONTROLLER_0		0
#define I2C_CONTROLLER_1		1

#define I2C_MASTER_FREQ_HZ		400000

#define I2C_MASTER_RX_BUF_DISABLE	0
#define I2C_MASTER_TX_BUF_DISABLE	0

#define ACK_CHECK_EN			0x1
#define ACK_CHECK_DIS			0x0

#define ACK_VAL				0x0
#define NACK_VAL			0x1

//void i2c_init(uint8_t i2c_master_port, uint8_t sda_io_num, uint8_t scl_io_num);

esp_err_t i2c_write_byte(uint8_t i2c_master_port, uint8_t address, uint8_t command, uint8_t data);
esp_err_t i2c_write_short(uint8_t i2c_master_port, uint8_t address, uint8_t command, uint16_t data);
esp_err_t i2c_write_buf(uint8_t i2c_master_port, uint8_t address, uint8_t command, uint8_t *data, uint8_t len);

uint8_t   i2c_read_byte(uint8_t i2c_master_port, uint8_t address, uint8_t command);
uint16_t  i2c_read_short(uint8_t i2c_master_port, uint8_t address, uint8_t command);
esp_err_t i2c_read_buf(uint8_t i2c_master_port, uint8_t address, uint8_t command, uint8_t *buffer, uint8_t len);


void ina228_i2c_init();
void ina228_init(uint8_t i2c_master_port,uint8_t i2c_address);
float ina228_voltage(uint8_t i2c_master_port,uint8_t i2c_address);
float ina228_dietemp(uint8_t i2c_master_port,uint8_t i2c_address);
float ina228_shuntvoltage(uint8_t i2c_master_port,uint8_t i2c_address);
float ina228_current(uint8_t i2c_master_port,uint8_t i2c_address);
float ina228_power(uint8_t i2c_master_port,uint8_t i2c_address);
float ina228_energy(uint8_t i2c_master_port,uint8_t i2c_address);
float ina228_charge(uint8_t i2c_master_port,uint8_t i2c_address);

#endif

#endif // INA228_H
