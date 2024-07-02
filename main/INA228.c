#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "esp_event.h"
#include "INA228.h"

/*
 * SHUNT_CAL is a conversion constant that represents the shunt resistance
 * used to calculate current value in Amps. This also sets the resolution
 * (CURRENT_LSB) for the current register.
 *
 * SHUNT_CAL is 15 bits wide (0 - 32768)
 *
 * SHUNT_CAL = 13107.2 x 10^6 x CURRENT_LSB x Rshunt
 *
 * CURRENT_LSB = Max Expected Current / 2^19
 */

#define CURRENT_LSB 	0.0000190735
#define SHUNT_CAL		2500

#define I2C_MASTER_SCL_IO 18
#define I2C_MASTER_SDA_IO 19
//#define I2C_MASTER_FREQ_HZ 100000

void ina228_i2c_init()
{
  /*i2c_config_t i2c_config = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = I2C_MASTER_FREQ_HZ
  };
  i2c_param_config(I2C_MASTER_NUM, &i2c_config);
  i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);*/
	esp_err_t res;

	i2c_config_t conf = {
	    .mode = I2C_MODE_MASTER,
	    .sda_io_num = I2C_MASTER_SDA_IO,         		// select GPIO specific to your project
	    .sda_pullup_en = GPIO_PULLUP_ENABLE,
	    .scl_io_num = I2C_MASTER_SCL_IO,         		// select GPIO specific to your project
	    .scl_pullup_en = GPIO_PULLUP_ENABLE,
	    .master.clk_speed = I2C_MASTER_FREQ_HZ,  // select frequency specific to your project
	};

	//res =
	i2c_param_config(I2C_MASTER_NUM, &conf);
	//if (res != ESP_OK) printf("Error in i2c_param_config()\r\n");

	//res =
	i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
	//if (res != ESP_OK) printf("Error in i2c_driver_install()\r\n");
}


void ina228_init(uint8_t i2c_master_port, uint8_t i2c_address)
{
	i2c_write_short(I2C_MASTER_NUM, i2c_address,/*INA228_SLAVE_ADDRESS*/ INA228_CONFIG, 0x8000);	// Reset

//	printf("Manufacturer ID:    0x%04X\r\n",i2c_read_short(i2c_master_port, INA228_SLAVE_ADDRESS, INA228_MANUFACTURER_ID));
//	printf("Device ID:          0x%04X\r\n",i2c_read_short(i2c_master_port, INA228_SLAVE_ADDRESS, INA228_DEVICE_ID));

	i2c_write_short(i2c_master_port, i2c_address,/*INA228_SLAVE_ADDRESS*/ INA228_SHUNT_CAL, SHUNT_CAL);
}

esp_err_t i2c_write_byte(uint8_t i2c_master_port, uint8_t address, uint8_t command, uint8_t data)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, command, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_OK) {
		//printf("i2c_write_byte successful\r\n");
	} else {
		printf("i2c_write_byte failed\r\n");
	}

	return(ret);
}

esp_err_t i2c_write_short(uint8_t i2c_master_port, uint8_t address, uint8_t command, uint16_t data)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, command, ACK_CHECK_EN);

	i2c_master_write_byte(cmd, (data & 0xFF00) >> 8, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, data & 0xFF, ACK_CHECK_EN);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_OK) {
		//printf("i2c_write successful\r\n");
	} else {
		//printf("i2c_write_short failed\r\n");
	}

	return(ret);
}

esp_err_t i2c_write_buf(uint8_t i2c_master_port, uint8_t address, uint8_t command, uint8_t *data, uint8_t len)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, command, ACK_CHECK_EN);
	if (len) {
		for (int i = 0; i < len; i++) {
			i2c_master_write_byte(cmd, data[i], ACK_CHECK_EN);
		}
	}
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_OK) {
		//printf("i2c_write successful\r\n");
	} else {
		//printf("i2c_write_buf failed\r\n");
	}

	return(ret);
}

uint8_t i2c_read_byte(uint8_t i2c_master_port, uint8_t address, uint8_t command)
{
	i2c_write_buf(i2c_master_port, address, command, NULL, 0);

	uint8_t data;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, &data, NACK_VAL);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_OK) {
		//printf("i2c_read_byte successful\r\n");
	} else if (ret == ESP_ERR_TIMEOUT) {
		//printf("i2c_read_byte timeout\r\n");
	} else {
		//printf("i2c_read_byte failed\r\n");
	}
	return(data);
}

uint16_t i2c_read_short(uint8_t i2c_master_port, uint8_t address, uint8_t command)
{
	i2c_write_buf(i2c_master_port, address, command, NULL, 0);

	uint16_t data;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
	i2c_master_read(cmd, (uint8_t *)&data, 2, ACK_VAL);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);


	if (ret == ESP_OK) {

	} else if (ret == ESP_ERR_TIMEOUT) {
		//ESP_LOGW(TAG, "Bus is busy");
	} else {
		//ESP_LOGW(TAG, "Read failed");
	}
	return(__bswap16(data));
}

esp_err_t i2c_read_buf(uint8_t i2c_master_port, uint8_t address, uint8_t command, uint8_t *buffer, uint8_t len)
{
	i2c_write_buf(i2c_master_port, address, command, NULL, 0);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
	i2c_master_read(cmd, buffer, len, ACK_VAL);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_OK) {
		for (int i = 0; i < len; i++) {
			//printf("0x%02x ", data[i]);
		}
	} else if (ret == ESP_ERR_TIMEOUT) {
		//ESP_LOGW(TAG, "Bus is busy");
	} else {
		//ESP_LOGW(TAG, "Read failed");
	}
	return(ret);
}

float ina228_voltage(uint8_t i2c_master_port,uint8_t i2c_address)
{
	int32_t iBusVoltage;
	float fBusVoltage;
	bool sign;

	i2c_read_buf(i2c_master_port,i2c_address,/*INA228_SLAVE_ADDRESS*/INA228_VBUS, (uint8_t *)&iBusVoltage, 3);
	sign = iBusVoltage & 0x80;
	iBusVoltage = __bswap32(iBusVoltage & 0xFFFFFF) >> 12;
	if (sign) iBusVoltage += 0xFFF00000;
	fBusVoltage = (iBusVoltage) * 0.0001953125;

	return (fBusVoltage);
}

float ina228_dietemp(uint8_t i2c_master_port,uint8_t i2c_address)
{
	uint16_t iDieTemp;
	float fDieTemp;

	iDieTemp = i2c_read_short(i2c_master_port, i2c_address, INA228_DIETEMP);
	fDieTemp = (iDieTemp) * 0.0078125;

	return (fDieTemp);
}

float ina228_shuntvoltage(uint8_t i2c_master_port, uint8_t i2c_address)
{
	int32_t iShuntVoltage;
	float fShuntVoltage;
	bool sign;

	i2c_read_buf(i2c_master_port, i2c_address, INA228_VSHUNT, (uint8_t *)&iShuntVoltage, 3);
	sign = iShuntVoltage & 0x80;
	iShuntVoltage = __bswap32(iShuntVoltage & 0xFFFFFF) >> 12;
	if (sign) iShuntVoltage += 0xFFF00000;

	fShuntVoltage = (iShuntVoltage) * 0.0003125;		// Output in mV when ADCRange = 0
	//fShuntVoltage = (iShuntVoltage) * 0.000078125;	// Output in mV when ADCRange = 1

	return (fShuntVoltage);
}

float ina228_current(uint8_t i2c_master_port, uint8_t i2c_address)
{
	int32_t iCurrent;
	float fCurrent;
	bool sign;

	i2c_read_buf(i2c_master_port, i2c_address, INA228_CURRENT, (uint8_t *)&iCurrent, 3);
	sign = iCurrent & 0x80;
	iCurrent = __bswap32(iCurrent & 0xFFFFFF) >> 12;
	if (sign) iCurrent += 0xFFF00000;
	fCurrent = (iCurrent) * CURRENT_LSB;

	return (fCurrent);
}

float ina228_power(uint8_t i2c_master_port, uint8_t i2c_address)
{
	uint32_t iPower;
	float fPower;

	i2c_read_buf(i2c_master_port, i2c_address, INA228_POWER, (uint8_t *)&iPower, 3);
	iPower = __bswap32(iPower & 0xFFFFFF) >> 8;
	fPower = 3.2 * CURRENT_LSB * iPower;

	return (fPower);
}

/*
 * Returns energy in Joules.
 * 1 Watt = 1 Joule per second
 * 1 W/hr = Joules / 3600
 */

float ina228_energy(uint8_t i2c_master_port, uint8_t i2c_address)
{
	uint64_t iEnergy;
	float fEnergy;

	i2c_read_buf(i2c_master_port, i2c_address, INA228_ENERGY, (uint8_t *)&iEnergy, 5);
	iEnergy = __bswap64(iEnergy & 0xFFFFFFFFFF) >> 24;

	fEnergy = 16 * 3.2 * CURRENT_LSB * iEnergy;

	return (fEnergy);
}

/*
 * Returns electric charge in Coulombs.
 * 1 Coulomb = 1 Ampere per second.
 * Hence Amp-Hours (Ah) = Coulombs / 3600
 */

float ina228_charge(uint8_t i2c_master_port, uint8_t i2c_address)
{
	int64_t iCharge;
	float fCharge;
	bool sign;

	i2c_read_buf(i2c_master_port, i2c_address, INA228_CHARGE, (uint8_t *)&iCharge, 5);
	sign = iCharge & 0x80;
	iCharge = __bswap64(iCharge & 0xFFFFFFFFFF) >> 24;
	if (sign) iCharge += 0xFFFFFF0000000000;

	fCharge = CURRENT_LSB * iCharge;

	return (fCharge);
}

#if 0
float ina228_get_voltage()
{
	uint16_t voltage_raw;
	float voltage;

	// Read the voltage register from the INA228
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (INA228_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, INA228_REG_VOLTAGE, true);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (INA228_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
	i2c_master_read_byte(cmd, &voltage_raw, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &voltage_raw, I2C_MASTER_NACK);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	// Convert the raw value to voltage
	voltage = (float)voltage_raw * INA228_LSB_VOLTAGE;

	return voltage;
}

float ina228_get_power()
{
    // Read the power from the INA228
    uint16_t power_raw;
    float power;

    // Read the power register from the INA228
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (INA228_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, INA228_REG_POWER, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (INA228_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &power_raw, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &power_raw, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    // Convert the raw value to power
    power = (float)power_raw * INA228_LSB_POWER;

    return power;
}
#endif
