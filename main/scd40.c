#include "sensirion_arch_config.h"
#include "sensirion_common.h"
#include "sgp_git_version.h"
#include "i2c_task.h"
#include "esp_err.h"

#define SCD40_CMD_START_PERIODIC_MEASUREMENT 0x0010
#define SCD40_CMD_STOP_PERIODIC_MEASUREMENT 0x0104
#define SCD40_CMD_READ_MEASUREMENT 0x0300
#define SCD40_CMD_SET_MEASUREMENT_INTERVAL 0x4600
#define SCD40_CMD_GET_DATA_READY 0x0202
#define SCD40_CMD_SET_TEMPERATURE_OFFSET 0x5403
#define SCD40_CMD_SET_ALTITUDE 0x5102
#define SCD40_CMD_SET_FORCED_RECALIBRATION 0x5204
#define SCD40_CMD_AUTO_SELF_CALIBRATION 0x5306
#define SCD40_CMD_READ_SERIAL 0xD033
#define SCD40_SERIAL_NUM_WORDS 16
#define SCD40_WRITE_DELAY_US 20000

#define SCD40_MAX_BUFFER_WORDS 24
#define SCD40_CMD_SINGLE_WORD_BUF_LEN \
    (SENSIRION_COMMAND_SIZE + SENSIRION_WORD_SIZE + CRC8_LEN)

int16_t scd40_start_periodic_measurement(uint16_t ambient_pressure_mbar) {
    if (ambient_pressure_mbar &&
        (ambient_pressure_mbar < 700 || ambient_pressure_mbar > 1400)) {
        /* out of allowable range */
        return STATUS_FAIL;
    }

    return sensirion_i2c_write_cmd_with_args(
    		SCD40_SENSOR_ADDR, SCD40_CMD_START_PERIODIC_MEASUREMENT,
        &ambient_pressure_mbar, SENSIRION_NUM_WORDS(ambient_pressure_mbar));
}

int16_t scd40_stop_periodic_measurement() {
    return sensirion_i2c_write_cmd(SCD40_SENSOR_ADDR,
                                   SCD40_CMD_STOP_PERIODIC_MEASUREMENT);
}

#if 0
int16_t scd40_read_measurement(float* co2_ppm, float* temperature,
                               float* humidity) {
    int16_t error;
    uint8_t data[3][4];

    error =
        sensirion_i2c_write_cmd(SCD40_SENSOR_ADDR, SCD40_CMD_READ_MEASUREMENT);
    if (error != ESP_OK)
        return error;
    error = sensirion_i2c_read_words_as_bytes(SCD40_SENSOR_ADDR, &data[0][0],
                                              SENSIRION_NUM_WORDS(data));
    if (error != ESP_OK)
        return error;

    *co2_ppm = sensirion_bytes_to_float(data[0]);
    *temperature = sensirion_bytes_to_float(data[1]);
    *humidity = sensirion_bytes_to_float(data[2]);

    return ESP_OK;
}
#endif


