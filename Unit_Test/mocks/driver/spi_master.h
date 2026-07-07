#ifndef DRIVER_SPI_MASTER_H
#define DRIVER_SPI_MASTER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI1_HOST 0
#define SPI2_HOST 1
#define SPI3_HOST 2

#define SPI_DMA_CH_AUTO 3

// Dummy SPI Master definitions
typedef int spi_host_device_t;
typedef void* spi_device_handle_t;

typedef struct {
    int miso_io_num;
    int mosi_io_num;
    int sclk_io_num;
    int quadwp_io_num;
    int quadhd_io_num;
    int max_transfer_sz;
} spi_bus_config_t;

#define SPI_DEVICE_NO_DUMMY (1 << 0)
#define SPI_DEVICE_HALFDUPLEX (1 << 1)

typedef struct {
    uint32_t mode;
    int clock_speed_hz;
    int spics_io_num;
    uint32_t queue_size;
    uint32_t flags;
} spi_device_interface_config_t;

typedef struct {
    uint32_t flags;
    const void* tx_buffer;
    void* rx_buffer;
    size_t length;
    size_t rxlength;
} spi_transaction_t;

int spi_bus_initialize(spi_host_device_t host, const spi_bus_config_t* bus_config, int dma_chan);
int spi_bus_add_device(spi_host_device_t host, const spi_device_interface_config_t* dev_config, spi_device_handle_t* handle);
int spi_device_transmit(spi_device_handle_t handle, spi_transaction_t* trans_desc);
int spi_bus_remove_device(spi_device_handle_t handle);
int spi_bus_free(spi_host_device_t host);

#ifdef __cplusplus
}
#endif

#endif // DRIVER_SPI_MASTER_H
