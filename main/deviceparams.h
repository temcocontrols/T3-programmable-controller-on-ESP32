#ifndef _DEVICE_PARAMS
#define _DEVICE_PARAMS

//#define A24_ARR_SIZE 24
#define SERIAL  0
#define TCP		1
#define USB		2
#define GSM		3
#define BAC_TO_MODBUS 4
#define WIFI  	5

#define UIP_HEAD 6

#define READ_VARIABLES      3
#define WRITE_VARIABLES     6
#define MULTIPLE_WRITE		16
#define CHECKONLINE			0x19
#define CHECKONLINE_WIHTCOM	 0x18

#define READ_COIL  			0X01
#define READ_DIS_INPUT 		0X02
#define READ_INPUT      	0x04
#define WRITE_COIL 			0X05
#define WRITE_MULTI_COIL 	0x0f

#define READ_REMOTE_INPUT		0x09

/* takes a byte out of a uint32 : var - uint32,  ByteNum - byte to take out (0 - 3) */
#define BREAK_UINT32( var, ByteNum ) \
          (uint8_t)((uint32_t)(((var) >>((ByteNum) * 8)) & 0x00FF))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
          + ((uint32_t)((Byte1) & 0x00FF) << 8) \
          + ((uint32_t)((Byte2) & 0x00FF) << 16) \
          + ((uint32_t)((Byte3) & 0x00FF) << 24)))

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

#define BUILD_UINT8(hiByte, loByte) \
          ((uint8_t)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))

#define HI_UINT8(a) (((a) >> 4) & 0x0F)
#define LO_UINT8(a) ((a) & 0x0F)

#pragma pack(1)
typedef struct
{
	unsigned short int cmd;   // low byte first
	unsigned short int len;   // low byte first
	unsigned short int own_sn[4]; // low byte first
	unsigned short int product;   // low byte first
	unsigned short int address;   // low byte first
	unsigned short int ipaddr[4]; // low byte first
	unsigned short int modbus_port; // low byte first
	unsigned short int firmwarerev; // low byte first
	unsigned short int hardwarerev;  // 28 29	// low byte first

	unsigned char master_sn[4];  // master's SN 30 31 32 33
	unsigned short int instance_low; // 34 35 hight byte first
	unsigned char panel_number; //  36
	char panelname[20]; // 37 - 56
	unsigned short int instance_hi; // 57 58 hight byte first

	unsigned char bootloader;  // 0 - app, 1 - bootloader, 2 - wrong bootloader , 3 - mstp device
	unsigned short int BAC_port;  //  hight byte first
	unsigned char zigbee_exist; // 0 - inexsit, 1 - exist

	unsigned char subnet_protocal; // 0 - modbus, 12 - bip to mstp

}STR_SCAN_CMD;
// This file defines structure of modbus parameters which reflect correspond modbus address space
// for each modbus register type (coils, discreet inputs, holding registers, input registers)
#pragma pack(push, 1)
typedef struct
{
    // Parameter: discrete_input0
    uint8_t discrete_input0:1;
    // Parameter: discrete_input1
    uint8_t discrete_input1:1;
    // Parameter: discrete_input2
    uint8_t discrete_input2:1;
    // Parameter: discrete_input3
    uint8_t discrete_input3:1;
    // Parameter: discrete_input4
    uint8_t discrete_input4:1;
    // Parameter: discrete_input5
    uint8_t discrete_input5:1;
    // Parameter: discrete_input6
    uint8_t discrete_input6:1;
    // Parameter: discrete_input7
    uint8_t discrete_input7:1;
    uint8_t discrete_input_port1:8;
} discrete_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    // Parameter: Coil 0 : Coil0
    uint8_t coil0:1;
    // Parameter: Coil 1 : Coil1
    uint8_t coil1:1;
    // Parameter: Coil 2 : Coil2
    uint8_t coil2:1;
    // Parameter: Coil 3 : Coil3
    uint8_t coil3:1;
    // Parameter: Coil 4 : Coil4
    uint8_t coil4:1;
    // Parameter: Coil 5 : Coil5
    uint8_t coil5:1;
    // Parameter: Coil 6 : Coil6
    uint8_t coil6:1;
    // Parameter: Coil 7 : Coil7
    uint8_t coil7:1;
    // Coils port 1
    uint8_t coil_port1:8;
} coil_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    // Parameter: Data channel 0 : data_chan0 : NV Address: 0
    float data_chan0;
    // Parameter: Data channel 1 : data_chan1 : NV Address: 0
    float data_chan1;
    // Parameter: Data channel 2 : data_chan2 : NV Address: 0
    float data_chan2;
    // Parameter: Data channel 3 : data_chan3 : NV Address: 0
    float data_chan3;
} input_reg_params_t;
#pragma pack(pop)

//See register map for more information.
#pragma pack(push, 1)
typedef struct
{
    // Parameter: Data channel 0 : DataChan0
    uint32_t serial_number_lo;
    // Parameter: Data channel 1 : DataChan1
    uint32_t serial_number_hi;
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

    uint16_t register75;
    uint16_t register76;
    uint16_t register77;
    uint16_t register78;
    uint16_t register79;
    uint16_t register80;
    uint16_t register81;
    uint16_t register82;
    uint16_t register83;
    uint16_t register84;
    uint16_t register85;
    uint16_t register86;
    uint16_t register87;
    uint16_t register88;
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

extern uint8_t CRClo;
extern uint8_t CRChi;
extern holding_reg_params_t holding_reg_params;
extern input_reg_params_t input_reg_params;
extern coil_reg_params_t coil_reg_params;
extern discrete_reg_params_t discrete_reg_params;

extern void modbus_init(void);
extern void modbus_task(void *arg);

extern void init_crc16(void);
extern void crc16_byte(uint8_t ch);
extern uint16_t crc16(uint8_t *p, uint8_t length);
extern void responseCmd(uint8_t type, uint8_t *pData, uint16_t len);

#endif // !defined(_DEVICE_PARAMS)
