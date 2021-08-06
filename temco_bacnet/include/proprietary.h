#ifndef PROPRIETARY_H
#define PROPRIETARY_H


#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "rp.h"
#include "wp.h"



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    void TemcoVars_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    bool TemcoVars_Valid_Instance(
        uint32_t object_instance);
    unsigned TemcoVars_Count(
        void);
    uint32_t TemcoVars_Index_To_Instance(
        unsigned index);
//    char *TemcoVars_Name(
//        uint32_t object_instance);

    int TemcoVars_Encode_Property_APDU(
        uint8_t * apdu,
        uint32_t object_instance,
        BACNET_PROPERTY_ID property,
        uint32_t array_index,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);

    bool TemcoVars_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);

    bool TemcoVars_Present_Value_Set(
        uint32_t object_instance,
        float value,
        uint8_t priority);
    float TemcoVars_Present_Value(
        uint32_t object_instance);

//    void TemcoVars_Init(
//        void);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
