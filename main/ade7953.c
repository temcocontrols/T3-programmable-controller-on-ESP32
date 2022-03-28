#include "ade7953.h"
//#include "mem.h"
//#include "gpio.h"
//#include "esp_attr.h"
#include "sdkconfig.h"
#include "i2c_task.h"//#include "esp_attr.h"

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


LOCAL uint32_t Ade7953Read(uint16_t reg){
    uint32_t response = 0;

    int size = Ade7953RegSize(reg);
    if (size) {
        i2c_master_start();
        i2c_master_writeByte(_ade7953_write_addr);

        if (!i2c_master_checkAck()){
            i2c_master_stop();
            return 0xFFFFFFFF;
        }
        i2c_master_writeByte((reg >> 8) & 0xFF);
        if (!i2c_master_checkAck()){
            i2c_master_stop();
            return 0xFFFFFFFF;
        }
        i2c_master_writeByte(reg & 0xFF);
        if (!i2c_master_checkAck()){
            i2c_master_stop();
            return 0xFFFFFFFF;
        }

        i2c_master_start();
        i2c_master_writeByte(_ade7953_read_addr);
        if (!i2c_master_checkAck()){
            i2c_master_stop();
            return 0xFFFFFFFF;
        }

        for (int i = 0; i < (size - 1); i++) {
            response = response << 8 | i2c_master_readByte();   // receive DATA (MSB first)
            i2c_master_send_ack();
        }
        response = response << 8 | i2c_master_readByte();

        i2c_master_send_nack(); // STOP
		i2c_master_stop();
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
        i2c_master_stop();
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        //os_delay_us(5);
    }
}

int8_t Ade7953_init(){
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

    ade7953_current_rms1 = Ade7953Read(0x21B);     // Relay 1
    if (ade7953_current_rms1 < 2000) {             // No load threshold (20mA)
        ade7953_current_rms1 = 0;
        ade7953_active_power1 = 0;
    } else {
        ade7953_active_power1 = (int32_t)Ade7953Read(0x313) * -1;//Ade7953Read(0x213);  // Relay 1
    }

    ade7953_current_rms2 = Ade7953Read(0x21A);     // Relay 2
    if (ade7953_current_rms2 < 2000) {             // No load threshold (20mA)
        ade7953_current_rms2 = 0;
        ade7953_active_power2 = 0;
    } else {
        ade7953_active_power2 = Ade7953Read(0x212);  // Relay 2
    }

  //  os_printf("V: %d, A1: %d, A2: %d, P1: %d, P2: %d\n",ade7953_voltage_rms,ade7953_current_rms1,ade7953_current_rms2, ade7953_active_power1,ade7953_active_power2);
}

uint16_t Ade7953_getVoltage(){
    return (ade7953_voltage_rms / ADE7953_UREF);
}
uint16_t Ade7953_getCurrent(uint8_t channel){
    return (channel < 2 ? ade7953_current_rms1 : ade7953_current_rms2) / ADE7953_IREF;
}
uint16_t Ade7953_getActivePower(uint8_t channel){
    return (channel < 2 ? ade7953_active_power1 : ade7953_active_power2 ) / ADE7953_PREF;
}

uint32_t Ade7953_getEnergy(uint8_t channel){ // Any read reset register. Energy count from zero after read.
    return ((Ade7953Read(channel < 2 ? 0x31F : 0x31E) * ADE7953_PREF ) / 1000);// Ws (watt * secound divide by 3600 for Wh)
}
#endif
