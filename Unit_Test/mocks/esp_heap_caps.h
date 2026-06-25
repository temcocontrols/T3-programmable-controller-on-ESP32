#ifndef ESP_HEAP_CAPS_H
#define ESP_HEAP_CAPS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MALLOC_CAP_DMA             (1 << 3)
#define MALLOC_CAP_8BIT            (1 << 2)
#define MALLOC_CAP_32BIT           (1 << 1)
#define MALLOC_CAP_SPIRAM          (1 << 10)
#define MALLOC_CAP_INTERNAL        (1 << 11)

void *heap_caps_malloc(size_t size, uint32_t caps);
void heap_caps_free(void *ptr);
void *heap_caps_realloc(void *ptr, size_t size, uint32_t caps);
void *heap_caps_calloc(size_t n, size_t size, uint32_t caps);
size_t heap_caps_get_total_size(uint32_t caps);
size_t heap_caps_get_free_size(uint32_t caps);

#ifdef __cplusplus
}
#endif

#endif // ESP_HEAP_CAPS_H
