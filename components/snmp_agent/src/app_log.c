#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"

// Recommended: Define this in your project's sdkconfig or build flags
// #define DEBUG_INFO_UART0 1  // Uncomment to enable UART0 output instead of ESP_LOG

#ifndef DEBUG_INFO_UART0
#define DEBUG_INFO_UART0 1  // Default to ESP_LOGI output
#endif

#ifndef DEBUG_INFO_BUFFER_SIZE
#define DEBUG_INFO_BUFFER_SIZE 256  // Adjust based on your longest expected message
#endif

#ifndef DEBUG_INFO_DEFAULT_TAG
#define DEBUG_INFO_DEFAULT_TAG "DEBUG"
#endif

/**
 * Debug logging function with printf-style formatting
 * Usage: debug_info(TAG, "Value: %d, Temp: %.2f", int_val, float_val);
 * 
 * @param tag    Component tag (appears in ESP_LOG output)
 * @param format printf-style format string
 * @param ...    Variable arguments matching format specifiers
 */
void app_log(const char *tag, const char *format, ...)
{
    if (!format) return;

    // Fallback to default tag if NULL/empty
    if (!tag || tag[0] == '\0') {
        tag = DEBUG_INFO_DEFAULT_TAG;
    }

    char buffer[DEBUG_INFO_BUFFER_SIZE];
    va_list args;

    // Format the message safely
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len < 0) return;  // Formatting error
    if (len >= DEBUG_INFO_BUFFER_SIZE) {
        // Truncation occurred - indicate in output
        const char *trunc_msg = "[TRUNCATED] ";
        if (DEBUG_INFO_UART0) {
            uart_write_bytes(UART_NUM_0, trunc_msg, strlen(trunc_msg));
        } else {
            ESP_LOGW(tag, "%s%s", trunc_msg, buffer);
            return;
        }
    }

#if DEBUG_INFO_UART0
    // UART output: explicit newline required
    uart_write_bytes(UART_NUM_0, buffer, strlen(buffer));
    uart_write_bytes(UART_NUM_0, "\r\n", 2);
#else
    // ESP_LOGI automatically appends newline - don't add extra \r\n
    ESP_LOGI(tag, "%s", buffer);
#endif
}

void check_heap_fragmentation(void)
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DMA);
    
    app_log(DEBUG_INFO_DEFAULT_TAG, "=== DMA HEAP FRAGMENTATION ===");
    app_log(DEBUG_INFO_DEFAULT_TAG, "Total free: %u bytes", info.total_free_bytes);
    app_log(DEBUG_INFO_DEFAULT_TAG, "Largest block: %u bytes", info.largest_free_block);
    app_log(DEBUG_INFO_DEFAULT_TAG, "Min free block: %u bytes", info.minimum_free_bytes);
    app_log(DEBUG_INFO_DEFAULT_TAG, "Allocated blocks: %u", info.allocated_blocks);
    app_log(DEBUG_INFO_DEFAULT_TAG, "Free blocks: %u", info.free_blocks);
    app_log(DEBUG_INFO_DEFAULT_TAG, "==============================");
    
    // I2C needs ~512 contiguous bytes – warn if largest block too small
    if (info.largest_free_block < 512) {
        app_log(DEBUG_INFO_DEFAULT_TAG, "CRITICAL: DMA heap fragmented! I2C will fail.");
    }
}