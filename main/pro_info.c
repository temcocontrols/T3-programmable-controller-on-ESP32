#include "stdio.h"
#include "stdint.h"
#include "esp_attr.h"
#include "esp_app_format.h"

#define SOFTREV     20

#define FW_VER_HIGH (SOFTREV >> 8)
#define FW_VER_LOW 	(SOFTREV & 0xff)

/**
 * @brief Description about temco product.
 */
typedef struct {
    uint8_t move_offset_to_512[224];
    char vid[5];
    char pid[10];
    uint8_t ver[2];
    uint8_t resvered[3];
} temco_product_desc_t;

/** @cond */
_Static_assert(sizeof(temco_product_desc_t) == 244, "temco_product_desc_t should be 20 bytes"); // 244=224+20
/** @endcond */

// const __attribute__((section(".rodata_desc"))) temco_product_desc_t pro_info = 
// {
//     {'T','E','M','C','O'},
//     {'A', 'i', 'r', 'L', 'a', 'b', '-', 'e', 's', 'p'},
//     {FW_VER_HIGH, FW_VER_LOW},
//     {0, 0, 0},
// };

const __attribute__((section(".rodata_desc"))) temco_product_desc_t pro_info = 
{
    .vid = {'T','e','m','c','o'},
    .pid = {'P', 'I', 'D', '9', '0', ' ', ' ', ' ', 0, 0},
    .ver = {FW_VER_HIGH, FW_VER_LOW},
};

