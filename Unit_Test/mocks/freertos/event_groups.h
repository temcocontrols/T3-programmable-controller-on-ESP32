#ifndef MOCK_FREERTOS_EVENT_GROUPS_H
#define MOCK_FREERTOS_EVENT_GROUPS_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* EventGroupHandle_t;
typedef uint32_t EventBits_t;

static inline EventGroupHandle_t xEventGroupCreate(void)
{
    return (EventGroupHandle_t)1;
}

static inline EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup)
{
    (void)xEventGroup;
    return 1; // Default bit 0 set (WIFI_CONNECTED_BIT)
}

static inline EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet)
{
    (void)xEventGroup;
    return uxBitsToSet;
}

static inline EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToClear)
{
    (void)xEventGroup;
    return uxBitsToClear;
}

static inline EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToWaitFor, const BaseType_t xClearOnExit, const BaseType_t xWaitForAllBits, TickType_t xTicksToWait)
{
    (void)xEventGroup; (void)uxBitsToWaitFor; (void)xClearOnExit; (void)xWaitForAllBits; (void)xTicksToWait;
    return uxBitsToWaitFor;
}

static inline void vEventGroupDelete(EventGroupHandle_t xEventGroup) { (void)xEventGroup; }

#ifdef __cplusplus
}
#endif

#endif // MOCK_FREERTOS_EVENT_GROUPS_H
