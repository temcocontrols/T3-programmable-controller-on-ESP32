#ifndef _I2C_TASK
#define _I2C_TASK

#include "stdbool.h"
#include "esp_err.h"

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define DATA_LENGTH 512                  /*!< Data buffer length of test buffer */
#define RW_TEST_LENGTH 128               /*!< Data length for r/w test, [0,DATA_LENGTH] */
#define DELAY_TIME_BETWEEN_ITEMS_MS 1000 /*!< delay time between different test items */

#define I2C_MASTER_NUM I2C_NUMBER(1) 	/*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

#define SHT31_SENSOR_ADDR 0x44 					/*!< slave address for SHT31 sensor */	//0x88-R, 0x89-W
#define VEML7700_SENSOR_ADDR	0x10			/*!< slave address for VEML7700 sensor*/   //0x20-R, 0x21-W
#define SGP30_SENSOR_ADDR	0x58				/*!< slave address for VOC SGP30 sensor*/  //0xB0-R, 0xB1-W
#define MLX90632_SENSOR_ADDR	0x3A			/*!< slave address for Infrared temperatrue sensor*/ //0x74-R, 0x75-W
#define SCD40_SENSOR_ADDR	0x62				/*!< slave address for SCD40 sensor*/ //0xC4-R, 0xC5-W
#define MLX90614_SENSOR_ADDR	0x5A			/*!< slave address for MLX90614 sensor*/ //0xB4-R, 0xB5-W
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

#define BUILD_UINT8(hiByte, loByte) \
          ((uint8_t)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))

#define HI_UINT8(a) (((a) >> 4) & 0x0F)
#define LO_UINT8(a) ((a) & 0x0F)



/*typedef enum{
	PROJECT_SAUTER,
	PROJECT_FAN_MODULE=13,
	PROJECT_POWER_METER=14,
	PROJECT_AIRLAB=15,
}project_e;*/

#pragma pack(push, 1)
typedef struct{
	int16_t temperature;
	int16_t humidity;
	uint16_t co2;
	uint16_t tvoc_ppb;
	uint16_t ethanol_raw_signal;
	uint16_t h2_raw_signal;
	uint8_t voc_baseline[4];
	uint16_t voc_value;
	uint16_t light_value;
	uint8_t occ;
	uint32_t sound;
	uint16_t ambient;
	uint16_t object;
	int16_t co2_temp;
	int16_t co2_humi;
	uint16_t absHumi;
	int16_t dewpoint;
	uint16_t enthalpy;
	float original_temperature;
	float original_humidity;
	bool co2_ready;
	bool voc_ini_baseline;
	bool co2_start_measure;
	bool co2_stop_measure;
}g_sensor_t;
#pragma pack(pop)

extern g_sensor_t g_sensors;
extern esp_err_t i2c_master_init();
extern void i2c_task(void *arg);
extern void sensirion_sleep_usec(uint32_t useconds);
extern esp_err_t sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count);
extern esp_err_t sensirion_i2c_write(uint8_t address, const uint8_t *data, uint16_t count);
extern esp_err_t LED_i2c_write(uint8_t address, const uint8_t *data,uint16_t count);
extern esp_err_t LED_i2c_read(uint8_t address, uint8_t *data, uint16_t count);

extern esp_err_t stm_i2c_write(uint8_t reg,const uint8_t *data, uint16_t count) ;
extern int32_t stm_i2c_read(int16_t reg, uint8_t *value,uint16_t len);
#endif
