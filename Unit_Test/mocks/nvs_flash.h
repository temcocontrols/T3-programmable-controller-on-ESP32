#ifndef NVS_FLASH_H
#define NVS_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

int nvs_flash_init(void);
int nvs_flash_erase(void);

#ifdef __cplusplus
}
#endif

#endif // NVS_FLASH_H
