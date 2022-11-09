#include "ade7953.h"
//#include "mem.h"
//#include "gpio.h"
//#include "esp_attr.h"
#include "sdkconfig.h"
#include "i2c_task.h"//#include "esp_attr.h"
#include "define.h"
#include "led_pwm.h"
#include "user_data.h"

//#include "i2c_jk.h"

#if 1

#define LOCAL //EXT_RAM_ATTR

uint8_t _ade7953_write_addr;
uint8_t _ade7953_read_addr;

#define ADE7953_PREF            (155)
#define ADE7953_UREF            (26000L)
#define ADE7953_IREF            (1000)

#define ADE7953_ADDR            (0x38)

LOCAL uint32_t ade7953_active_power1 = 0;
LOCAL uint32_t ade7953_active_power2 = 0;
LOCAL uint32_t ade7953_current_rms1 = 0;
LOCAL uint32_t ade7953_current_rms2 = 0;
LOCAL uint32_t ade7953_voltage_rms = 0;
uint32_t ade_7953_voltage = 0;

struct {
	uint32_t buf;
	uint16_t register_name;
} ade7953_reg_t;

/*
struct {
	double voltage_scale;
	double voltageOffset;
	double currentScale0;
	double currentScale1;
	double currentOffset0;
	double currentOffset1;
	double powerScale0;
	double powerScale1;
	double energyScale0;
	double energyScale1;
	double voltagePgaGain;
	double currentPgaGain0;
	double currentPgaGain1;
}mgos_config_ade7953;


const struct mgos_config_ade7953 ade_cfg = {
    .voltage_scale = .0000382602,
    .voltage_offset = -0.068,
    .current_scale_0 = 0.00000949523,
    .current_scale_1 = 0.00000949523,
    .current_offset_0 = -0.017,
    .current_offset_1 = -0.017,
    .apower_scale_0 = (1 / 164.0),
    .apower_scale_1 = (1 / 164.0),
    .aenergy_scale_0 = (1 / 25240.0),
    .aenergy_scale_1 = (1 / 25240.0),
    .voltage_pga_gain = MGOS_ADE7953_PGA_GAIN_1,
    .current_pga_gain_0 = MGOS_ADE7953_PGA_GAIN_8,
    .current_pga_gain_1 = MGOS_ADE7953_PGA_GAIN_8,
};*/

LOCAL int8_t Ade7953RegSize(uint16_t reg){
    int size = 0;
    switch ((reg >> 8) & 0x0F) {
        case 0x03:
        size++;
        case 0x02:
        size++;
        case 0x01:
        size++;
        case 0x00:
        case 0x07:
        case 0x08:
        size++;
    }
    return size;
}

void newAde7953Read (uint16 reg, uint8_t* value)
{
	uint8_t cmd_h, cmd_l;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	cmd_h = HI_UINT16(reg);
	cmd_l = LO_UINT16(reg);
	int size = Ade7953RegSize(reg);
	if (size) {
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, _ade7953_write_addr, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, cmd_h, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, cmd_l, ACK_CHECK_EN);
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, _ade7953_read_addr, ACK_CHECK_EN);
		for (int i = 0; i < (size - 1); i++) {
			i2c_master_read_byte(cmd, value, ACK_VAL);
		}
		i2c_master_read_byte(cmd, value[size-1], NACK_VAL);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
		i2c_cmd_link_delete(cmd);
	}
}

LOCAL uint32_t Ade7953Read(uint16_t reg){
	uint8_t tempBuf[4];
	uint8_t temp_data=0;
    uint32_t response = 0;
    uint8_t cmd_h,cmd_l;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    cmd_h = HI_UINT16(reg);
    cmd_l = LO_UINT16(reg);

    int size = Ade7953RegSize(reg);
    if (size) {
        i2c_master_start(cmd);
        //i2c_master_writeByte(_ade7953_write_addr);
        i2c_master_write_byte(cmd, _ade7953_write_addr, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, cmd_h, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, cmd_l, ACK_CHECK_EN);



        i2c_master_start(cmd);
        //i2c_master_writeByte(_ade7953_read_addr);
        i2c_master_write_byte(cmd, _ade7953_read_addr, ACK_CHECK_EN);

        for (int i = 0; i < (size - 1); i++) {
            //response = response << 8 | i2c_master_readByte();   // receive DATA (MSB first)
        	//i2c_master_read_byte(cmd, &temp_data, ACK_VAL);
        	i2c_master_read_byte(cmd, &holding_reg_params.testBuf[i], ACK_VAL);
        	//response = response << 8 | temp_data;
        }
        //i2c_master_read(cmd, tempBuf, size-1, ACK_VAL);
        //holding_reg_params.testBuf[0] = 0;
        //holding_reg_params.testBuf[1] = 0;
        //holding_reg_params.testBuf[2] = 0;
        //holding_reg_params.testBuf[3] = 0;
        //i2c_master_read(cmd, holding_reg_params.testBuf, size-1, ACK_VAL);
        i2c_master_read_byte(cmd, &holding_reg_params.testBuf[3], NACK_VAL);
        if(size == 4)
        	response = BUILD_UINT32(holding_reg_params.testBuf[3],holding_reg_params.testBuf[2],holding_reg_params.testBuf[1],holding_reg_params.testBuf[0]);
        else if( size == 3)
        	response = BUILD_UINT32(holding_reg_params.testBuf[3],holding_reg_params.testBuf[1],holding_reg_params.testBuf[0],holding_reg_params.testBuf[2]);
        else
        	response = BUILD_UINT32(holding_reg_params.testBuf[3],holding_reg_params.testBuf[1],holding_reg_params.testBuf[0],holding_reg_params.testBuf[2]);
        Test[15] = holding_reg_params.testBuf[0];//tempBuf[0];
        Test[16] = holding_reg_params.testBuf[1];//tempBuf[1];
        Test[17] = holding_reg_params.testBuf[2];//tempBuf[2];
        Test[18] = holding_reg_params.testBuf[3];//tempBuf[3];
    	//i2c_master_read_byte(cmd, &temp_data, NACK_VAL);
    	//response = response << 8 | temp_data;
    	//response =  temp_data;
        //response = response << 8 | i2c_master_readByte();

        //i2c_master_send_nack(); // STOP
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
		i2c_cmd_link_delete(cmd);
    }

    return response;
}

void Ade7953Write(uint16_t reg, uint32_t val){
    int ret;
    uint8_t cmd_h,cmd_l;
    //uint16_t got_data;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    int size = Ade7953RegSize(reg);
    cmd_h = HI_UINT16(reg);
    cmd_l = LO_UINT16(reg);
    if (size) {
        i2c_master_start(cmd);
        //i2c_master_writeByte(_ade7953_write_addr);
        i2c_master_write_byte(cmd, _ade7953_write_addr, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, cmd_h, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, cmd_l, ACK_CHECK_EN);
        //if (!i2c_master_checkAck()){
        //    i2c_master_stop();
        //    return;
        //}
        //i2c_master_writeByte((reg >> 8) & 0xFF);

        //i2c_master_writeByte(reg & 0xFF);

        while (size--) {
           // i2c_master_writeByte((val >> (8 * size)) & 0xFF);
        	i2c_master_write_byte(cmd, (val >> (8 * size)) & 0xFF, ACK_CHECK_EN);
        }
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        //os_delay_us(5);
    }
}

void Ade7953_init(){
    uint8_t addr = ADE7953_ADDR;
    _ade7953_write_addr = addr << 1;
    _ade7953_read_addr = _ade7953_write_addr + 0x1;

    Ade7953Write(0x102, 0x0004);    // Locking the communication interface (Clear bit COMM_LOCK), Enable HPF
    Ade7953Write(0x0FE, 0x00AD);    // Unlock register 0x120
    Ade7953Write(0x120, 0x0030);    // Configure optimum setting
 // Ade7953Write(0x201, 0b0101);    // Only positive acumulation od energy (not necessary)
}

void Ade7953GetData(void){
    ade7953_voltage_rms = Ade7953Read(0x21C);      // Both relays
    vTaskDelay(100 / portTICK_RATE_MS);
    ade_7953_voltage = Ade7953Read(0x218);
    ade7953_current_rms1 = Ade7953Read(0x21B);     // Relay 1
    if (ade7953_current_rms1 < 2000) {             // No load threshold (20mA)
        ade7953_current_rms1 = 0;
        ade7953_active_power1 = 0;
    } else {
        ade7953_active_power1 = (int32_t)Ade7953Read(0x313) * -1;//Ade7953Read(0x213);  // Relay 1
    }
    vTaskDelay(100 / portTICK_RATE_MS);
    ade7953_current_rms2 = Ade7953Read(0x21A);     // Relay 2
    if (ade7953_current_rms2 < 2000) {             // No load threshold (20mA)
        ade7953_current_rms2 = 0;
        ade7953_active_power2 = 0;
    } else {
        ade7953_active_power2 = Ade7953Read(0x212);  // Relay 2
    }
	Test[8] = ((uint16_t)ade7953_voltage_rms) & 0xffff;
	Test[9] = ((uint16_t)ade7953_voltage_rms >> 16) & 0xffff;


	Test[10] = ade_7953_voltage & 0xffff;
	Test[11] = (ade_7953_voltage >> 16) & 0xffff;
	Test[12] = ade7953_active_power1 & 0xffff;
	Test[13] = ade7953_active_power2 & 0xffff;

  //  os_printf("V: %d, A1: %d, A2: %d, P1: %d, P2: %d\n",ade7953_voltage_rms,ade7953_current_rms1,ade7953_current_rms2, ade7953_active_power1,ade7953_active_power2);
}

uint16_t Ade7953_getVoltage(){
	//ade7953_voltage_rms = Ade7953Read(0x21C);      // Both relays
	newAde7953Read(MGOS_ADE7953_REG_V, &inputs[0].value);//
    return (ade7953_voltage_rms / ADE7953_UREF);
}
uint16_t Ade7953_getCurrent(uint8_t channel){
	//ade7953_current_rms2 = Ade7953Read(0x21A);     // Relay 2
	newAde7953Read(MGOS_ADE7953_REG_IA, &inputs[1].value); //
    return (channel < 2 ? ade7953_current_rms1 : ade7953_current_rms2) / ADE7953_IREF;
}
uint16_t Ade7953_getActivePower(uint8_t channel){
	newAde7953Read(MGOS_ADE7953_REG_AWATT, &inputs[3].value);
    return (channel < 2 ? ade7953_active_power1 : ade7953_active_power2 ) / ADE7953_PREF;
}

uint32_t Ade7953_getEnergy(uint8_t channel){ // Any read reset register. Energy count from zero after read.
    return ((Ade7953Read(channel < 2 ? 0x31F : 0x31E) * ADE7953_PREF ) / 1000);// Ws (watt * secound divide by 3600 for Wh)
}
#endif
