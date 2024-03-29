/**************************************************************************
*
* Copyright (C) 2009 Peter Mc Shane
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#ifndef TRENDLOG_MUL_H
#define TRENDLOG_MUL_H

#include <stdbool.h>
#include <stdint.h>
//
#include <time.h>       /* for time_t */
#include "bacdef.h"
#include "cov.h"
#include "rp.h"
#include "wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



    void Trend_Mul_Log_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    bool Trend_Mul_Log_Valid_Instance(
        uint32_t object_instance);
    unsigned Trend_Mul_Log_Count(
        void);
    uint32_t Trend_Mul_Log_Index_To_Instance(
        unsigned index);
    unsigned Trend_Mul_Log_Instance_To_Index(
        uint32_t instance);
    bool Trend_Mul_Log_Object_Instance_Add(
        uint32_t instance);

    bool Trend_Mul_Log_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);

//    int Trend_Log_Read_Property(
//        BACNET_READ_PROPERTY_DATA * rpdata);
int  Trend_Mul_Log_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code);

    bool Trend_Mul_Log_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);
    void Trend_Mul_Log_Init(
        void);

    void TL_Insert_Status_Rec(
        int iLog,
        BACNET_LOG_STATUS eStatus,
        bool bState);

//    bool TL_Is_Enabled(
//        int iLog);

//    time_t TL_BAC_Time_To_Local(
//        BACNET_DATE_TIME * SourceTime);

//    void TL_Local_Time_To_BAC(
//        BACNET_DATE_TIME * DestTime,
//        time_t SourceTime);

//    int TL_encode_entry(
//        uint8_t * apdu,
//        int iLog,
//        int iEntry);

//    int TL_encode_by_position(
//        uint8_t * apdu,
//        BACNET_READ_RANGE_DATA * pRequest);

//    int TL_encode_by_sequence(
//        uint8_t * apdu,
//        BACNET_READ_RANGE_DATA * pRequest);

//    int TL_encode_by_time(
//        uint8_t * apdu,
//        BACNET_READ_RANGE_DATA * pRequest);

//    bool TrendLogGetRRInfo(
//        BACNET_READ_RANGE_DATA * pRequest,      /* Info on the request */
//        RR_PROP_INFO * pInfo);  /* Where to put the information */

//    int rr_trend_log_encode(
//        uint8_t * apdu,
//        BACNET_READ_RANGE_DATA * pRequest);

//    void trend_log_timer(
//        uint16_t uSeconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
