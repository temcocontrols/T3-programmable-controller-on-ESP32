#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "i2c_task.h"
#include "unistd.h"
#include "sgp30.h"
#include "driver/uart.h"
//#include "deviceparams.h"
#include <string.h>
#include "mlx90632.h"
#include "controls.h"
#include "define.h"
#include "scd4x_i2c.h"
#include "sht4x.h"
#include "ade7953.h"

static const char *TAG = "i2c-task";

SemaphoreHandle_t print_mux = NULL;
/**
 * @brief  code to operate on SHT31 sensor
 *
 * 1. set operation mode(e.g One time L-resolution mode)
 * _________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write 1 byte + ack  | stop |
 * --------|---------------------------|---------------------|------|
 * 2. wait more than 24 ms
 * 3. read data
 * ______________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read 1 byte + ack  | read 1 byte + nack | stop |
 * --------|---------------------------|--------------------|--------------------|------|
 */
// example P_R values
int32_t P_R = 0x00587f5b;
int32_t P_G = 0x04a10289;
int32_t P_T = 0xfff966f8;
int32_t P_O = 0x00001e0f;
int32_t Ea = 4859535;
int32_t Eb = 5686508;
int32_t Fa = 53855361;
int32_t Fb = 42874149;
int32_t Ga = -14556410;
int16_t Ha = 16384;
int16_t Hb = 0;
int16_t Gb = 9728;
int16_t Ka = 10752;

uint8_t sht31_data[6];
uint16_t scd40_data[6];
uint8_t light_data[2];
uint8_t temp_data[2];
g_sensor_t g_sensors;
uint8_t mlx90614_data[3];
//uint8_t which_project= PROJECT_FAN_MODULE;
double ambient; /**< Ambient temperature in degrees Celsius */
double object; /**< Object temperature in degrees Celsius */
float mlx90614_ambient;
float mlx90614_object;
extern uint8_t tempBuf_CO2[9];

int32_t mlx90632_i2c_read(int16_t register_address, uint16_t *value)
{
    int ret;
    uint8_t cmd_h,cmd_l;
    uint16_t got_data;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    cmd_h = HI_UINT16(register_address);
    cmd_l = LO_UINT16(register_address);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MLX90632_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_h,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_l,ACK_CHECK_EN);
	//i2c_master_write_byte(cmd, 0x00,ACK_CHECK_EN);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MLX90632_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, &temp_data[0], ACK_VAL);
	i2c_master_read_byte(cmd, &temp_data[1], NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	got_data = BUILD_UINT16(temp_data[1], temp_data[0]);
	memcpy(value, &got_data, 2);
	return ret;
}

int32_t mlx90632_i2c_write(int16_t register_address, uint16_t value)
{
	int ret;
	uint8_t cmd_h,cmd_l,data_h,data_l;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    cmd_h = HI_UINT16(register_address);
    cmd_l = LO_UINT16(register_address);
    data_h = HI_UINT16(value);
    data_l = LO_UINT16(value);
    i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MLX90632_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_h,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_l,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, data_h,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, data_l,ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}

void WaitEE(uint16_t ms)
{
	vTaskDelay(ms / portTICK_RATE_MS);
}

uint8_t Calculate_PEC (uint8_t initPEC, uint8_t newData)
{
    uint8_t data;
    uint8_t bitCheck;

    data = initPEC ^ newData;

    for (int i=0; i<8; i++ )
    {
        bitCheck = data & 0x80;
        data = data << 1;

        if (bitCheck != 0)
        {
            data = data ^ 0x07;
        }

    }
    return data;
}

int MLX90614_SMBusRead(uint8_t slaveAddr, uint8_t readAddress, uint16_t *data)
{
    uint8_t sa;
    int ack = 0;
    uint8_t pec;
    char cmd = 0;
    char smbData[3] = {0,0,0};
    uint16_t *p;

    p = data;
    sa = (slaveAddr << 1);
    pec = sa;
    cmd = readAddress;

    //i2c.stop();
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    usleep(5);
    i2c_master_start(i2c_cmd);
    //ack = i2c.write(sa, &cmd, 1, 1);
    i2c_master_write_byte(i2c_cmd, sa | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(i2c_cmd, cmd,ACK_CHECK_EN);

    //sa = sa | 0x01;
    //ack = i2c.read(sa, smbData, 3, 0);
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, sa | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(i2c_cmd, (uint8_t*)&smbData[0], ACK_VAL);
    i2c_master_read_byte(i2c_cmd, (uint8_t*)&smbData[1], ACK_VAL);
    i2c_master_read_byte(i2c_cmd, (uint8_t*)&smbData[2], ACK_VAL);
    i2c_master_stop(i2c_cmd);
	i2c_master_cmd_begin(I2C_MASTER_NUM, i2c_cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(i2c_cmd);
    //i2c.stop();
	sa = sa | 0x01;
    pec = Calculate_PEC(0, pec);
    pec = Calculate_PEC(pec, cmd);
    pec = Calculate_PEC(pec, sa);
    pec = Calculate_PEC(pec, smbData[0]);
    pec = Calculate_PEC(pec, smbData[1]);

    if (pec != smbData[2])
    {
    	//debug_msg("MLX90614_SMBusRead....failed...\r\n");
        return -2;
    }

    *p = (uint16_t)smbData[1]*256 + (uint16_t)smbData[0];
    //debug_msg("MLX90614_SMBusRead....SUCCESS...\r\n");
    return 0;
}

int MLX90614_GetTa(uint8_t slaveAddr, float *ta)
{
    int error = 0;
    uint16_t data = 0;

    error = MLX90614_SMBusRead(slaveAddr, 0x06, &data);

    if (data > 0x7FFF)
    {
        return -4;
    }

    *ta = (float)data * 0.02f - 273.15;

    return error;
}

int MLX90614_GetTo(uint8_t slaveAddr, float *to)
{
    int error = 0;
    uint16_t data = 0;

    error = MLX90614_SMBusRead(slaveAddr, 0x07, &data);

    if (data > 0x7FFF)
    {
        return -4;
    }

    if (error == 0)
    {
        *to = (float)data * 0.02f - 273.15;
    }

    return error;
}

void msleep(int msecs)
{
	vTaskDelay(msecs / portTICK_RATE_MS);
}

void mlx_usleep(int min_range, int max_range)
{
	usleep((min_range+max_range)/2);
}


float SHT3X_getTemperature(uint8_t * measure_data)
{
	float temperatureC;
	uint16_t temperature_raw;
	temperature_raw = measure_data[0];
	temperature_raw = temperature_raw << (uint16_t) 8 ;
	temperature_raw = temperature_raw + (uint16_t) measure_data[1];
	temperatureC = 175.0 * (float) temperature_raw / 65535.0;
	temperatureC = temperatureC - 45;
	return temperatureC;
}

float SHT3X_getHumidity(uint8_t * measure_data)
{
	float humidity;
	uint16_t humidity_raw;
	humidity_raw = measure_data[0];
	humidity_raw = humidity_raw << (uint16_t) 8;
	humidity_raw = humidity_raw + (uint16_t) measure_data[1];
	humidity = 100.0 * (float) humidity_raw / 65535.0;
	return humidity;
}

static esp_err_t i2c_master_sensor_mlx90614(i2c_port_t i2c_num, uint8_t *data)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    /*i2c_master_start(cmd);
    //i2c_master_write_byte(cmd, MLX90614_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x00,ACK_CHECK_EN);
    i2c_master_stop(cmd);
    i2c_master_start(cmd);
    //i2c_master_write_byte(cmd, MLX90614_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x00,ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret != ESP_OK) {
		return ret;
	}
    vTaskDelay(15 / portTICK_RATE_MS);*/

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MLX90614_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x1f,ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MLX90614_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd,data,3,ACK_VAL);
    //vTaskDelay(10 / portTICK_RATE_MS);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret != ESP_OK) {
		return ret;
	}
	return ret;
}

static esp_err_t i2c_master_sensor_sht31(i2c_port_t i2c_num, uint8_t *data)//_h, uint8_t *data_l)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHT31_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x24,ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x00,ACK_CHECK_EN);
    //BH1750_CMD_START, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(30 / portTICK_RATE_MS);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHT31_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    //i2c_master_read_byte(cmd, data_h, ACK_VAL);
    //i2c_master_read_byte(cmd, data_l, NACK_VAL);
    i2c_master_read(cmd,data,6,ACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t i2c_master_sensor_veml7700(i2c_port_t i2c_num, uint8_t *data_l, uint8_t *data_h)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, VEML7700_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, 0X00,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, 0x00,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, 0x00,ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
	if (ret != ESP_OK) {
		return ret;
	}
	vTaskDelay(30 / portTICK_RATE_MS);
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, VEML7700_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, 0x04,ACK_CHECK_EN);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, VEML7700_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, data_l, ACK_VAL);
	i2c_master_read_byte(cmd, data_h, NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}

static esp_err_t i2c_master_sensor_mlx90621(i2c_port_t i2c_num, uint8_t cmd_h, uint8_t cmd_l,
		uint8_t *data_l, uint8_t *data_h)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MLX90632_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_h,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_l,ACK_CHECK_EN);
	//i2c_master_write_byte(cmd, 0x00,ACK_CHECK_EN);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MLX90632_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, data_l, ACK_VAL);
	i2c_master_read_byte(cmd, data_h, NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}

static esp_err_t i2c_master_sensor_scd40(i2c_port_t i2c_num, uint8_t cmd_h, uint8_t cmd_l,
		uint8_t *data)
{
	int ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, SCD40_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_h,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_l,ACK_CHECK_EN);
	vTaskDelay(500/portTICK_RATE_MS);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, SCD40_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read(cmd,data,9,ACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}

esp_err_t sensirion_i2c_write(uint8_t address, const uint8_t *data,
                           uint16_t count) {
    int ret;
    uint16_t i;


    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    i2c_master_write_byte(cmd,address << 1| WRITE_BIT, ACK_CHECK_EN);
    for (i = 0; i < count; i++) {
        ret = i2c_master_write_byte(cmd, data[i],ACK_CHECK_EN);
    }
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}

esp_err_t sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count) {
	int ret;
	uint16_t i;
	uint8_t send_ack;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd,address << 1| READ_BIT, ACK_CHECK_EN);
    //usleep(600);
    for (i = 0; i < count; i++) {
		send_ack = i < (count - 1); /* last byte must be NACK'ed */
		i2c_master_read_byte(cmd, &data[i],!send_ack);
	}
    i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t LED_i2c_write(uint8_t address, const uint8_t *data,
                           uint16_t count) {
    int ret;
    uint16_t i;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    i2c_master_write_byte(cmd,address << 1| WRITE_BIT, ACK_CHECK_EN);
    for (i = 0; i < count; i++) {
        ret = i2c_master_write_byte(cmd, data[i],ACK_CHECK_EN);
    }
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}

// 0x74 STM32 chip device
esp_err_t stm_i2c_write(uint8_t reg,const uint8_t *value, uint16_t count)
{
    int ret;
    uint16_t i;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    i2c_master_write_byte(cmd,(0x74 << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg , ACK_CHECK_EN);
    for (i = 0; i < count; i++) {
        ret = i2c_master_write_byte(cmd, value[i],ACK_CHECK_EN);
    }

	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}

// 0x74 STM32 chip device
int32_t stm_i2c_read(int16_t reg, uint8_t *value,uint16_t len)
{
    int ret;
    uint8_t i;
    uint8_t send_ack;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x74 << 1) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg,ACK_CHECK_EN);

	vTaskDelay(50/portTICK_RATE_MS);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (0x74 << 1) | READ_BIT, ACK_CHECK_EN);
	/*for(i = 0;i < len - 1;i++)
		i2c_master_read_byte(cmd, &value[i], ACK_VAL);
	i2c_master_read_byte(cmd, &value[i], NACK_VAL);*/
	for (i = 0; i < len; i++) {
			send_ack = i < (len - 1); /* last byte must be NACK'ed */
			i2c_master_read_byte(cmd, &value[i],!send_ack);
		}


	i2c_master_stop(cmd);

	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}


void sensirion_sleep_usec(uint32_t useconds) {
	//delay_us(useconds);
	usleep(useconds);
    //HAL_Delay(useconds / 1000 + 1);
}
/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init()
{
	print_mux = xSemaphoreCreateMutex();
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    if((Modbus.mini_type == PROJECT_FAN_MODULE)||(Modbus.mini_type == PROJECT_TRANSDUCER)||((Modbus.mini_type == PROJECT_POWER_METER)))
    	conf.sda_io_num = 12;//4;//I2C_MASTER_SDA_IO;
    else
    	conf.sda_io_num = 4;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = 14;//I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
    memset(&g_sensors, 0, sizeof(g_sensor_t));
    //memset(&inputs,0,sizeof(Str_in_point));
    memcpy(inputs[0].label,"TEMP",strlen("TEMP"));
    memcpy(inputs[0].description,"Temperature",strlen("Temperature"));
}

void i2c_task(void *arg)
{
    int ret,i;
    uint8_t voc_ok;
	uint32_t temp;
	uint32_t iaq_baseline;
    uint32_t task_idx = (uint32_t)arg;
    int16_t ambient_new_raw=0;
    int16_t ambient_old_raw=0;
    int16_t object_new_raw=0;
    int16_t object_old_raw=0;
    int32_t sht4x_temp, sht4x_hum;

    g_sensors.co2_start_measure = false;
//    uint8_t sensor_data_h, sensor_data_l;
    int cnt = 0;
    g_sensors.co2_ready = false;
#if 0
    g_sensors.voc_baseline[0] = 0;
    g_sensors.voc_baseline[1] = 0;
    g_sensors.voc_baseline[2] = 0;
    g_sensors.voc_baseline[3] = 0;
    //g_sensors.ambient = 100;
    //g_sensors.object = 10;
    uint16_t voc_buf[5];
    static uint8_t voc_cnt;
    static	uint16_t baseline_time = 0;

    voc_ok = 1;
    voc_cnt = 0;
	temp = 0;
	while (sgp30_probe() != STATUS_OK) {
		temp++;
		if(temp>20)
		{
			voc_ok = 0;
			break;
		}
		vTaskDelay(100/ portTICK_RATE_MS);
	}
	temp = 0;
	if(voc_ok)
	{
		uint16_t feature_set_version;
		uint8_t product_type;
		ret = sgp30_get_feature_set_version(&feature_set_version, &product_type);

		uint64_t serial_id;
		ret = sgp30_get_serial_id(&serial_id);

		ret = sgp30_measure_raw_blocking_read(&g_sensors.ethanol_raw_signal, &g_sensors.h2_raw_signal);

		ret = sgp30_iaq_init();
		iaq_baseline = 0;
		iaq_baseline |= (uint32_t)g_sensors.voc_baseline[3] << 24;
		iaq_baseline |= (uint32_t)g_sensors.voc_baseline[2] << 16;
		iaq_baseline |= (uint32_t)g_sensors.voc_baseline[1] << 8;
		iaq_baseline |= (uint32_t)g_sensors.voc_baseline[0];

		if(iaq_baseline == 0xffffffff)
		{
			ret = sgp30_set_iaq_baseline(iaq_baseline);
			if(ret == STATUS_OK)
				printf(" set baseline: OK");

			vTaskDelay(100 / portTICK_RATE_MS);
		}
		if (ret == ESP_ERR_TIMEOUT) {
			ESP_LOGE(TAG, "I2C SGP30 Timeout");
		} else if (ret == ESP_OK) {
			// printf("*******************\n");
			// printf("TASK[%d]  MASTER READ SENSOR( SGP30 )\n", task_idx);
			// printf("*******************\n");
			//printf("data_1: %02x\n", client_data.buffer.words[0]);//sensor_data_h);
			//printf("data_2: %02x\n", client_data.buffer.words[1]);//sensor_data_l);
			//printf("data_3: %02x\n", client_data.buffer.words[2]);//sensor_data_h);
			//printf("data_4: %02x\n", client_data.buffer.words[3]);//sensor_data_l);
		}else {
			ESP_LOGW(TAG, "%s: No ack, SGP30 sensor not connected...skip...", esp_err_to_name(ret));
		}
	}

	if(which_project == PROJECT_SAUTER){
		ret = mlx90632_init();
		if(ret == 0)
		{

		}
	}
#endif
	if(Modbus.mini_type == PROJECT_POWER_METER)
	{
		Ade7953_init();
	}
	else
	{
		scd4x_wake_up();
		scd4x_stop_periodic_measurement();
		scd4x_reinit();
		scd4x_start_periodic_measurement();
	}
	while (1) {//Test[20]++;
#if 1
		if(Modbus.mini_type == PROJECT_POWER_METER)
		{
			Ade7953GetData();
			/*
			Test[0] = Ade7953_getVoltage();
			vTaskDelay(100 / portTICK_RATE_MS);
			//Test[1] = Ade7953_getCurrent(1);
			Test[2] = Ade7953_getCurrent(2);
			Test[3] = Ade7953_getActivePower(1);
			Test[4] = Ade7953_getActivePower(2);
			//Test[5] = Ade7953_getEnergy(1);
			//Test[6] = Ade7953_getEnergy(2);
			for(i=0;i<63;i++){
				vars[i].auto_manual = 0;
				//newAde7953Read()
			}*/
			//uint8 tempI2cBuf[4];
			for(i=0; i<49; i++)
			{
				//newAde7953Read(0x200+i, (uint8_t *)&vars[i].value);
				vars[i].value = Ade7953Read(0x200+i);
			}
			for(i=0; i<24;i++)
			{
				//newAde7953Read(0x280+i, (uint8_t *)&vars[i+49].value);
				vars[i+49].value = Ade7953Read(0x280+i);
			}
			memcpy(vars[0].description,"SAG VOLTAGE LEVEL",strlen("SAG VOLTAGE LEVEL"));
			memcpy(vars[0].label,"SAGLVL",strlen("SAGLVL"));
			//vars[0].value = 136;
			memcpy(vars[1].description,"ACCUMULATION MODE",strlen("ACCUMULATION MODE"));
			memcpy(vars[1].label,"ACCMODE",strlen("ACCMODE"));
			memcpy(vars[2].description,"ACTIVE PW NO LOAD LV",strlen("ACTIVE PW NO LOAD LV"));
			memcpy(vars[2].label,"APNOLOAD",strlen("APNOLOAD"));
			memcpy(vars[3].description,"Reactive PW no-load LV",strlen("Reactive PW no-load LV"));
			memcpy(vars[3].label,"VARNOLD",strlen("VARNOLD"));
			memcpy(vars[4].description,"Apparent PW no-load level",strlen("Apparent PW no-load level"));
			memcpy(vars[4].label,"VANOLD",strlen("VANOLD"));
			memcpy(vars[5].description,"Instantaneous Apparent Power A",strlen("Instantaneous Apparent Power"));
			memcpy(vars[5].label,"AVA",strlen("AVA"));
			memcpy(vars[6].description,"Instantaneous Apparent Power B",strlen("Instantaneous Apparent Power"));
			memcpy(vars[6].label,"AVB",strlen("AVB"));
			memcpy(vars[7].description,"Instantaneous Active Power",strlen("Instantaneous Active Power"));
			memcpy(vars[7].label,"AWATT",strlen("AWATT"));
			memcpy(vars[8].description,"Instantaneous Active Power",strlen("Instantaneous Active Power"));
			memcpy(vars[8].label,"BWATT",strlen("BWATT"));
			memcpy(vars[9].description,"Instantaneous Reactive Power",strlen("Instantaneous Reactive Power"));
			memcpy(vars[9].label,"AVAR",strlen("AVAR"));
			memcpy(vars[10].description,"Instantaneous Reactive Power",strlen("Instantaneous Reactive Power"));
			memcpy(vars[10].label,"BVAR",strlen("BVAR"));
			memcpy(vars[11].description,"Instantaneous Current",strlen("Instantaneous Current"));
			memcpy(vars[11].label,"IA",strlen("IA"));
			memcpy(vars[12].description,"Instantaneous Current",strlen("Instantaneous Current"));
			memcpy(vars[12].label,"IB",strlen("IB"));
			memcpy(vars[13].description,"Instantaneous Voltage",strlen("Instantaneous Voltage"));
			memcpy(vars[13].label,"VOLT",strlen("VOLT"));
			memcpy(vars[14].description,"IRMSA register",strlen("IRMSA register"));
			memcpy(vars[14].label,"IRMSA",strlen("IRMSA"));
			memcpy(vars[15].description,"IRMSB register",strlen("IRMSB register"));
			memcpy(vars[15].label,"IRMSB",strlen("IRMSB"));
			memcpy(vars[16].description,"VRMS register",strlen("VRMS register"));
			memcpy(vars[16].label,"VRMS",strlen("VRMS"));
			memcpy(vars[17].description,"Active Energy A",strlen("Active Energy A"));
			memcpy(vars[17].label,"AENGA",strlen("AENGA"));
			memcpy(vars[18].description,"Active Energy B",strlen("Active Energy B"));
			memcpy(vars[18].label,"AENGB",strlen("AENGB"));
			memcpy(vars[19].description,"Reactive Energy A",strlen("Reactive Energy A"));
			memcpy(vars[19].label,"RENGA",strlen("RENGA"));
			memcpy(vars[20].description,"Reactive Energy B",strlen("Reactive Energy B"));
			memcpy(vars[20].label,"RENGB",strlen("RENGB"));
			memcpy(vars[21].description,"Apparent Energy A",strlen("Apparent Energy A"));
			memcpy(vars[21].label,"APENGA",strlen("APENGA"));
			memcpy(vars[22].description,"Apparent Energy B",strlen("Apparent Energy B"));
			memcpy(vars[22].label,"APENGB",strlen("APENGB"));
			memcpy(vars[23].description,"Overvolatage Level",strlen("Overvolatage Level"));
			memcpy(vars[23].label,"OVLVL",strlen("OVLVL"));
			memcpy(vars[24].description,"Overcurrent Level",strlen("Overcurrent Level"));
			memcpy(vars[24].label,"OILVL",strlen("OILVL"));
			memcpy(vars[25].description,"Voltage Channel Peak ",strlen("Voltage Channel Peak "));
			memcpy(vars[25].label,"VPEAK",strlen("VPEAK"));
			memcpy(vars[26].description,"Read Voltage Peak With Reset",strlen("Read Voltage Peak With Reset"));
			memcpy(vars[26].label,"RSTVPEAK",strlen("RSTVPEAK"));
			memcpy(vars[27].description,"Current Channel A Peak",strlen("Current Channel A Peak"));
			memcpy(vars[27].label,"IAPEAK",strlen("IAPEAK"));
			memcpy(vars[28].description,"Read Current Channel A Peak With Reset",strlen("Read Current Channel A Peak With Reset"));
			memcpy(vars[28].label,"RSTIAPEAK",strlen("RSTIAPEAK"));
			memcpy(vars[29].description,"Current Channel B Peak",strlen("Current Channel A Peak"));
			memcpy(vars[29].label,"IBPEAK",strlen("IBPEAK"));
			memcpy(vars[30].description,"Read Current Channel B Peak With Reset",strlen("Read Current Channel A Peak With Reset"));
			memcpy(vars[30].label,"RSTIBPEAK",strlen("RSTIBPEAK"));
			memcpy(vars[31].description,"Interrupt Enable A",strlen("Interrupt Enable A"));
			memcpy(vars[31].label,"IRQENA",strlen("IRQENA"));
			memcpy(vars[32].description,"Interrupt Status A",strlen("Interrupt Status A"));
			memcpy(vars[32].label,"ISTATA",strlen("ISTATA"));
			memcpy(vars[33].description,"Reset Interrupt Status A",strlen("Reset Interrupt Status A"));
			memcpy(vars[33].label,"RSTISA",strlen("RSTISA"));
			memcpy(vars[34].description,"Interrupt Enable B",strlen("Interrupt Enable A"));
			memcpy(vars[34].label,"IRQENB",strlen("IRQENB"));
			memcpy(vars[35].description,"Interrupt Status B",strlen("Interrupt Status A"));
			memcpy(vars[35].label,"ISTATB",strlen("ISTATB"));
			memcpy(vars[36].description,"Reset Interrupt Status B",strlen("Reset Interrupt Status B"));
			memcpy(vars[36].label,"RSTISB",strlen("RSTISB"));
			memcpy(vars[37].description,"Checksum",strlen("Checksum"));
			memcpy(vars[37].label,"CRC",strlen("CRC"));
			memcpy(vars[38].description,"Current Channel Gain A",strlen("Current Channel Gain A"));
			memcpy(vars[38].label,"IAGAIN",strlen("IAGAIN"));
			memcpy(vars[39].description,"Voltage Channel Gain A",strlen("Voltage Channel Gain A"));
			memcpy(vars[39].label,"VGAIN",strlen("VGAIN"));
			memcpy(vars[40].description,"Active Power Gain A",strlen("Active Power Gain A"));
			memcpy(vars[40].label,"AWGAIN",strlen("AWGAIN"));
			memcpy(vars[41].description,"Reactive Power Gain A",strlen("Reactive Power Gain A"));
			memcpy(vars[41].label,"AVARG",strlen("AVARG"));
			memcpy(vars[42].description,"Apparent Power Gain A",strlen("Apparent Power Gain A"));
			memcpy(vars[42].label,"AVAG",strlen("AVAG"));
			memcpy(vars[43].description,"IRMS Offset A",strlen("IRMS Offset A"));
			memcpy(vars[43].label,"AIRMSOS",strlen("AIRMSOS"));
			memcpy(vars[44].description,"VRMS Offset",strlen("VRMS Offset"));
			memcpy(vars[44].label,"VRMSOS",strlen("VRMSOS"));
			memcpy(vars[45].description,"Active Power Offset Correction A",strlen("Active Power Offset Correction A"));
			memcpy(vars[45].label,"AWATTOS",strlen("AWATTOS"));
			memcpy(vars[46].description,"Reactive Power Offset Correction A",strlen("Reactive Power Offset Correction A"));
			memcpy(vars[46].label,"AVAROS",strlen("AVAROS"));
			memcpy(vars[47].description,"Apparent Power Offset Correction A",strlen("Apparent Power Offset Correction A"));
			memcpy(vars[47].label,"AVAOS",strlen("AVAOS"));
			memcpy(vars[48].description,"Voltage Channel Gain B",strlen("Voltage Channel Gain B"));
			memcpy(vars[48].label,"VGBIN",strlen("VGBIN"));
			memcpy(vars[49].description,"Active Power Gain B",strlen("Active Power Gain B"));
			memcpy(vars[49].label,"BWGBIN",strlen("BWGBIN"));
			memcpy(vars[50].description,"Reactive Power Gain B",strlen("Reactive Power Gain B"));
			memcpy(vars[50].label,"BVBRG",strlen("BVBRG"));
			memcpy(vars[51].description,"Apparent Power Gain B",strlen("Apparent Power Gain B"));
			memcpy(vars[51].label,"BVBG",strlen("BVBG"));
			memcpy(vars[52].description,"IRMS Offset B",strlen("IRMS Offset B"));
			memcpy(vars[52].label,"BIRMSOS",strlen("BIRMSOS"));
			memcpy(vars[53].description,"Active Power Offset Correction B",strlen("Active Power Offset Correction B"));
			memcpy(vars[53].label,"BWBTTOS",strlen("BWBTTOS"));
			memcpy(vars[54].description,"Reactive Power Offset Correction B",strlen("Reactive Power Offset Correction B"));
			memcpy(vars[54].label,"BVBROS",strlen("BVBROS"));
			memcpy(vars[55].description,"Apparent Power Offset Correction B",strlen("Apparent Power Offset Correction B"));
			memcpy(vars[55].label,"BVBOS",strlen("BVBOS"));

			memcpy(vars[56].description,"VOLTAGE FACTOR",strlen("VOLTAGE FACTOR"));
			vars[56].value = 136;
			memcpy(vars[57].description,"CURRENT1 FACTOR",strlen("CURRENT1 FACTOR"));
			vars[57].value = 1318;
			memcpy(vars[58].description,"CURRENT2 FACTOR",strlen("CURRENT1 FACTOR"));
			vars[58].value = 1318;
			memcpy(vars[59].description,"POWER1 FACTOR",strlen("POWER1 FACTOR"));
			vars[59].value = 164;
			memcpy(vars[60].description,"POWER2 FACTOR",strlen("POWER2 FACTOR"));
			vars[60].value = 164;
			memcpy(vars[61].description,"ENERGY1 FACTOR",strlen("ENERGY1 FACTOR"));
			vars[61].value = 25240;
			memcpy(vars[62].description,"ENERGY2 FACTOR",strlen("ENERGY1 FACTOR"));
			vars[62].value = 25240;
			memcpy(inputs[0].description,"VOLTAGE",strlen("VOLTAGE"));
			memcpy(inputs[1].description,"CURRENT1",strlen("CURRENT1"));
			memcpy(inputs[2].description,"CURRENT2",strlen("CURRENT1"));
			memcpy(inputs[3].description,"POWER1",strlen("POWER1"));
			memcpy(inputs[4].description,"POWER2",strlen("POWER1"));
			memcpy(inputs[5].description,"ENERGY1",strlen("ENERGY1"));
			memcpy(inputs[6].description,"ENERGY1",strlen("ENERGY1"));
		}
		if(Modbus.mini_type == PROJECT_FAN_MODULE)
		{
			ret = i2c_master_sensor_sht31(I2C_MASTER_NUM, sht31_data);//&sensor_data_h, &sensor_data_l);

			/*memcpy(inputs[0].description,"TEMP ON BOARD",strlen("TEMP ON BOARD"));
			memcpy(inputs[1].description,"HUMIDITY",strlen("HUMIDITY"));
			{
				memcpy(inputs[2].description,"TEMP REMOTE",strlen("TEMP REMOTE"));
			}
			{
				memcpy(inputs[3].description,"FAN STATUS",strlen("FAN STATUS"));
			}
			{
				memcpy(inputs[4].description,"FAN SPEED",strlen("FAN SPEED"));
			}
			memcpy(inputs[5].description,"THERMEL TEMP",strlen("THERMEL TEMP"));
			memcpy(outputs[0].description,"FAN AO",strlen("FAN AO"));
			memcpy(outputs[1].description,"FAN PWM",strlen("FAN PWM"));
			//memcpy(inputs[4].description,"RPM",strlen("RPM"));*/
			xSemaphoreTake(print_mux, portMAX_DELAY);
			if (ret == ESP_ERR_TIMEOUT) {
				ESP_LOGE(TAG, "I2C Timeout");
				//Test[0] = 120;
			} else if (ret == ESP_OK) {
				g_sensors.original_temperature = SHT3X_getTemperature(sht31_data);
				g_sensors.original_humidity = SHT3X_getHumidity(&sht31_data[3]);
				g_sensors.temperature = (uint16_t)(g_sensors.original_temperature*10);
				g_sensors.humidity = (uint16_t)(g_sensors.original_humidity*10);
				if(!inputs[0].calibration_sign)
					g_sensors.temperature += (inputs[0].calibration_hi * 256 + inputs[0].calibration_lo);
				else
					g_sensors.temperature += -(inputs[0].calibration_hi * 256 + inputs[0].calibration_lo);
				if(!inputs[1].calibration_sign)
					g_sensors.humidity += (inputs[1].calibration_hi * 256 + inputs[1].calibration_lo);
				else
					g_sensors.humidity += -(inputs[1].calibration_hi * 256 + inputs[1].calibration_lo);
				if(inputs[0].range == 3)
					inputs[0].value = g_sensors.temperature*100;
				if(inputs[0].range == 4)
					inputs[0].value = (g_sensors.temperature*9/5)*100+32000;
				inputs[1].value = g_sensors.humidity*100;
				//Test[21]++;
				//Test[22] = g_sensors.humidity;
				//g_sensors.temperature = Filter(0,g_sensors.temperature);
				//g_sensors.humidity = Filter(9,g_sensors.humidity);
				//g_sensors.temperature += holding_reg_params.sht31_temp_offset;
				//printf("sensor val: %.02f [Lux]\n", (sensor_data_h << 8 | sensor_data_l) / 1.2);
			} else {//Test[23]++;
				//ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...", esp_err_to_name(ret));
				//Test[1] = ret;
			}
			xSemaphoreGive(print_mux);
		}
#endif
        vTaskDelay(100 / portTICK_RATE_MS);
        if(Modbus.mini_type == PROJECT_TRANSDUCER)
        {
			xSemaphoreTake(print_mux, portMAX_DELAY);
			scd4x_start_periodic_measurement();
			vTaskDelay(100 / portTICK_RATE_MS);
			scd4x_read_measurement(&g_sensors.co2, &g_sensors.co2_temp, &g_sensors.co2_humi);
			xSemaphoreGive(print_mux);

			xSemaphoreTake(print_mux, portMAX_DELAY);
			ret = sht4x_measure_blocking_read( &sht4x_temp, &sht4x_hum);
			if(ret != ESP_OK)
			{

			}
			else
			{
				g_sensors.temperature = (sht4x_temp)/100;
				g_sensors.humidity = (sht4x_hum)/100;
				Test[0] = g_sensors.temperature;
				Test[1] = g_sensors.humidity;
				Test[2] = g_sensors.co2;
			}
			xSemaphoreGive(print_mux);
        }
#if 0
		xSemaphoreTake(print_mux, portMAX_DELAY);
        ret = i2c_master_sensor_veml7700(I2C_MASTER_NUM,&light_data[0], &light_data[1]);
		if (ret == ESP_ERR_TIMEOUT) {
			ESP_LOGE(TAG, "I2C LIGHT Timeout");
		} else if (ret == ESP_OK) {
			// printf("*******************\n");
			// printf("TASK[%d]  MASTER READ SENSOR( VEML7700 )\n", task_idx);
			// printf("*******************\n");
			// printf("data_h: %02x\n", light_data[1]);//sensor_data_h);
			// printf("data_l: %02x\n", light_data[0]);//sensor_data_l);
			g_sensors.light_value = ((uint16_t)light_data[1]<<8)+light_data[0];
			//g_sensors.light_value = Filter(8,g_sensors.light_value);
		}else {
			ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...", esp_err_to_name(ret));
		}
		xSemaphoreGive(print_mux);
		vTaskDelay(100 / portTICK_RATE_MS);
        /*ret =i2c_master_sensor_mlx90621(I2C_MASTER_NUM,0x24,0x05,&temp_data[0], &temp_data[1]);
        xSemaphoreTake(print_mux, portMAX_DELAY);
		if (ret == ESP_ERR_TIMEOUT) {
			ESP_LOGE(TAG, "I2C MLX90632 Timeout");
		} else if (ret == ESP_OK) {
			//printf("*******************\n");
			//printf("TASK[%d]  MASTER READ SENSOR( MLX90621 )\n", task_idx);
			//printf("*******************\n");
			//printf("data_h: %02x\n", temp_data[0]);//sensor_data_h);
			//printf("data_l: %02x\n", temp_data[1]);//sensor_data_l);
			g_sensors.infrared_temp1 = ((uint16_t)temp_data[0]<<8)+temp_data[1];
		}else {
			ESP_LOGW(TAG, "%s: No ack, MLX90632 sensor not connected...skip...", esp_err_to_name(ret));
		}*/
		/* Read sensor EEPROM registers needed for calcualtions */

		/* Now we read current ambient and object temperature */
		if(which_project == PROJECT_SAUTER){
			xSemaphoreTake(print_mux, portMAX_DELAY);
			ret = mlx90632_read_temp_raw(&ambient_new_raw, &ambient_old_raw,
										 &object_new_raw, &object_old_raw);
			/* Now start calculations (no more i2c accesses) */
			/* Calculate ambient temperature */
			ambient = mlx90632_calc_temp_ambient(ambient_new_raw, ambient_old_raw,
												 P_T, P_R, P_G, P_O, Gb);

			/* Get preprocessed temperatures needed for object temperature calculation */
			double pre_ambient = mlx90632_preprocess_temp_ambient(ambient_new_raw,
																  ambient_old_raw, Gb);
			double pre_object = mlx90632_preprocess_temp_object(object_new_raw, object_old_raw,
																ambient_new_raw, ambient_old_raw,
																Ka);
			/* Calculate object temperature */
			object = mlx90632_calc_temp_object(pre_object, pre_ambient, Ea, Eb, Ga, Fa, Fb, Ha, Hb);
			g_sensors.ambient = (uint16_t)(ambient*10)/2;
			g_sensors.object = (uint16_t )(object*10)/2;
			//g_sensors.infrared_temp1 = 321;
			xSemaphoreGive(print_mux);
			vTaskDelay(100/ portTICK_RATE_MS);
		}

		//-----------------tVOC
		xSemaphoreTake(print_mux, portMAX_DELAY);
		/*ret = sensirion_i2c_delayed_read_cmd(
				SGP30_SENSOR_ADDR, sgp30_cmd_get_serial_id,
		        SGP_CMD_GET_SERIAL_ID_DURATION_US, client_data.buffer.words,
		        SGP_CMD_GET_SERIAL_ID_WORDS);*/
		/*if(sgp30_measure_tvoc() == ESP_OK)
		{
			vTaskDelay(100/ portTICK_RATE_MS);
		}*/
		if(voc_ok)
		{
			if(sgp30_measure_tvoc() == STATUS_OK)
			{
				vTaskDelay(100 / portTICK_RATE_MS);
				if( sgp30_read_tvoc(&g_sensors.tvoc_ppb) == STATUS_OK)
				{
				  if(voc_cnt<4)
						voc_buf[voc_cnt++] = g_sensors.tvoc_ppb;
					else if(voc_cnt == 4)
					{
						voc_buf[voc_cnt] = g_sensors.tvoc_ppb;
						voc_cnt = 0;
					}
					temp = 0;
					for(int i=0;i<5;i++)
						temp += voc_buf[i];

					g_sensors.voc_value = temp/5;
					//g_sensors.voc_value = Filter(7,g_sensors.voc_value);
				 //TXEN = SEND;
				//	printf("tVOC  Concentration: %dppb\n", tvoc_ppb);
				 //TXEN = RECEIVE;
				}
			}


		// Persist the current baseline every hour
		if (++baseline_time % 3600 == 3599)
		{
			ret = sgp30_get_iaq_baseline(&iaq_baseline);
			if (ret == STATUS_OK)
			{
			  //write_eeprom(EEP_BASELINE1, iaq_baseline & 0XFF);  // IMPLEMENT: store baseline to presistent storage
			  //write_eeprom(EEP_BASELINE2, (iaq_baseline>>8) & 0XFF);
			  //write_eeprom(EEP_BASELINE3, (iaq_baseline>>16) & 0XFF);
			  //write_eeprom(EEP_BASELINE4, (iaq_baseline>>24) & 0XFF);
			  g_sensors.voc_baseline[0] = iaq_baseline & 0XFF;
			  g_sensors.voc_baseline[1] = (iaq_baseline>>8)& 0XFF;
			  g_sensors.voc_baseline[2] = (iaq_baseline>>16) & 0XFF;
			  g_sensors.voc_baseline[3] = (iaq_baseline>>24) & 0XFF;
     		}
		}

			if(g_sensors.voc_ini_baseline == 1)
			{
				ret = sgp30_iaq_init();
				if (ret == STATUS_OK) {
					g_sensors.voc_ini_baseline = 0;
					printf("sgp30_iaq_init done\n");
				} else
					printf("sgp30_iaq_init failed!\n");
			}
	  }
		xSemaphoreGive(print_mux);
		vTaskDelay(500/portTICK_RATE_MS);
		//------------------SCD40

		xSemaphoreTake(print_mux, portMAX_DELAY);
		//ret = i2c_master_sensor_scd40(I2C_MASTER_NUM,0x36,0x82,scd40_data);
		//ret = sensirion_i2c_delayed_read_cmd(SCD40_SENSOR_ADDR, 0x3682,	200, scd40_data, 3);
		ret = ESP_OK;
		if (ret == ESP_ERR_TIMEOUT) {
			ESP_LOGE(TAG, "I2C Timeout");
		} else if (ret == ESP_OK) {
			if(g_sensors.co2_start_measure != true)
			{
				sensirion_i2c_write_cmd(SCD40_SENSOR_ADDR, 0x3608);
				g_sensors.co2_start_measure = true;
			}
			vTaskDelay(1000/portTICK_RATE_MS);
			ret = sensirion_i2c_delayed_read_cmd(
							SCD40_SENSOR_ADDR, 0xEC05,
							2000, scd40_data,
							3);
			if(ret == ESP_OK)
			{
				//sensirion_i2c_write_cmd(SCD40_SENSOR_ADDR, 0x3F86);
				//g_sensors.co2 = scd40_data[0];//(uint16_t)(scd40_data[0]<<8) + scd40_data[1];
				g_sensors.co2 = BUILD_UINT16(tempBuf_CO2[1],tempBuf_CO2[0]);//(uint16_t)(tempBuf_CO2[0]<<8) + tempBuf_CO2[1];//tempBuf_CO2
				//g_sensors.co2 = Filter(2, g_sensors.co2);
			}
			// printf("*******************\n");
			// printf("TASK[%d]  MASTER READ SENSOR( SCD40 )\n", task_idx);
			// printf("*******************\n");
			// printf("data_1: %02x\n", tempBuf_CO2[0]);//sensor_data_h);
			// printf("data_2: %02x\n", tempBuf_CO2[1]);//sensor_data_l);
			// printf("data_3: %02x\n", scd40_data[2]);//sensor_data_h);
			// printf("data_4: %02x\n", scd40_data[3]);//sensor_data_l);
			// printf("data_5: %02x\n", scd40_data[4]);//sensor_data_h);
			// printf("data_6: %02x\n", scd40_data[5]);//sensor_data_l);
			//printf("sensor val: %.02f [Lux]\n", (sensor_data_h << 8 | sensor_data_l) / 1.2);
			//uart_write_bytes(0, "\r\n", 2);
			//uart_write_bytes(0, (const char*)&g_sensors.co2, 2);
			//uart_write_bytes(0, (const char*)&tempBuf_CO2[0], 1);
			//uart_write_bytes(0, (const char*)&tempBuf_CO2[1], 1);
		} else {
			// ESP_LOGW(TAG, "%s: No ack, scd40 sensor not connected...skip...", esp_err_to_name(ret));
		}
		xSemaphoreGive(print_mux);
		vTaskDelay(DELAY_TIME_BETWEEN_ITEMS_MS / portTICK_RATE_MS);
#endif
		//if(which_project == PROJECT_FAN_MODULE)
		/*{
			xSemaphoreTake(print_mux, portMAX_DELAY);
			//ret = i2c_master_sensor_mlx90614(I2C_MASTER_NUM, mlx90614_data);
			MLX90614_GetTa(MLX90614_SENSOR_ADDR, &mlx90614_ambient);
			MLX90614_GetTo(MLX90614_SENSOR_ADDR, &mlx90614_object);
			g_sensors.ambient = (uint16_t)(mlx90614_ambient*10);
			g_sensors.object = (uint16_t )(mlx90614_object*10);
			//g_sensors.ambient += holding_reg_params.ambient_temp_offset;
			//g_sensors.object += holding_reg_params.object_temp_offset;
			xSemaphoreGive(print_mux);
			vTaskDelay(DELAY_TIME_BETWEEN_ITEMS_MS / portTICK_RATE_MS);
		}*/
#if 0
		//vTaskDelay(1000/portTICK_RATE_MS);

		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, SCD40_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, 0x36,ACK_CHECK_EN);
		i2c_master_write_byte(cmd, 0x08,ACK_CHECK_EN);
		//i2c_master_stop(cmd);
		//vTaskDelay(2100/portTICK_RATE_MS);
		//i2c_master_start(cmd);
		//i2c_master_write_byte(cmd, SCD40_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
		//i2c_master_read(cmd,scd40_data,9,ACK_VAL);
		ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
		i2c_cmd_link_delete(cmd);
		vTaskDelay(2050/portTICK_RATE_MS);
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, SCD40_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
		i2c_master_read(cmd,scd40_data,8,ACK_VAL);
		i2c_master_read(cmd,&scd40_data[8],1,NACK_VAL);
		i2c_master_stop(cmd);
		ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
		i2c_cmd_link_delete(cmd);
        //vTaskDelay(DELAY_TIME_BETWEEN_ITEMS_MS / portTICK_RATE_MS);
		vTaskDelay(5000 / portTICK_RATE_MS);
#endif
        //---------------------------------------------------

    }
    vSemaphoreDelete(print_mux);
    vTaskDelete(NULL);
}
