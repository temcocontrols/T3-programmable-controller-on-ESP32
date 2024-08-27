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
#include "sht3x.h"
#include "airlab.h"

static const char *TAG = "i2c-task";

//SemaphoreHandle_t print_mux = NULL;
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
uint8_t co2_present = 0;
uint8_t scd4x_perform_forced = 0;
uint16_t co2_asc = 0;
uint16_t co2_frc = 0;

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
	i2c_master_write_byte(cmd, (SCD40_SENSOR_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_h,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_l,ACK_CHECK_EN);
	vTaskDelay(500/portTICK_RATE_MS);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (SCD40_SENSOR_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
	i2c_master_read(cmd,data,9,ACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}

esp_err_t sensirion_i2c_write(uint8_t address, const uint8_t *data,
                           uint16_t count) {
    int ret = 0;
    uint16_t i;


    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    ret = i2c_master_write_byte(cmd,address << 1| WRITE_BIT, ACK_CHECK_EN);
    for (i = 0; i < count; i++) {
        ret = i2c_master_write_byte(cmd, data[i],ACK_CHECK_EN);
    }
    ret = i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 5000 / portTICK_RATE_MS);
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
    vTaskDelay(5/portTICK_RATE_MS);
    for (i = 0; i < count; i++) {
		send_ack = i < (count - 1); // last byte must be NACK'ed
		i2c_master_read_byte(cmd, &data[i],!send_ack);
	}
    i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 2000 / portTICK_RATE_MS);
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

/*
void sensirion_sleep_usec(uint32_t useconds) {
	//delay_us(useconds);
	usleep(useconds);
    //HAL_Delay(useconds / 1000 + 1);
}*/
/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init()
{
//	print_mux = xSemaphoreCreateMutex();
    int i2c_master_port = I2C_MASTER_NUM;
    static i2c_config_t conf;
    Str_points_ptr ptr;
    conf.mode = I2C_MODE_MASTER;
    if((Modbus.mini_type == PROJECT_FAN_MODULE)||(Modbus.mini_type == PROJECT_TRANSDUCER)||((Modbus.mini_type == PROJECT_POWER_METER)))
    	conf.sda_io_num = 12;//4;//I2C_MASTER_SDA_IO;
    else  // PROJECT_LIGHT_SWITCH
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
    ptr = put_io_buf(IN,0);
    memcpy(ptr.pin->label,"TEMP",strlen("TEMP"));
    memcpy(ptr.pin->description,"Temperature",strlen("Temperature"));
}

uint8_t hum_sensor_type = 0; // SHT3X exist
float tem_org = 0;
float hum_org = 0;
// ------- for airlab, i2c sensor initial
void SHT3X_Inital(void)  // humidity & temp
{
	SHT3X_Init(0x44);
	for(u8 i=0;i<10;i++)
	{
		if(SHT3X_GetTempAndHumi(&tem_org, &hum_org, REPEATAB_HIGH, MODE_POLLING, 50) == 0)
		hum_sensor_type = 1;
	}
	if(hum_sensor_type != 1)
		hum_sensor_type = 0;
}

void SCD4X_Inital(void) // humdity , temperature and co2
{

}

uint8 voc_ok = 0;
uint32_t iaq_baseline;
uint8_t flag_voc_init;
uint8_t count_voc_int;

void VOC_Initial(void) 	// SGP30
{
	int16_t ret;
	g_sensors.voc_baseline[0] = 0;
	g_sensors.voc_baseline[1] = 0;
	g_sensors.voc_baseline[2] = 0;
	g_sensors.voc_baseline[3] = 0;
	//g_sensors.ambient = 100;
	//g_sensors.object = 10;
	uint16_t voc_buf[5];
	static uint8_t voc_cnt;
	static uint16_t baseline_time = 0;

	voc_ok = 1;
	voc_cnt = 0;
	u8 temp = 0;
	while (sgp30_probe() != STATUS_OK) {
		temp++;
		if(temp > 20)
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
}


void Refresh_SCD40(void);
void I2C_sensor_Init(void);
//-------end --- for airlab, i2c sensor initial
void i2c_sensor_task(void *arg)
{
    int ret,i;
    uint16 voc_buf[5] = {0,0,0,0,0};
    uint8 voc_cnt = 0;
    Str_points_ptr ptr;
    uint16 baseline_time = 0;
	uint32_t temp;
	uint32_t iaq_baseline;
    uint32_t task_idx = (uint32_t)arg;
    int16_t ambient_new_raw=0;
    int16_t ambient_old_raw=0;
    int16_t object_new_raw=0;
    int16_t object_old_raw=0;
    int32_t sht4x_temp, sht4x_hum;

    I2C_sensor_Init();

    g_sensors.co2_start_measure = false;
//    uint8_t sensor_data_h, sensor_data_l;
    int cnt = 0;
    g_sensors.co2_ready = false;

   if( (Modbus.mini_type == PROJECT_AIRLAB) || (Modbus.mini_type == PROJECT_LIGHT_SWITCH))
    {
    	//voc_value_raw = 0;
    	VOC_Initial();
    	// check SCD40
    	co2_present = 1;

    }

#if 0  //????????????
	if(Modbus.mini_type == PROJECT_SAUTER){
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

	while (1) {

		if(Modbus.mini_type == PROJECT_POWER_METER)
		{
			Ade7953GetData();

			for(i=0; i<49; i++)
			{
				ptr = put_io_buf(VAR,i);
				//newAde7953Read(0x200+i, (uint8_t *)&vars[i].value);
				ptr.pvar->value = Ade7953Read(0x200+i);
			}
			for(i=0; i<24;i++)
			{
				ptr = put_io_buf(VAR,i + 49);
				//newAde7953Read(0x280+i, (uint8_t *)&vars[i+49].value);
				ptr.pvar->value = Ade7953Read(0x280+i);
			}
			ptr = put_io_buf(VAR,0);
			memcpy(ptr.pin->description,"SAG VOLTAGE LEVEL",strlen("SAG VOLTAGE LEVEL"));
			memcpy(ptr.pin->label,"SAGLVL",strlen("SAGLVL"));
			ptr = put_io_buf(VAR,1);
			memcpy(ptr.pin->description,"ACCUMULATION MODE",strlen("ACCUMULATION MODE"));
			memcpy(ptr.pin->label,"ACCMODE",strlen("ACCMODE"));
			ptr = put_io_buf(VAR,2);
			memcpy(ptr.pin->description,"ACTIVE PW NO LOAD LV",strlen("ACTIVE PW NO LOAD LV"));
			memcpy(ptr.pin->label,"APNOLOAD",strlen("APNOLOAD"));
			ptr = put_io_buf(VAR,3);
			memcpy(ptr.pin->description,"Reactive PW no-load LV",strlen("Reactive PW no-load LV"));
			memcpy(ptr.pin->label,"VARNOLD",strlen("VARNOLD"));
			ptr = put_io_buf(VAR,4);
			memcpy(ptr.pin->description,"Apparent PW no-load level",strlen("Apparent PW no-load level"));
			memcpy(ptr.pin->label,"VANOLD",strlen("VANOLD"));
			ptr = put_io_buf(VAR,5);
			memcpy(ptr.pin->description,"Instantaneous Apparent Power A",strlen("Instantaneous Apparent Power"));
			memcpy(ptr.pin->label,"AVA",strlen("AVA"));
			ptr = put_io_buf(VAR,6);
			memcpy(ptr.pin->description,"Instantaneous Apparent Power B",strlen("Instantaneous Apparent Power"));
			memcpy(ptr.pin->label,"AVB",strlen("AVB"));
			ptr = put_io_buf(VAR,7);
			memcpy(ptr.pin->description,"Instantaneous Active Power",strlen("Instantaneous Active Power"));
			memcpy(ptr.pin->label,"AWATT",strlen("AWATT"));
			ptr = put_io_buf(VAR,8);
			memcpy(ptr.pin->description,"Instantaneous Active Power",strlen("Instantaneous Active Power"));
			memcpy(ptr.pin->label,"BWATT",strlen("BWATT"));
			ptr = put_io_buf(VAR,9);
			memcpy(ptr.pin->description,"Instantaneous Reactive Power",strlen("Instantaneous Reactive Power"));
			memcpy(ptr.pin->label,"AVAR",strlen("AVAR"));
			ptr = put_io_buf(VAR,10);
			memcpy(ptr.pin->description,"Instantaneous Reactive Power",strlen("Instantaneous Reactive Power"));
			memcpy(ptr.pin->label,"BVAR",strlen("BVAR"));
			ptr = put_io_buf(VAR,11);
			memcpy(ptr.pin->description,"Instantaneous Current",strlen("Instantaneous Current"));
			memcpy(ptr.pin->label,"IA",strlen("IA"));
			ptr = put_io_buf(VAR,12);
			memcpy(ptr.pin->description,"Instantaneous Current",strlen("Instantaneous Current"));
			memcpy(ptr.pin->label,"IB",strlen("IB"));
			ptr = put_io_buf(VAR,13);
			memcpy(ptr.pin->description,"Instantaneous Voltage",strlen("Instantaneous Voltage"));
			memcpy(ptr.pin->label,"VOLT",strlen("VOLT"));
			ptr = put_io_buf(VAR,14);
			memcpy(ptr.pin->description,"IRMSA register",strlen("IRMSA register"));
			memcpy(ptr.pin->label,"IRMSA",strlen("IRMSA"));
			ptr = put_io_buf(VAR,15);
			memcpy(ptr.pin->description,"IRMSB register",strlen("IRMSB register"));
			memcpy(ptr.pin->label,"IRMSB",strlen("IRMSB"));
			ptr = put_io_buf(VAR,16);
			memcpy(ptr.pin->description,"VRMS register",strlen("VRMS register"));
			memcpy(ptr.pin->label,"VRMS",strlen("VRMS"));
			ptr = put_io_buf(VAR,17);
			memcpy(ptr.pin->description,"Active Energy A",strlen("Active Energy A"));
			memcpy(ptr.pin->label,"AENGA",strlen("AENGA"));
			ptr = put_io_buf(VAR,18);
			memcpy(ptr.pin->description,"Active Energy B",strlen("Active Energy B"));
			memcpy(ptr.pin->label,"AENGB",strlen("AENGB"));
			ptr = put_io_buf(VAR,19);
			memcpy(ptr.pin->description,"Reactive Energy A",strlen("Reactive Energy A"));
			memcpy(ptr.pin->label,"RENGA",strlen("RENGA"));
			ptr = put_io_buf(VAR,20);
			memcpy(ptr.pin->description,"Reactive Energy B",strlen("Reactive Energy B"));
			memcpy(ptr.pin->label,"RENGB",strlen("RENGB"));
			ptr = put_io_buf(VAR,21);
			memcpy(ptr.pin->description,"Apparent Energy A",strlen("Apparent Energy A"));
			memcpy(ptr.pin->label,"APENGA",strlen("APENGA"));
			ptr = put_io_buf(VAR,22);
			memcpy(ptr.pin->description,"Apparent Energy B",strlen("Apparent Energy B"));
			memcpy(ptr.pin->label,"APENGB",strlen("APENGB"));
			ptr = put_io_buf(VAR,23);
			memcpy(ptr.pin->description,"Overvolatage Level",strlen("Overvolatage Level"));
			memcpy(ptr.pin->label,"OVLVL",strlen("OVLVL"));
			ptr = put_io_buf(VAR,24);
			memcpy(ptr.pin->description,"Overcurrent Level",strlen("Overcurrent Level"));
			memcpy(ptr.pin->label,"OILVL",strlen("OILVL"));
			ptr = put_io_buf(VAR,25);
			memcpy(ptr.pin->description,"Voltage Channel Peak ",strlen("Voltage Channel Peak "));
			memcpy(ptr.pin->label,"VPEAK",strlen("VPEAK"));
			ptr = put_io_buf(VAR,26);
			memcpy(ptr.pin->description,"Read Voltage Peak With Reset",strlen("Read Voltage Peak With Reset"));
			memcpy(ptr.pin->label,"RSTVPEAK",strlen("RSTVPEAK"));
			ptr = put_io_buf(VAR,27);
			memcpy(ptr.pin->description,"Current Channel A Peak",strlen("Current Channel A Peak"));
			memcpy(ptr.pin->label,"IAPEAK",strlen("IAPEAK"));
			ptr = put_io_buf(VAR,28);
			memcpy(ptr.pin->description,"Read Current Channel A Peak With Reset",strlen("Read Current Channel A Peak With Reset"));
			memcpy(ptr.pin->label,"RSTIAPEAK",strlen("RSTIAPEAK"));
			ptr = put_io_buf(VAR,29);
			memcpy(ptr.pin->description,"Current Channel B Peak",strlen("Current Channel A Peak"));
			memcpy(ptr.pin->label,"IBPEAK",strlen("IBPEAK"));
			ptr = put_io_buf(VAR,30);
			memcpy(ptr.pin->description,"Read Current Channel B Peak With Reset",strlen("Read Current Channel A Peak With Reset"));
			memcpy(ptr.pin->label,"RSTIBPEAK",strlen("RSTIBPEAK"));
			ptr = put_io_buf(VAR,31);
			memcpy(ptr.pin->description,"Interrupt Enable A",strlen("Interrupt Enable A"));
			memcpy(ptr.pin->label,"IRQENA",strlen("IRQENA"));
			ptr = put_io_buf(VAR,32);
			memcpy(ptr.pin->description,"Interrupt Status A",strlen("Interrupt Status A"));
			memcpy(ptr.pin->label,"ISTATA",strlen("ISTATA"));
			ptr = put_io_buf(VAR,33);
			memcpy(ptr.pin->description,"Reset Interrupt Status A",strlen("Reset Interrupt Status A"));
			memcpy(ptr.pin->label,"RSTISA",strlen("RSTISA"));
			ptr = put_io_buf(VAR,34);
			memcpy(ptr.pin->description,"Interrupt Enable B",strlen("Interrupt Enable A"));
			memcpy(ptr.pin->label,"IRQENB",strlen("IRQENB"));
			ptr = put_io_buf(VAR,35);
			memcpy(ptr.pin->description,"Interrupt Status B",strlen("Interrupt Status A"));
			memcpy(ptr.pin->label,"ISTATB",strlen("ISTATB"));
			ptr = put_io_buf(VAR,36);
			memcpy(ptr.pin->description,"Reset Interrupt Status B",strlen("Reset Interrupt Status B"));
			memcpy(ptr.pin->label,"RSTISB",strlen("RSTISB"));
			ptr = put_io_buf(VAR,37);
			memcpy(ptr.pin->description,"Checksum",strlen("Checksum"));
			memcpy(ptr.pin->label,"CRC",strlen("CRC"));
			ptr = put_io_buf(VAR,38);
			memcpy(ptr.pin->description,"Current Channel Gain A",strlen("Current Channel Gain A"));
			memcpy(ptr.pin->label,"IAGAIN",strlen("IAGAIN"));
			ptr = put_io_buf(VAR,39);
			memcpy(ptr.pin->description,"Voltage Channel Gain A",strlen("Voltage Channel Gain A"));
			memcpy(ptr.pin->label,"VGAIN",strlen("VGAIN"));
			ptr = put_io_buf(VAR,40);
			memcpy(ptr.pin->description,"Active Power Gain A",strlen("Active Power Gain A"));
			memcpy(ptr.pin->label,"AWGAIN",strlen("AWGAIN"));
			ptr = put_io_buf(VAR,41);
			memcpy(ptr.pin->description,"Reactive Power Gain A",strlen("Reactive Power Gain A"));
			memcpy(ptr.pin->label,"AVARG",strlen("AVARG"));
			ptr = put_io_buf(VAR,42);
			memcpy(ptr.pin->description,"Apparent Power Gain A",strlen("Apparent Power Gain A"));
			memcpy(ptr.pin->label,"AVAG",strlen("AVAG"));
			ptr = put_io_buf(VAR,43);
			memcpy(ptr.pin->description,"IRMS Offset A",strlen("IRMS Offset A"));
			memcpy(ptr.pin->label,"AIRMSOS",strlen("AIRMSOS"));
			ptr = put_io_buf(VAR,44);
			memcpy(ptr.pin->description,"VRMS Offset",strlen("VRMS Offset"));
			memcpy(ptr.pin->label,"VRMSOS",strlen("VRMSOS"));
			ptr = put_io_buf(VAR,45);
			memcpy(ptr.pin->description,"Active Power Offset Correction A",strlen("Active Power Offset Correction A"));
			memcpy(ptr.pin->label,"AWATTOS",strlen("AWATTOS"));
			ptr = put_io_buf(VAR,46);
			memcpy(ptr.pin->description,"Reactive Power Offset Correction A",strlen("Reactive Power Offset Correction A"));
			memcpy(ptr.pin->label,"AVAROS",strlen("AVAROS"));
			ptr = put_io_buf(VAR,47);
			memcpy(ptr.pin->description,"Apparent Power Offset Correction A",strlen("Apparent Power Offset Correction A"));
			memcpy(ptr.pin->label,"AVAOS",strlen("AVAOS"));
			ptr = put_io_buf(VAR,48);
			memcpy(ptr.pin->description,"Voltage Channel Gain B",strlen("Voltage Channel Gain B"));
			memcpy(ptr.pin->label,"VGBIN",strlen("VGBIN"));
			ptr = put_io_buf(VAR,49);
			memcpy(ptr.pin->description,"Active Power Gain B",strlen("Active Power Gain B"));
			memcpy(ptr.pin->label,"BWGBIN",strlen("BWGBIN"));
			ptr = put_io_buf(VAR,50);
			memcpy(ptr.pin->description,"Reactive Power Gain B",strlen("Reactive Power Gain B"));
			memcpy(ptr.pin->label,"BVBRG",strlen("BVBRG"));
			ptr = put_io_buf(VAR,51);
			memcpy(ptr.pin->description,"Apparent Power Gain B",strlen("Apparent Power Gain B"));
			memcpy(ptr.pin->label,"BVBG",strlen("BVBG"));
			ptr = put_io_buf(VAR,52);
			memcpy(ptr.pin->description,"IRMS Offset B",strlen("IRMS Offset B"));
			memcpy(ptr.pin->label,"BIRMSOS",strlen("BIRMSOS"));
			ptr = put_io_buf(VAR,53);
			memcpy(ptr.pin->description,"Active Power Offset Correction B",strlen("Active Power Offset Correction B"));
			memcpy(ptr.pin->label,"BWBTTOS",strlen("BWBTTOS"));
			ptr = put_io_buf(VAR,54);
			memcpy(ptr.pin->description,"Reactive Power Offset Correction B",strlen("Reactive Power Offset Correction B"));
			memcpy(ptr.pin->label,"BVBROS",strlen("BVBROS"));
			ptr = put_io_buf(VAR,55);
			memcpy(ptr.pin->description,"Apparent Power Offset Correction B",strlen("Apparent Power Offset Correction B"));
			memcpy(ptr.pin->label,"BVBOS",strlen("BVBOS"));
			ptr = put_io_buf(VAR,56);
			memcpy(ptr.pin->description,"VOLTAGE FACTOR",strlen("VOLTAGE FACTOR"));
			ptr.pin->value = 136;
			ptr = put_io_buf(VAR,57);
			memcpy(ptr.pin->description,"CURRENT1 FACTOR",strlen("CURRENT1 FACTOR"));
			ptr.pin->value = 1318;
			ptr = put_io_buf(VAR,58);
			memcpy(ptr.pin->description,"CURRENT2 FACTOR",strlen("CURRENT2 FACTOR"));
			ptr.pin->value = 1318;
			put_io_buf(VAR,59);
			memcpy(ptr.pin->description,"POWER1 FACTOR",strlen("POWER1 FACTOR"));
			ptr.pin->value = 164;
			ptr = put_io_buf(VAR,60);
			memcpy(ptr.pin->description,"POWER2 FACTOR",strlen("POWER2 FACTOR"));
			ptr.pin->value = 164;
			ptr = put_io_buf(VAR,61);
			memcpy(ptr.pin->description,"ENERGY1 FACTOR",strlen("ENERGY1 FACTOR"));
			ptr.pin->value = 25240;
			ptr = put_io_buf(VAR,62);
			memcpy(ptr.pin->description,"ENERGY2 FACTOR",strlen("ENERGY2 FACTOR"));
			ptr.pin->value = 25240;
			ptr = put_io_buf(IN,0);memcpy(ptr.pin->description,"VOLTAGE",strlen("VOLTAGE"));
			ptr = put_io_buf(IN,1);memcpy(ptr.pin->description,"CURRENT1",strlen("CURRENT1"));
			ptr = put_io_buf(IN,2);memcpy(ptr.pin->description,"CURRENT2",strlen("CURRENT2"));
			ptr = put_io_buf(IN,3);memcpy(ptr.pin->description,"POWER1",strlen("POWER1"));
			ptr = put_io_buf(IN,4);memcpy(ptr.pin->description,"POWER2",strlen("POWER2"));
			ptr = put_io_buf(IN,5);memcpy(ptr.pin->description,"ENERGY1",strlen("ENERGY1"));
			ptr = put_io_buf(IN,6);memcpy(ptr.pin->description,"ENERGY2",strlen("ENERGY2"));

		}
		if(Modbus.mini_type == PROJECT_FAN_MODULE || Modbus.mini_type == PROJECT_AIRLAB)
		{
			ret = i2c_master_sensor_sht31(I2C_MASTER_NUM, sht31_data);//&sensor_data_h, &sensor_data_l);
//			xSemaphoreTake(print_mux, portMAX_DELAY);
			if (ret == ESP_ERR_TIMEOUT) {
				ESP_LOGE(TAG, "I2C Timeout");
				//Test[0] = 120;
			} else if (ret == ESP_OK) {
				hum_sensor_type = 1;
				g_sensors.original_temperature = SHT3X_getTemperature(sht31_data);
				g_sensors.original_humidity = SHT3X_getHumidity(&sht31_data[3]);
				g_sensors.temperature = (uint16_t)(g_sensors.original_temperature*10);
				g_sensors.humidity = (uint16_t)(g_sensors.original_humidity*10);
				put_io_buf(IN,0);
				if(ptr.pin->range == 3)
					ptr.pin->value = g_sensors.temperature*100;
				if(ptr.pin->range == 4)
					ptr.pin->value = (g_sensors.temperature*9/5)*100+32000;
				put_io_buf(IN,1);
				ptr.pin->value = g_sensors.humidity*100;

			} else {
				//ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...", esp_err_to_name(ret));

			}
//			xSemaphoreGive(print_mux);
		}


        vTaskDelay(100 / portTICK_RATE_MS);
        if(Modbus.mini_type == PROJECT_TRANSDUCER || Modbus.mini_type == PROJECT_LIGHT_SWITCH)
        {
			scd4x_start_periodic_measurement();
			vTaskDelay(100 / portTICK_RATE_MS);
			scd4x_read_measurement(&g_sensors.co2, &g_sensors.co2_temp, &g_sensors.co2_humi);
			Test[22]++;
			ret = sht4x_measure_blocking_read(&sht4x_temp, &sht4x_hum);
			if(ret != ESP_OK)
			{

			}
			else
			{
				g_sensors.temperature = (sht4x_temp)/100;
				g_sensors.humidity = (sht4x_hum)/100;
			}
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
#endif
		//-----------------tVOC

//		xSemaphoreTake(print_mux, portMAX_DELAY);

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
					ptr = put_io_buf(IN,3);
					ptr.pin->value = g_sensors.voc_value * 1000;

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
			  g_sensors.voc_baseline[1] = (iaq_baseline>>8) & 0XFF;
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

//		xSemaphoreGive(print_mux);
		vTaskDelay(500/portTICK_RATE_MS);
	}

		//------------------SCD40
#if 1
//		xSemaphoreTake(print_mux, portMAX_DELAY);
		// get  serial number
		if(co2_present == 1)
		{
			uint16_t co2;
			int32_t temperature;
			int32_t humidity;
			static uint8_t count_err = 0;
			if(scd4x_perform_forced == 1)
			{
				scd4x_stop_periodic_measurement();
				vTaskDelay(1000/portTICK_RATE_MS);
				scd4x_perform_forced_recalibration(co2_frc,&co2_asc);
				vTaskDelay(1000/portTICK_RATE_MS);
				scd4x_start_periodic_measurement();
				vTaskDelay(5000/portTICK_RATE_MS);
				scd4x_perform_forced = 0;
			}
			uint8_t error = scd4x_read_measurement(&co2, &temperature, &humidity);
			if (error) { count_err++;
					//printf("Error executing scd4x_read_measurement(): %i\n", error);
			} else if (co2 == 0) {
				 // printf("Invalid sample detected, skipping.\n");
			} else {count_err = 0;
				//CO2_get_value(co2,temperature / 100,humidity / 100);
				if(hum_sensor_type == 1)
				{

				}
				else
				{
					g_sensors.temperature = temperature / 100;
					g_sensors.humidity = humidity/ 100;
					ptr = put_io_buf(IN,0);
					ptr.pin->value = g_sensors.temperature;
					ptr = put_io_buf(IN,1);
					ptr.pin->value = g_sensors.humidity;
				}

				g_sensors.co2 = co2;
				ptr = put_io_buf(IN,2);
				ptr.pin->value = g_sensors.co2 * 1000;
			}
			if(count_err > 10)
				co2_present = 0;

		}
//		xSemaphoreGive(print_mux);
		vTaskDelay(2000 / portTICK_RATE_MS);
#endif

        //---------------------------------------------------

    }
//    vSemaphoreDelete(print_mux);
    vTaskDelete(NULL);
}


