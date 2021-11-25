#ifndef __LED_PWM_H
#define __LED_PWM_H


#include "driver/ledc.h"

//See register map for more information.
#pragma pack(push, 1)
typedef struct
{
    // Parameter: Data channel 0 : DataChan0
    uint16_t serial_number_lo;
    // Parameter: Data channel 1 : DataChan1
    uint16_t serial_number_hi;
    // Parameter: Data channel 2 : DataChan2
    uint16_t version_number_lo;
    // Parameter: Data channel 3 : DataChan3
    uint16_t version_number_hi;
    // Parameter: Modbus Network Address : modbus_address
    uint8_t modbus_address;    //6
    // Parameter: Protocol version  : protocol_version
    uint8_t product_model;
    // Parameter: Hardware version  : hardware_version
    uint8_t hardware_version;   //8
    // Parameter: Software Version : software_version
    uint16_t isp_mode_indication;
    // Parameter: Modbus Baudrate : modbus_baud
    uint8_t baud_rate;
    uint16_t update_status;
    uint16_t ethernet_status;
    uint8_t modbus_bacnet_switch;

    uint16_t testBuf[20];

    uint8_t mac_addr[6];
    uint16_t ip_mode;
    uint8_t ip_addr[4];
    uint8_t ip_net_mask[4];
    uint8_t ip_gateway[4];

    uint8_t which_project;
    uint16_t fan_module_pwm1;
    uint16_t fan_module_pwm2;
    uint16_t led_rx485_tx;
    uint16_t led_rx485_rx;
    uint16_t fan_module_pulse;
    uint16_t fan_module_input_voltage;
    uint16_t fan_module_10k_temp;
    int16_t sht31_temp_offset;
    int16_t temp_10k_offset;
    int16_t ambient_temp_offset;
    int16_t object_temp_offset;
    uint16_t co2_frc;
    uint16_t co2_asc_enable;
    uint16_t wifi_led;
    uint16_t register89;
    uint16_t register90;
    uint16_t register91;
    uint16_t register92;
    uint16_t register93;
    uint16_t register94;
    uint16_t register95;
    uint16_t register96;
    uint16_t register97;
    uint16_t register98;
    uint16_t register99;
    uint16_t register100;

    uint16_t coolheatmode;
    uint16_t pid1_mode_operation;

    char panelname[20];
    //uint8_t stm32_uart_send[STM32_UART_SEND_SIZE];
    // Parameter: Modbus parity  : modbus_parity
    //uint16_t modbus_parity;
    // Parameter: Modbus stopbit  : modbus_stop_bits
    //uint16_t modbus_stop_bits;
    // Parameter: Brace control  : modbus_brace_ctrl
    uint16_t modbus_brace_ctrl;
    // Parameter: Up time  : up_time
    uint32_t up_time;
    // Parameter: Device state  : device_state
    uint16_t device_state;
    // readyToUpdate set to 1, OTA will download bin file and update
    uint16_t readyToUpdate;
    // temperature
    uint16_t sht31temperature;
    // humidity
    uint16_t sht31humidity;

} holding_reg_params_t;
#pragma pack(pop)

extern holding_reg_params_t holding_reg_params;

extern ledc_channel_config_t ledc_channel[];
extern void led_pwm_init(void);
extern void led_init(void);
extern void my_pcnt_init(void);
extern void adc_init(void);

#endif
