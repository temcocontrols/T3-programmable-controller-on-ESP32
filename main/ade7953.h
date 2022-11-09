#ifndef __ADE7953_H
#define __ADE7953_H

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define BREAK_UINT32( var, ByteNum ) \
          (uint8)((uint32)(((var) >>((ByteNum) * 8)) & 0x00FF))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32)((uint32)((Byte0) & 0x00FF) \
          + ((uint32)((Byte1) & 0x00FF) << 8) \
          + ((uint32)((Byte2) & 0x00FF) << 16) \
          + ((uint32)((Byte3) & 0x00FF) << 24)))

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

#define BUILD_UINT8(hiByte, loByte) \
          ((uint8_t)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))

#define HI_UINT8(a) (((a) >> 4) & 0x0F)
#define LO_UINT8(a) ((a) & 0x0F)
// Registers
//
#define MGOS_ADE7953_REG_LCYCMODE 0x004
#define MGOS_ADE7953_REG_PGA_V 0x007
#define MGOS_ADE7953_REG_PGA_IA 0x008
#define MGOS_ADE7953_REG_PGA_IB 0x009
#define MGOS_ADE7953_REG_UNNAMED 0x0FE

#define MGOS_ADE7953_REG_LINECYC 0x101
#define MGOS_ADE7953_REG_CONFIG 0x102
#define MGOS_ADE7953_REG_PERIOD 0x10E
#define MGOS_ADE7953_REG_RESERVED 0x120

#define MGOS_ADE7953_REG_AIRMSOS 0x386
#define MGOS_ADE7953_REG_VRMSOS 0x388
#define MGOS_ADE7953_REG_BIRMSOS 0x392
#define MGOS_ADE7953_REG_AVA 0x310
#define MGOS_ADE7953_REG_BVA 0x311
#define MGOS_ADE7953_REG_AWATT 0x312
#define MGOS_ADE7953_REG_BWATT 0x313
#define MGOS_ADE7953_REG_AVAR 0x314
#define MGOS_ADE7953_REG_BVAR 0x315
#define MGOS_ADE7953_REG_IA 0x31A
#define MGOS_ADE7953_REG_IB 0x31B
#define MGOS_ADE7953_REG_V 0x31C
#define MGOS_ADE7953_REG_ANENERGYA 0x31E
#define MGOS_ADE7953_REG_ANENERGYB 0x31F
#define MGOS_ADE7953_REG_PFA 0x10A
#define MGOS_ADE7953_REG_PFB 0x10B

#define MGOS_ADE7953_REG_IRQSTATA 0x32D
#define MGOS_ADE7953_REG_IRQSTATA_RESET (1 << 20)

#define MGOS_ADE7953_REG_VERSION 0x702
#define MGOS_ADE7953_REG_EX_REF 0x800

#define MGOS_ADE7953_REG_CONFIG_HPFEN (1 << 2)
#define MGOS_ADE7953_REG_CONFIG_SWRST (1 << 7)

enum ade7953_pga_gain {
  MGOS_ADE7953_PGA_GAIN_1 = 0x00,
  MGOS_ADE7953_PGA_GAIN_2 = 0x01,
  MGOS_ADE7953_PGA_GAIN_4 = 0x02,
  MGOS_ADE7953_PGA_GAIN_8 = 0x03,
  MGOS_ADE7953_PGA_GAIN_16 = 0x04,
  MGOS_ADE7953_PGA_GAIN_22 = 0x05
};
//
#define MGOS_ADE7953_REG_LCYCMODE 0x004
#define MGOS_ADE7953_REG_PGA_V 0x007
#define MGOS_ADE7953_REG_PGA_IA 0x008
#define MGOS_ADE7953_REG_PGA_IB 0x009
#define MGOS_ADE7953_REG_UNNAMED 0x0FE

#define MGOS_ADE7953_REG_LINECYC 0x101
#define MGOS_ADE7953_REG_CONFIG 0x102
#define MGOS_ADE7953_REG_PERIOD 0x10E
#define MGOS_ADE7953_REG_RESERVED 0x120

#define MGOS_ADE7953_REG_AIRMSOS 0x386
#define MGOS_ADE7953_REG_VRMSOS 0x388
#define MGOS_ADE7953_REG_BIRMSOS 0x392
#define MGOS_ADE7953_REG_AVA 0x310
#define MGOS_ADE7953_REG_BVA 0x311
#define MGOS_ADE7953_REG_AWATT 0x312
#define MGOS_ADE7953_REG_BWATT 0x313
#define MGOS_ADE7953_REG_AVAR 0x314
#define MGOS_ADE7953_REG_BVAR 0x315
#define MGOS_ADE7953_REG_IA 0x31A
#define MGOS_ADE7953_REG_IB 0x31B
#define MGOS_ADE7953_REG_V 0x31C
#define MGOS_ADE7953_REG_ANENERGYA 0x31E
#define MGOS_ADE7953_REG_ANENERGYB 0x31F
#define MGOS_ADE7953_REG_PFA 0x10A
#define MGOS_ADE7953_REG_PFB 0x10B

#define MGOS_ADE7953_REG_IRQSTATA 0x32D
#define MGOS_ADE7953_REG_IRQSTATA_RESET (1 << 20)

#define MGOS_ADE7953_REG_VERSION 0x702
#define MGOS_ADE7953_REG_EX_REF 0x800

#define MGOS_ADE7953_REG_CONFIG_HPFEN (1 << 2)
#define MGOS_ADE7953_REG_CONFIG_SWRST (1 << 7)

void Ade7953_init(); // Setup GPIO and ADE7953 registers. Call first!

// Each value reading resets the register (nergy count from zero after read).
uint32_t Ade7953_getEnergy(uint8_t channel); // Ws (watt * secound divide by 3600 for Wh)

void Ade7953GetData(void); // You must called this function before any get¡±.
// The data in get¡± finctions is current at the moment the read function is called.
uint16_t Ade7953_getCurrent(uint8_t channel); // A x100 (dyvide by 100 for Amper). Data
uint16_t Ade7953_getVoltage();
uint16_t Ade7953_getActivePower(uint8_t channel);
void newAde7953Read (uint16_t reg, uint8_t* value);
uint32_t Ade7953Read(uint16_t reg);

#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif //ADE7953_h
