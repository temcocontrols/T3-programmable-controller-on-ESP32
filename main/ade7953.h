#ifndef __ADE7953_H
#define __ADE7953_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

int8_t Ade7953_init(); // Setup GPIO and ADE7953 registers. Call first!

// Each value reading resets the register (nergy count from zero after read).
uint32_t Ade7953_getEnergy(uint8_t channel); // Ws (watt * secound divide by 3600 for Wh)

void Ade7953GetData(void); // You must called this function before any get¡±.
// The data in get¡± finctions is current at the moment the read function is called.
uint16_t Ade7953_getCurrent(uint8_t channel); // A x100 (dyvide by 100 for Amper). Data
uint16_t Ade7953_getVoltage();
uint16_t Ade7953_getActivePower(uint8_t channel);

#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif //ADE7953_h
