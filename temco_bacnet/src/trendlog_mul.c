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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>     /* for memmove */
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "bacnet.h"
#include "apdu.h"
#include "wp.h" /* write property handling */
#include "version.h"
#include "device.h"     /* me */
#include "handlers.h"
#include "datalink.h"
#include "address.h"
#include "bacdevobjpropref.h"
#include "trendlog_mul.h"
#if defined(BACFILE)
#include "bacfile.h"    /* object list dependency */
#endif

#if BAC_TRENDLOG
/* number of demo objects */
//#ifndef MAX_TREND_LOGS
//#define MAX_TREND_LOGS 8
//#endif

uint8_t  TRENDLOGS_MUL;

//TL_DATA_REC Logs[MAX_TREND_LOGS][TL_MAX_ENTRIES];
//static TL_LOG_INFO LogInfo[MAX_TREND_LOGS];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Trend_Mul_Log_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_ENABLE,
    PROP_STOP_WHEN_FULL,
    PROP_BUFFER_SIZE,
    PROP_LOG_BUFFER,
    PROP_RECORD_COUNT,
    PROP_TOTAL_RECORD_COUNT,
    PROP_EVENT_STATE,
    PROP_LOGGING_TYPE,
    PROP_STATUS_FLAGS,
    -1
};

static const int Trend_Mul_Log_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_START_TIME,
    PROP_STOP_TIME,
    PROP_LOG_DEVICE_OBJECT_PROPERTY,
    PROP_LOG_INTERVAL,

/* Required if COV logging supported
    PROP_COV_RESUBSCRIPTION_INTERVAL,
    PROP_CLIENT_COV_INCREMENT, */

/* Required if intrinsic reporting supported
    PROP_NOTIFICATION_THRESHOLD,
    PROP_RECORDS_SINCE_NOTIFICATION,
    PROP_LAST_NOTIFY_RECORD,
    PROP_NOTIFICATION_CLASS,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS, */

    PROP_ALIGN_INTERVALS,
    PROP_INTERVAL_OFFSET,
    PROP_TRIGGER,
    -1
};

static const int Trend_Mul_Log_Properties_Proprietary[] = {
    -1
};

void Trend_Mul_Log_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Trend_Mul_Log_Properties_Required;
    if (pOptional)
        *pOptional = Trend_Mul_Log_Properties_Optional;
    if (pProprietary)
        *pProprietary = Trend_Mul_Log_Properties_Proprietary;

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Trend_Mul_Log_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_TREND_LOGS + OBJECT_BASE) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Trend_Mul_Log_Count(
    void)
{
    //return MAX_TREND_LOGS;
	return  TRENDLOGS_MUL;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Trend_Mul_Log_Index_To_Instance(
    unsigned index)
{
 //   return index;
	 return index + OBJECT_BASE;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Trend_Mul_Log_Instance_To_Index(
    uint32_t object_instance)
{
//    unsigned index = MAX_TREND_LOGS;

//    if (object_instance < MAX_TREND_LOGS) {
//        index = object_instance;
//    }

//    return index;
	return object_instance - OBJECT_BASE; 
}

/*
 * Things to do when starting up the stack for Trend Logs.
 * Should be called whenever we reset the device or power it up
 */
//void Trend_Mul_Log_Init(
//    void)
//{
//    static bool initialized = false;
//    int iLog;
//    int iEntry;
///*    struct tm TempTime;*/
//		UN_Time	TempTime;
//    time_t tClock;
//    if (!initialized) {
//        initialized = true;
//        /* initialize all the values */

//        for (iLog = 0; iLog < MAX_TREND_LOGS; iLog++) {
//            /*
//             * Do we need to do anything here?
//             * Trend logs are usually assumed to survive over resets
//             * and are frequently implemented using Battery Backed RAM
//             * If they are implemented using Flash or SD cards or some
//             * such mechanism there may be some RAM based setup needed
//             * for log management purposes.
//             * We probably need to look at inserting LOG_INTERRUPTED
//             * entries into any active logs if the power down or reset
//             * may have caused us to miss readings.
//             */

//            /* We will just fill the logs with some entries for testing
//             * purposes.
//             */
//            TempTime.Clk.year = 17;
//            TempTime.Clk.mon = 12; /* Different month for each log */
//            TempTime.Clk.day = 1;
//            TempTime.Clk.hour = 15;
//            TempTime.Clk.min = 0;
//            TempTime.Clk.sec = 0;
//            tClock = my_mktime(&TempTime);//mktime(&TempTime);
//						
//						
//            for (iEntry = 0; iEntry < TL_MAX_ENTRIES; iEntry++) {
//                Logs[iLog][iEntry].tTimeStamp = tClock;
//                Logs[iLog][iEntry].ucRecType = TL_TYPE_REAL;
//                Logs[iLog][iEntry].Datum.fReal =
//                    (float) (iEntry + (iLog * TL_MAX_ENTRIES));
//                /* Put status flags with every second log */
//                if ((iLog & 1) == 0)
//                    Logs[iLog][iEntry].ucStatus = 128;
//                else
//                    Logs[iLog][iEntry].ucStatus = 0;
//                tClock += 10;//900;  /* advance 15 minutes */
//            }

//            LogInfo[iLog].tLastDataTime = tClock - 10;//900;
//            LogInfo[iLog].bAlignIntervals = true;
//            LogInfo[iLog].bEnable = true;
//            LogInfo[iLog].bStopWhenFull = false;
//            LogInfo[iLog].bTrigger = false;
//            LogInfo[iLog].LoggingType = LOGGING_TYPE_POLLED;
//            LogInfo[iLog].Source.arrayIndex = 0;
//            LogInfo[iLog].ucTimeFlags = 0;
//            LogInfo[iLog].ulIntervalOffset = 0;
//            LogInfo[iLog].iIndex = 0;
//            LogInfo[iLog].ulLogInterval = 10;//900;
//            LogInfo[iLog].ulRecordCount = 0;//TL_MAX_ENTRIES;
//            LogInfo[iLog].ulTotalRecordCount = 100;//10000;

//            LogInfo[iLog].Source.deviceIndentifier.instance =
//                Device_Object_Instance_Number();
//            LogInfo[iLog].Source.deviceIndentifier.type = OBJECT_DEVICE;
//						
//            LogInfo[iLog].Source.objectIdentifier.instance = iLog + 1;
//            LogInfo[iLog].Source.objectIdentifier.type = OBJECT_ANALOG_INPUT;
//            LogInfo[iLog].Source.arrayIndex = BACNET_ARRAY_ALL;
//            LogInfo[iLog].Source.propertyIdentifier = PROP_PRESENT_VALUE;

//            datetime_set_values(&LogInfo[iLog].StartTime, 2017, 11, 25, 0, 0, 0,
//                0);
//            LogInfo[iLog].tStartTime =
//                TL_BAC_Time_To_Local(&LogInfo[iLog].StartTime);
//            datetime_set_values(&LogInfo[iLog].StopTime, 2020, 11, 22, 23, 59,
//                59, 99);
//            LogInfo[iLog].tStopTime =
//                TL_BAC_Time_To_Local(&LogInfo[iLog].StopTime);							

//						
//        }
//    }

//    return;
//}


/*
 * Note: we use the instance number here and build the name based
 * on the assumption that there is a 1 to 1 correspondance. If there
 * is not we need to convert to index before proceeding.
 */
//bool Trend_Log_Object_Name(
//    uint32_t object_instance,
//    BACNET_CHARACTER_STRING * object_name)
//{
//    static char text_string[32] = "";   /* okay for single thread */
//    bool status = false;

//    if (object_instance < MAX_TREND_LOGS) {
//        sprintf(text_string, "Trend Log %u", object_instance);
//        status = characterstring_init_ansi(object_name, text_string);
//    }

//    return status;
//}


/* return the length of the apdu encoded or BACNET_STATUS_ERROR for error or
   BACNET_STATUS_ABORT for abort message */
//int Trend_Log_Read_Property(
//    BACNET_READ_PROPERTY_DATA * rpdata)
int  Trend_Mul_Log_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;   /* return value */
    int len = 0;        /* apdu len intermediate value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
//    TL_MUL_LOG_INFO *CurrentLog;
	  unsigned  object_index;
 //   uint8_t *apdu = NULL;

//    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
//        (rpdata->application_data_len == 0)) {
//        return 0;
//    }
//    apdu = rpdata->application_data;
	
		object_index = Trend_Log_Instance_To_Index(object_instance);
//    CurrentLog = &LogInfo[object_index];        /* Pin down which log to look at */
//		CurrentLog = get_trendlog_mul(object_index);
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_TREND_LOG_MULTIPLE,
                object_index);
            break;

        case PROP_DESCRIPTION:
        case PROP_OBJECT_NAME:
            //Trend_Log_Object_Name(object_index, &char_string);
						characterstring_init_ansi(&char_string, get_label(TRENDLOG_MUL,object_index));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_TREND_LOG_MULTIPLE);
            break;

        case PROP_ENABLE:
            apdu_len =
                encode_application_boolean(&apdu[0], Trendlog_Enable()/*CurrentLog->bEnable*/);
            break;

//        case PROP_STOP_WHEN_FULL:
//            apdu_len =
//                encode_application_boolean(&apdu[0],
//                CurrentLog->bStopWhenFull);
//            break;

        case PROP_BUFFER_SIZE:
            apdu_len = encode_application_unsigned(&apdu[0], TL_MAX_ENTRIES);
            break;

        case PROP_LOG_BUFFER:
            /* You can only read the buffer via the ReadRange service */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_READ_ACCESS_DENIED;
            apdu_len = BACNET_STATUS_ERROR;
            break;

        case PROP_RECORD_COUNT:
            apdu_len +=
                encode_application_unsigned(&apdu[0],
                CurrentLog->ulRecordCount);
            break;

        case PROP_TOTAL_RECORD_COUNT:
            apdu_len +=
                encode_application_unsigned(&apdu[0],
                CurrentLog->ulTotalRecordCount);
            break;

        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;

        case PROP_LOGGING_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0],
                CurrentLog->LoggingType);
            break;

        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_START_TIME:
            len =
                encode_application_date(&apdu[0], &CurrentLog->StartTime.date);
            apdu_len = len;
            len =
                encode_application_time(&apdu[apdu_len],
                &CurrentLog->StartTime.time);
            apdu_len += len;
				
            break;

        case PROP_STOP_TIME:
            len =
                encode_application_date(&apdu[0], &CurrentLog->StopTime.date);
            apdu_len = len;
            len =
                encode_application_time(&apdu[apdu_len],
                &CurrentLog->StopTime.time);
            apdu_len += len;
            break;

        case PROP_LOG_DEVICE_OBJECT_PROPERTY:
            /*
             * BACnetDeviceObjectPropertyReference ::= SEQUENCE {
             *     objectIdentifier   [0] BACnetObjectIdentifier,
             *     propertyIdentifier [1] BACnetPropertyIdentifier,
             *     propertyArrayIndex [2] Unsigned OPTIONAL, -- used only with array datatype
             *                                               -- if omitted with an array then
             *                                               -- the entire array is referenced
             *     deviceIdentifier   [3] BACnetObjectIdentifier OPTIONAL
             * }
             */
            apdu_len +=
                bacapp_encode_device_obj_property_ref(&apdu[0],
                &CurrentLog->Source);
            break;

        case PROP_LOG_INTERVAL:
            /* We only log to 1 sec accuracy so must multiply by 100 before passing it on */
            apdu_len +=
                encode_application_unsigned(&apdu[0],
                CurrentLog->ulLogInterval * 100);
            break;

        case PROP_ALIGN_INTERVALS:
            apdu_len =
                encode_application_boolean(&apdu[0],
                CurrentLog->bAlignIntervals);
            break;

        case PROP_INTERVAL_OFFSET:
            /* We only log to 1 sec accuracy so must multiply by 100 before passing it on */
            apdu_len +=
                encode_application_unsigned(&apdu[0],
                CurrentLog->ulIntervalOffset * 100);
            break;

        case PROP_TRIGGER:
            apdu_len =
                encode_application_boolean(&apdu[0], CurrentLog->bTrigger);
            break;

        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (property != PROP_EVENT_TIME_STAMPS)
        && (array_index != BACNET_ARRAY_ALL)) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Trend_Log_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    int len = 0;
    int iOffset = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    TL_LOG_INFO *CurrentLog;
    BACNET_DATE TempDate;       /* build here in case of error in time half of datetime */
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE TempSource;
    bool bEffectiveEnable;
    int log_index;

    /* Pin down which log to look at */
    log_index = Trend_Log_Instance_To_Index(wp_data->object_instance);
    CurrentLog = &LogInfo[log_index];

    /* decode the some of the request */
    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    if ((wp_data->object_property != PROP_EVENT_TIME_STAMPS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_ENABLE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                /* Section 12.25.5 can't enable a full log with stop when full set */
                if ((CurrentLog->bEnable == false) &&
                    (CurrentLog->bStopWhenFull == true) &&
                    (CurrentLog->ulRecordCount == TL_MAX_ENTRIES) &&
                    (value.type.Boolean == true)) {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_OBJECT;
                    wp_data->error_code = ERROR_CODE_LOG_BUFFER_FULL;
                    break;
                }

                /* Only trigger this validation on a potential change of state */
                if (CurrentLog->bEnable != value.type.Boolean) {
                    bEffectiveEnable = TL_Is_Enabled(log_index);
                    CurrentLog->bEnable = value.type.Boolean;
                    /* To do: what actions do we need to take on writing ? */
                    if (value.type.Boolean == false) {
                        if (bEffectiveEnable == true) {
                            /* Only insert record if we really were
                               enabled i.e. times and enable flags */
                            TL_Insert_Status_Rec(log_index,
                                LOG_STATUS_LOG_DISABLED, true);
                        }
                    } else {
                        if (TL_Is_Enabled(log_index)) {
                            /* Have really gone from disabled to enabled as
                             * enable flag and times were correct
                             */
                            TL_Insert_Status_Rec(log_index,
                                LOG_STATUS_LOG_DISABLED, false);
                        }
                    }
                }
            }
            break;

        case PROP_STOP_WHEN_FULL:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                /* Only trigger this on a change of state */
                if (CurrentLog->bStopWhenFull != value.type.Boolean) {
                    CurrentLog->bStopWhenFull = value.type.Boolean;

                    if ((value.type.Boolean == true) &&
                        (CurrentLog->ulRecordCount == TL_MAX_ENTRIES) &&
                        (CurrentLog->bEnable == true)) {

                        /* When full log is switched from normal to stop when full
                         * disable the log and record the fact - see 135-2008 12.25.12
                         */
                        CurrentLog->bEnable = false;
                        TL_Insert_Status_Rec(log_index,
                            LOG_STATUS_LOG_DISABLED, true);
                    }
                }
            }
            break;

        case PROP_BUFFER_SIZE:
            /* Fixed size buffer so deny write. If buffer size was writable
             * we would probably erase the current log, resize, re-initalise
             * and carry on - however write is not allowed if enable is true.
             */
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;

        case PROP_RECORD_COUNT:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                if (value.type.Unsigned_Int == 0) {
                    /* Time to clear down the log */
                    CurrentLog->ulRecordCount = 0;
                    CurrentLog->iIndex = 0;
                    TL_Insert_Status_Rec(log_index, LOG_STATUS_BUFFER_PURGED,
                        true);
                }
            }
            break;

        case PROP_LOGGING_TYPE:
            /* logic
             * triggered and polled options.
             */
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_ENUMERATED,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                if (value.type.Enumerated != LOGGING_TYPE_COV) {
                    CurrentLog->LoggingType = value.type.Enumerated;
                    if (value.type.Enumerated == LOGGING_TYPE_POLLED) {
                        /* As per 12.25.27 pick a suitable default if interval is 0 */
                        if (CurrentLog->ulLogInterval == 0) {
                            CurrentLog->ulLogInterval = 900;
                        }
                    }
                    if (value.type.Enumerated == LOGGING_TYPE_TRIGGERED) {
                        /* As per 12.25.27 0 the interval if triggered logging selected */
                        CurrentLog->ulLogInterval = 0;
                    }
                } else {
                    /* We don't currently support COV */
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code =
                        ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                }
            }
            break;

        case PROP_START_TIME:
            /* Copy the date part to safe place */
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_DATE,
                &wp_data->error_class, &wp_data->error_code);
            if (!status) {
                break;
            }
            TempDate = value.type.Date;
            /* Then decode the time part */
            len =
                bacapp_decode_application_data(wp_data->application_data + len,
                wp_data->application_data_len - len, &value);

            if (len) {
                status =
                    WPValidateArgType(&value, BACNET_APPLICATION_TAG_TIME,
                    &wp_data->error_class, &wp_data->error_code);
                if (!status) {
                    break;
                }
                /* First record the current enable state of the log */
                bEffectiveEnable = TL_Is_Enabled(log_index);
                CurrentLog->StartTime.date = TempDate;  /* Safe to copy the date now */
                CurrentLog->StartTime.time = value.type.Time;

                if (datetime_wildcard_present(&CurrentLog->StartTime)) {
                    /* Mark start time as wild carded */
                    CurrentLog->ucTimeFlags |= TL_T_START_WILD;
                    CurrentLog->tStartTime = 0;
                } else {
                    /* Clear wild card flag and set time in local format */
                    CurrentLog->ucTimeFlags &= ~TL_T_START_WILD;
                    CurrentLog->tStartTime =
                        TL_BAC_Time_To_Local(&CurrentLog->StartTime);
                }

                if (bEffectiveEnable != TL_Is_Enabled(log_index)) {
                    /* Enable status has changed because of time update */
                    if (bEffectiveEnable == true) {
                        /* Say we went from enabled to disabled */
                        TL_Insert_Status_Rec(log_index,
                            LOG_STATUS_LOG_DISABLED, true);
                    } else {
                        /* Say we went from disabled to enabled */
                        TL_Insert_Status_Rec(log_index,
                            LOG_STATUS_LOG_DISABLED, false);
                    }
                }
            }
            break;

        case PROP_STOP_TIME:
            /* Copy the date part to safe place */
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_DATE,
                &wp_data->error_class, &wp_data->error_code);
            if (!status) {
                break;
            }
            TempDate = value.type.Date;
            /* Then decode the time part */
            len =
                bacapp_decode_application_data(wp_data->application_data + len,
                wp_data->application_data_len - len, &value);

            if (len) {
                status =
                    WPValidateArgType(&value, BACNET_APPLICATION_TAG_TIME,
                    &wp_data->error_class, &wp_data->error_code);
                if (!status) {
                    break;
                }
                /* First record the current enable state of the log */
                bEffectiveEnable = TL_Is_Enabled(log_index);
                CurrentLog->StopTime.date = TempDate;   /* Safe to copy the date now */
                CurrentLog->StopTime.time = value.type.Time;

                if (datetime_wildcard_present(&CurrentLog->StopTime)) {
                    /* Mark stop time as wild carded */
                    CurrentLog->ucTimeFlags |= TL_T_STOP_WILD;
                    CurrentLog->tStopTime = 0xFFFFFFFF; /* Fixme: how do we set this to max for time_t ? */
                } else {
                    /* Clear wild card flag and set time in local format */
                    CurrentLog->ucTimeFlags &= ~TL_T_STOP_WILD;
                    CurrentLog->tStopTime =
                        TL_BAC_Time_To_Local(&CurrentLog->StopTime);
                }

                if (bEffectiveEnable != TL_Is_Enabled(log_index)) {
                    /* Enable status has changed because of time update */
                    if (bEffectiveEnable == true) {
                        /* Say we went from enabled to disabled */
                        TL_Insert_Status_Rec(log_index,
                            LOG_STATUS_LOG_DISABLED, true);
                    } else {
                        /* Say we went from disabled to enabled */
                        TL_Insert_Status_Rec(log_index,
                            LOG_STATUS_LOG_DISABLED, false);
                    }
                }
            }
            break;

        case PROP_LOG_DEVICE_OBJECT_PROPERTY:
            memset(&TempSource, 0, sizeof(TempSource)); /* Start with clean sheet */
            TempSource.arrayIndex = BACNET_ARRAY_ALL;   /* Need this so if no array index set we read properties in full */

            /* First up is the object ID */
            len =
                bacapp_decode_context_data(wp_data->application_data,
                wp_data->application_data_len, &value,
                PROP_LOG_DEVICE_OBJECT_PROPERTY);
            if ((len == 0) || (value.context_tag != 0) ||
                ((wp_data->application_data_len - len) == 0)) {
                /* Bad decode, wrong tag or following required parameter missing */
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_OTHER;
                break;
            }

            TempSource.objectIdentifier = value.type.Object_Id;
            wp_data->application_data_len -= len;
            iOffset = len;
            /* Second up is the property id */
            len =
                bacapp_decode_context_data(&wp_data->application_data[iOffset],
                wp_data->application_data_len, &value,
                PROP_LOG_DEVICE_OBJECT_PROPERTY);
            if ((len == 0) || (value.context_tag != 1)) {
                /* Bad decode or wrong tag */
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_OTHER;
                break;
            }

            TempSource.propertyIdentifier = value.type.Enumerated;
            wp_data->application_data_len -= len;

            /* If there is still more to come */
            if (wp_data->application_data_len != 0) {
                iOffset += len;
                len =
                    bacapp_decode_context_data(&wp_data->application_data
                    [iOffset], wp_data->application_data_len, &value,
                    PROP_LOG_DEVICE_OBJECT_PROPERTY);
                if ((len == 0) || ((value.context_tag != 2) &&
                        (value.context_tag != 3))) {
                    /* Bad decode or wrong tag */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_OTHER;
                    break;
                }

                if (value.context_tag == 2) {
                    /* Got an index so deal with it */
                    TempSource.arrayIndex = value.type.Unsigned_Int;
                    wp_data->application_data_len -= len;
                    /* Still some remaining so fetch potential device ID */
                    if (wp_data->application_data_len != 0) {
                        iOffset += len;
                        len =
                            bacapp_decode_context_data
                            (&wp_data->application_data[iOffset],
                            wp_data->application_data_len, &value,
                            PROP_LOG_DEVICE_OBJECT_PROPERTY);
                        if ((len == 0) || (value.context_tag != 3)) {
                            /* Bad decode or wrong tag */
                            wp_data->error_class = ERROR_CLASS_PROPERTY;
                            wp_data->error_code = ERROR_CODE_OTHER;
                            break;
                        }
                    }
                }

                if (value.context_tag == 3) {
                    /* Got a device ID so deal with it */
                    TempSource.deviceIndentifier = value.type.Object_Id;
                    if ((TempSource.deviceIndentifier.instance !=
                            Device_Object_Instance_Number()) ||
                        (TempSource.deviceIndentifier.type != OBJECT_DEVICE)) {
                        /* Not our ID so can't handle it at the moment */
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code =
                            ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                        break;
                    }
                }
            }
            /* Make sure device ID is set to ours in case not supplied */
            TempSource.deviceIndentifier.type = OBJECT_DEVICE;
            TempSource.deviceIndentifier.instance =
                Device_Object_Instance_Number();
            /* Quick comparison if structures are packed ... */
            if (memcmp(&TempSource, &CurrentLog->Source,
                    sizeof(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE)) != 0) {
                /* Clear buffer if property being logged is changed */
                CurrentLog->ulRecordCount = 0;
                CurrentLog->iIndex = 0;
                TL_Insert_Status_Rec(log_index, LOG_STATUS_BUFFER_PURGED,
                    true);
            }
            CurrentLog->Source = TempSource;
            status = true;
            break;

        case PROP_LOG_INTERVAL:
            if (CurrentLog->LoggingType == LOGGING_TYPE_TRIGGERED) {
                /* Read only if triggered log so flag error and bail out */
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                break;
            }
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                if ((CurrentLog->LoggingType == LOGGING_TYPE_POLLED) &&
                    (value.type.Unsigned_Int == 0)) {
                    /* We don't support COV at the moment so don't allow switching
                     * to it by clearing interval whilst in polling mode */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code =
                        ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                    status = false;
                } else {
                    /* We only log to 1 sec accuracy so must divide by 100 before passing it on */
                    CurrentLog->ulLogInterval = value.type.Unsigned_Int / 100;
                }
            }
            break;

        case PROP_ALIGN_INTERVALS:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                CurrentLog->bAlignIntervals = value.type.Boolean;
            }
            break;

        case PROP_INTERVAL_OFFSET:
            /* We only log to 1 sec accuracy so must divide by 100 before passing it on */
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                CurrentLog->ulIntervalOffset = value.type.Unsigned_Int / 100;
            }
            break;

        case PROP_TRIGGER:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                /* We will not allow triggered operation if polling with aligning
                 * to the clock as that will produce non aligned readings which
                 * goes against the reason for selscting this mode
                 */
                if ((CurrentLog->LoggingType == LOGGING_TYPE_POLLED) &&
                    (CurrentLog->bAlignIntervals == true)) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code =
                        ERROR_CODE_NOT_CONFIGURED_FOR_TRIGGERED_LOGGING;
                    status = false;
                } else {
                    CurrentLog->bTrigger = value.type.Boolean;
                }
            }
            break;

        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return status;
}

bool TrendLogGetRRInfo(
    BACNET_READ_RANGE_DATA * pRequest,  /* Info on the request */
    RR_PROP_INFO * pInfo)
{       /* Where to put the information */
    int log_index;

    log_index = Trend_Log_Instance_To_Index(pRequest->object_instance);
    if (log_index >= MAX_TREND_LOGS) {
        pRequest->error_class = ERROR_CLASS_OBJECT;
        pRequest->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    } else if (pRequest->object_property == PROP_LOG_BUFFER) {
        pInfo->RequestTypes = RR_BY_POSITION | RR_BY_TIME | RR_BY_SEQUENCE;
        pInfo->Handler = rr_trend_log_encode;
        return (true);
    } else {
        pRequest->error_class = ERROR_CLASS_SERVICES;
        pRequest->error_code = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
    }

    return (false);
}

/*****************************************************************************
 * Insert a status record into a trend log - does not check for enable/log   *
 * full, time slots and so on as these type of entries have to go in         *
 * irrespective of such things which means that valid readings may get       *
 * pushed out of the log to make room.                                       *
 *****************************************************************************/

void TL_Insert_Status_Rec(
    int iLog,
    BACNET_LOG_STATUS eStatus,
    bool bState)
{
    TL_LOG_INFO *CurrentLog;
    TL_DATA_REC TempRec;

    CurrentLog = &LogInfo[iLog];
    TempRec.tTimeStamp = get_current_time();
    TempRec.ucRecType = TL_TYPE_STATUS;
    TempRec.ucStatus = 0;
    TempRec.Datum.ucLogStatus = 0;
    /* Note we set the bits in correct order so that we can place them directly
     * into the bitstring structure later on when we have to encode them */
    switch (eStatus) {
        case LOG_STATUS_LOG_DISABLED:
            if (bState)
                TempRec.Datum.ucLogStatus = 1 << LOG_STATUS_LOG_DISABLED;
            break;
        case LOG_STATUS_BUFFER_PURGED:
            if (bState)
                TempRec.Datum.ucLogStatus = 1 << LOG_STATUS_BUFFER_PURGED;
            break;
        case LOG_STATUS_LOG_INTERRUPTED:
            TempRec.Datum.ucLogStatus = 1 << LOG_STATUS_LOG_INTERRUPTED;
            break;
        default:
            break;
    }

    Logs[iLog][CurrentLog->iIndex++] = TempRec;
    if (CurrentLog->iIndex >= TL_MAX_ENTRIES)
        CurrentLog->iIndex = 0;

    CurrentLog->ulTotalRecordCount++;

    if (CurrentLog->ulRecordCount < TL_MAX_ENTRIES)
        CurrentLog->ulRecordCount++;
}

/*****************************************************************************
 * Use the combination of the enable flag and the enable times to determine  *
 * if the log is really enabled now. See 135-2008 sections 12.25.5 - 12.25.7 *
 *****************************************************************************/

bool TL_Is_Enabled(
    int iLog)
{
    TL_LOG_INFO *CurrentLog;
    time_t tNow;
    bool bStatus;

    bStatus = true;
    CurrentLog = &LogInfo[iLog];
#if 0
    printf("\nFlags - %u, Start - %u, Stop - %u\n",
        (unsigned int) CurrentLog->ucTimeFlags,
        (unsigned int) CurrentLog->tStartTime,
        (unsigned int) CurrentLog->tStopTime);
#endif
    if (CurrentLog->bEnable == false) { 
        /* Not enabled so time is irrelevant */
        bStatus = false;
    } else if ((CurrentLog->ucTimeFlags == 0) &&
        (CurrentLog->tStopTime < CurrentLog->tStartTime)) {
        /* Start time was after stop time as per 12.25.6 and 12.25.7 */
        bStatus = false;
    } else if (CurrentLog->ucTimeFlags != (TL_T_START_WILD | TL_T_STOP_WILD)) {
        /* enabled and either 1 wild card or none */
        tNow = get_current_time();
#if 0
        printf("\nFlags - %u, Current - %u, Start - %u, Stop - %u\n",
            (unsigned int) CurrentLog->ucTimeFlags, (unsigned int) Now,
            (unsigned int) CurrentLog->tStartTime,
            (unsigned int) CurrentLog->tStopTime);
#endif
        if ((CurrentLog->ucTimeFlags & TL_T_START_WILD) != 0) {
            /* wild card start time */
            if (tNow > CurrentLog->tStopTime)
                bStatus = false;
        } else if ((CurrentLog->ucTimeFlags & TL_T_STOP_WILD) != 0) {
            /* wild card stop time */
            if (tNow < CurrentLog->tStartTime)
                bStatus = false;
        } else {
#if 0
            printf("\nCurrent - %u, Start - %u, Stop - %u\n",
                (unsigned int) tNow, (unsigned int) CurrentLog->tStartTime,
                (unsigned int) CurrentLog->tStopTime);
#endif
								
            /* No wildcards so use both times */
            if ((tNow < CurrentLog->tStartTime) ||
                (tNow > CurrentLog->tStopTime))
                bStatus = false;
        }
    }

    return (bStatus);
}

/*****************************************************************************
 * Convert a BACnet time into a local time in seconds since the local epoch  *
 *****************************************************************************/

time_t TL_BAC_Time_To_Local(
    BACNET_DATE_TIME * SourceTime)
{
/*    struct tm LocalTime;*/
		UN_Time LocalTime;
    int iTemp;

    LocalTime.Clk.year = SourceTime->date.year - 2000;   /* We store BACnet year in full format */
    /* Some clients send a date of all 0s to indicate start of epoch
     * even though this is not a valid date. Pick this up here and
     * correct the day and month for the local time functions.
     */
    iTemp =
        SourceTime->date.year + SourceTime->date.month + SourceTime->date.day;
    if (iTemp == 1900) {
        LocalTime.Clk.mon = 0;
        LocalTime.Clk.day = 1;
    } else {
        LocalTime.Clk.mon = SourceTime->date.month;// - 1;
        LocalTime.Clk.day = SourceTime->date.day;
    }

    LocalTime.Clk.hour = SourceTime->time.hour;
    LocalTime.Clk.min = SourceTime->time.min;
    LocalTime.Clk.sec = SourceTime->time.sec;

    return my_mktime(&LocalTime);  
}

/*****************************************************************************
 * Convert a local time in seconds since the local epoch into a BACnet time  *
 *****************************************************************************/
void Get_Time_by_sec(u32 sec_time,UN_Time *  rtc);

void TL_Local_Time_To_BAC(
    BACNET_DATE_TIME * DestTime,
    time_t SourceTime)
{
    /*struct tm *TempTime;		
    TempTime = localtime(&SourceTime);*/
	
		UN_Time TempTime;
	  Get_Time_by_sec(SourceTime,&TempTime);
	
		DestTime->date.year = (uint16_t) (TempTime.Clk.year + 2000);
    DestTime->date.month = (uint8_t) (TempTime.Clk.mon);
    DestTime->date.day = (uint8_t) TempTime.Clk.day;
    /* BACnet is 1 to 7 = Monday to Sunday
     * Windows is days from Sunday 0 - 6 so we
     * have to adjust */
    if(TempTime.Clk.week == 0)
        DestTime->date.wday = 7;
    else
        DestTime->date.wday = (uint8_t) TempTime.Clk.week;
    DestTime->time.hour = (uint8_t) TempTime.Clk.hour;
    DestTime->time.min = (uint8_t) TempTime.Clk.min;
    DestTime->time.sec = (uint8_t) TempTime.Clk.sec;
    DestTime->time.hundredths = 0;
}

/****************************************************************************
 * Build a list of Trend Log entries from the Log Buffer property as        *
 * required for the ReadsRange functionality.                               *
 *                                                                          *
 * We have to support By Position, By Sequence and By Time requests.        *
 *                                                                          *
 * We do assume the list cannot change whilst we are accessing it so would  *
 * not be multithread safe if there are other tasks that write to the log.  *
 *                                                                          *
 * We take the simple approach here to filling the buffer by taking a max   *
 * size for a single entry and then stopping if there is less than that     *
 * left in the buffer. You could build each entry in a seperate buffer and  *
 * determine the exact length before copying but this is time consuming,    *
 * requires more memory and would probably only let you sqeeeze one more    *
 * entry in on occasion. The value is calculated as 10 bytes for the time   *
 * stamp + 6 bytes for our largest data item (bit string capped at 32 bits) *
 * + 3 bytes for the status flags + 4 for the context tags to give 23.      *
 ****************************************************************************/

#define TL_MAX_ENC 23   /* Maximum size of encoded log entry, see above */

int rr_trend_log_encode(
    uint8_t * apdu,
    BACNET_READ_RANGE_DATA * pRequest)
{
    /* Initialise result flags to all false */
    bitstring_init(&pRequest->ResultFlags);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, false);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, false);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, false);
    pRequest->ItemCount = 0;    /* Start out with nothing */
    /* Bail out now if nowt - should never happen for a Trend Log but ... */
    if (LogInfo[Trend_Log_Instance_To_Index(pRequest->
                object_instance)].ulRecordCount == 0)
        return (0);

    if ((pRequest->RequestType == RR_BY_POSITION) ||
        (pRequest->RequestType == RR_READ_ALL))
        return (TL_encode_by_position(apdu, pRequest));
    else if (pRequest->RequestType == RR_BY_SEQUENCE)
        return (TL_encode_by_sequence(apdu, pRequest));

    return (TL_encode_by_time(apdu, pRequest));
}

/****************************************************************************
 * Handle encoding for the By Position and All options.                     *
 * Does All option by converting to a By Position request starting at index *
 * 1 and of maximum log size length.                                        *
 ****************************************************************************/

int TL_encode_by_position(
    uint8_t * apdu,
    BACNET_READ_RANGE_DATA * pRequest)
{
    int log_index = 0;
    int iLen = 0;
    int32_t iTemp = 0;
    TL_LOG_INFO *CurrentLog = NULL;

    uint32_t uiIndex = 0;       /* Current entry number */
    uint32_t uiFirst = 0;       /* Entry number we started encoding from */
    uint32_t uiLast = 0;        /* Entry number we finished encoding on */
    uint32_t uiTarget = 0;      /* Last entry we are required to encode */
    uint32_t uiRemaining = 0;   /* Amount of unused space in packet */

    /* See how much space we have */
    uiRemaining = MAX_APDU - pRequest->Overhead;
    log_index = Trend_Log_Instance_To_Index(pRequest->object_instance);
    CurrentLog = &LogInfo[log_index];
    if (pRequest->RequestType == RR_READ_ALL) {
        /*
         * Read all the list or as much as will fit in the buffer by selecting
         * a range that covers the whole list and falling through to the next
         * section of code
         */
        pRequest->Count = CurrentLog->ulRecordCount;    /* Full list */
        pRequest->Range.RefIndex = 1;   /* Starting at the beginning */
    }

    if (pRequest->Count < 0) {  /* negative count means work from index backwards */
        /*
         * Convert from end index/negative count to
         * start index/positive count and then process as
         * normal. This assumes that the order to return items
         * is always first to last, if this is not true we will
         * have to handle this differently.
         *
         * Note: We need to be careful about how we convert these
         * values due to the mix of signed and unsigned types - don't
         * try to optimise the code unless you understand all the
         * implications of the data type conversions!
         */

        iTemp = pRequest->Range.RefIndex;       /* pull out and convert to signed */
        iTemp += pRequest->Count + 1;   /* Adjust backwards, remember count is -ve */
        if (iTemp < 1) {        /* if count is too much, return from 1 to start index */
            pRequest->Count = pRequest->Range.RefIndex;
            pRequest->Range.RefIndex = 1;
        } else {        /* Otherwise adjust the start index and make count +ve */
            pRequest->Range.RefIndex = iTemp;
            pRequest->Count = -pRequest->Count;
        }
    }

    /* From here on in we only have a starting point and a positive count */
    if (pRequest->Range.RefIndex > CurrentLog->ulRecordCount)   /* Nothing to return as we are past the end of the list */
        return (0);
    uiTarget = pRequest->Range.RefIndex + pRequest->Count - 1;  /* Index of last required entry */
    if (uiTarget > CurrentLog->ulRecordCount)   /* Capped at end of list if necessary */
        uiTarget = CurrentLog->ulRecordCount;

    uiIndex = pRequest->Range.RefIndex;
    uiFirst = uiIndex;  /* Record where we started from */
    while (uiIndex <= uiTarget) {
        if (uiRemaining < TL_MAX_ENC) {
            /*
             * Can't fit any more in! We just set the result flag to say there
             * was more and drop out of the loop early
             */
            bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS,
                true);
            break;
        }
        iTemp = TL_encode_entry(&apdu[iLen], log_index, uiIndex);

        uiRemaining -= iTemp;   /* Reduce the remaining space */
        iLen += iTemp;  /* and increase the length consumed */
        uiLast = uiIndex;       /* Record the last entry encoded */
        uiIndex++;      /* and get ready for next one */
        pRequest->ItemCount++;  /* Chalk up another one for the response count */
    }

    /* Set remaining result flags if necessary */
    if (uiFirst == 1)
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM,
            true);

    if (uiLast == CurrentLog->ulRecordCount)
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);
    return (iLen);
}


/****************************************************************************
 * Handle encoding for the By Sequence option.                              *
 * The fact that the buffer always has at least a single entry is used      *
 * implicetly in the following as we don't have to handle the case of an    *
 * empty buffer.                                                            *
 ****************************************************************************/

int TL_encode_by_sequence(
    uint8_t * apdu,
    BACNET_READ_RANGE_DATA * pRequest)
{
    int log_index = 0;
    int iLen = 0;
    int32_t iTemp = 0;
    TL_LOG_INFO *CurrentLog = NULL;

    uint32_t uiIndex = 0;       /* Current entry number */
    uint32_t uiFirst = 0;       /* Entry number we started encoding from */
    uint32_t uiLast = 0;        /* Entry number we finished encoding on */
    uint32_t uiSequence = 0;    /* Tracking sequenc number when encoding */
    uint32_t uiRemaining = 0;   /* Amount of unused space in packet */
    uint32_t uiFirstSeq = 0;    /* Sequence number for 1st record in log */

    uint32_t uiBegin = 0;       /* Starting Sequence number for request */
    uint32_t uiEnd = 0; /* Ending Sequence number for request */
    bool bWrapReq = false;      /* Has request sequence range spanned the max for uint32_t? */
    bool bWrapLog = false;      /* Has log sequence range spanned the max for uint32_t? */

    /* See how much space we have */
    uiRemaining = MAX_APDU - pRequest->Overhead;
    log_index = Trend_Log_Instance_To_Index(pRequest->object_instance);
    CurrentLog = &LogInfo[log_index];
    /* Figure out the sequence number for the first record, last is ulTotalRecordCount */
    uiFirstSeq =
        CurrentLog->ulTotalRecordCount - (CurrentLog->ulRecordCount - 1);

    /* Calculate start and end sequence numbers from request */
    if (pRequest->Count < 0) {
        uiBegin = pRequest->Range.RefSeqNum + pRequest->Count + 1;
        uiEnd = pRequest->Range.RefSeqNum;
    } else {
        uiBegin = pRequest->Range.RefSeqNum;
        uiEnd = pRequest->Range.RefSeqNum + pRequest->Count - 1;
    }
    /* See if we have any wrap around situations */
    if (uiBegin > uiEnd)
        bWrapReq = true;
    if (uiFirstSeq > CurrentLog->ulTotalRecordCount)
        bWrapLog = true;

    if ((bWrapReq == false) && (bWrapLog == false)) {   /* Simple case no wraps */
        /* If no overlap between request range and buffer contents bail out */
        if ((uiEnd < uiFirstSeq) || (uiBegin > CurrentLog->ulTotalRecordCount))
            return (0);

        /* Truncate range if necessary so it is guaranteed to lie
         * between the first and last sequence numbers in the buffer
         * inclusive.
         */
        if (uiBegin < uiFirstSeq)
            uiBegin = uiFirstSeq;

        if (uiEnd > CurrentLog->ulTotalRecordCount)
            uiEnd = CurrentLog->ulTotalRecordCount;
    } else {    /* There are wrap arounds to contend with */
        /* First check for non overlap condition as it is common to all */
        if ((uiBegin > CurrentLog->ulTotalRecordCount) && (uiEnd < uiFirstSeq))
            return (0);

        if (bWrapLog == false) {        /* Only request range wraps */
            if (uiEnd < uiFirstSeq) {
                uiEnd = CurrentLog->ulTotalRecordCount;
                if (uiBegin < uiFirstSeq)
                    uiBegin = uiFirstSeq;
            } else {
                uiBegin = uiFirstSeq;
                if (uiEnd > CurrentLog->ulTotalRecordCount)
                    uiEnd = CurrentLog->ulTotalRecordCount;
            }
        } else if (bWrapReq == false) { /* Only log wraps */
            if (uiBegin > CurrentLog->ulTotalRecordCount) {
                if (uiBegin > uiFirstSeq)
                    uiBegin = uiFirstSeq;
            } else {
                if (uiEnd > CurrentLog->ulTotalRecordCount)
                    uiEnd = CurrentLog->ulTotalRecordCount;
            }
        } else {        /* Both wrap */
            if (uiBegin < uiFirstSeq)
                uiBegin = uiFirstSeq;

            if (uiEnd > CurrentLog->ulTotalRecordCount)
                uiEnd = CurrentLog->ulTotalRecordCount;
        }
    }

    /* We now have a range that lies completely within the log buffer
     * and we need to figure out where that starts in the buffer.
     */
    uiIndex = uiBegin - uiFirstSeq + 1;
    uiSequence = uiBegin;
    uiFirst = uiIndex;  /* Record where we started from */
    while (uiSequence != uiEnd + 1) {
        if (uiRemaining < TL_MAX_ENC) {
            /*
             * Can't fit any more in! We just set the result flag to say there
             * was more and drop out of the loop early
             */
            bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS,
                true);
            break;
        }

        iTemp = TL_encode_entry(&apdu[iLen], log_index, uiIndex);

        uiRemaining -= iTemp;   /* Reduce the remaining space */
        iLen += iTemp;  /* and increase the length consumed */
        uiLast = uiIndex;       /* Record the last entry encoded */
        uiIndex++;      /* and get ready for next one */
        uiSequence++;
        pRequest->ItemCount++;  /* Chalk up another one for the response count */
    }

    /* Set remaining result flags if necessary */
    if (uiFirst == 1)
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM,
            true);

    if (uiLast == CurrentLog->ulRecordCount)
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);

    pRequest->FirstSequence = uiBegin;

    return (iLen);
}

/****************************************************************************
 * Handle encoding for the By Time option.                                  *
 * The fact that the buffer always has at least a single entry is used      *
 * implicetly in the following as we don't have to handle the case of an    *
 * empty buffer.                                                            *
 ****************************************************************************/

int TL_encode_by_time(
    uint8_t * apdu,
    BACNET_READ_RANGE_DATA * pRequest)
{
    int log_index = 0;
    int iLen = 0;
    int32_t iTemp = 0;
    int iCount = 0;
    TL_LOG_INFO *CurrentLog = NULL;

    uint32_t uiIndex = 0;       /* Current entry number */
    uint32_t uiFirst = 0;       /* Entry number we started encoding from */
    uint32_t uiLast = 0;        /* Entry number we finished encoding on */
    uint32_t uiRemaining = 0;   /* Amount of unused space in packet */
    uint32_t uiFirstSeq = 0;    /* Sequence number for 1st record in log */
    time_t tRefTime = 0;        /* The time from the request in local format */
    /* See how much space we have */
    uiRemaining = MAX_APDU - pRequest->Overhead;
    log_index = Trend_Log_Instance_To_Index(pRequest->object_instance);
    CurrentLog = &LogInfo[log_index];

    tRefTime = TL_BAC_Time_To_Local(&pRequest->Range.RefTime);
    /* Find correct position for oldest entry in log */
    if (CurrentLog->ulRecordCount < TL_MAX_ENTRIES)
        uiIndex = 0;
    else
        uiIndex = CurrentLog->iIndex;

    if (pRequest->Count < 0) {
        /* Start at end of log and look for record which has
         * timestamp greater than or equal to the reference.
         */
        iCount = CurrentLog->ulRecordCount - 1;
        /* Start out with the sequence number for the last record */
        uiFirstSeq = CurrentLog->ulTotalRecordCount;
        for (;;) {
            if (Logs[pRequest->object_instance][(uiIndex +
                        iCount) % TL_MAX_ENTRIES].tTimeStamp < tRefTime)
                break;

            uiFirstSeq--;
            iCount--;
            if (iCount < 0)
                return (0);
        }

        /* We have an and point for our request,
         * now work backwards to find where we should start from
         */

        pRequest->Count = -pRequest->Count;     /* Conveert to +ve count */
        /* If count would bring us back beyond the limits
         * Of the buffer then pin it to the start of the buffer
         * otherwise adjust starting point and sequence number
         * appropriately.
         */
        iTemp = pRequest->Count - 1;
        if (iTemp > iCount) {
            uiFirstSeq -= iCount;
            pRequest->Count = iCount + 1;
            iCount = 0;
        } else {
            uiFirstSeq -= iTemp;
            iCount -= iTemp;
        }
    } else {
        /* Start at beginning of log and look for 1st record which has
         * timestamp greater than the reference time.
         */
        iCount = 0;
        /* Figure out the sequence number for the first record, last is ulTotalRecordCount */
        uiFirstSeq =
            CurrentLog->ulTotalRecordCount - (CurrentLog->ulRecordCount - 1);
        for (;;) {
            if (Logs[pRequest->object_instance][(uiIndex +
                        iCount) % TL_MAX_ENTRIES].tTimeStamp > tRefTime)
                break;

            uiFirstSeq++;
            iCount++;
            if ((uint32_t) iCount == CurrentLog->ulRecordCount)
                return (0);
        }
    }

    /* We now have a starting point for the operation and a +ve count */

    uiIndex = iCount + 1;       /* Convert to BACnet 1 based reference */
    uiFirst = uiIndex;  /* Record where we started from */
    iCount = pRequest->Count;
    while (iCount != 0) {
        if (uiRemaining < TL_MAX_ENC) {
            /*
             * Can't fit any more in! We just set the result flag to say there
             * was more and drop out of the loop early
             */
            bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS,
                true);
            break;
        }

        iTemp = TL_encode_entry(&apdu[iLen], log_index, uiIndex);

        uiRemaining -= iTemp;   /* Reduce the remaining space */
        iLen += iTemp;  /* and increase the length consumed */
        uiLast = uiIndex;       /* Record the last entry encoded */
        uiIndex++;      /* and get ready for next one */
        pRequest->ItemCount++;  /* Chalk up another one for the response count */
        iCount--;       /* And finally cross another one off the requested count */

        if (uiIndex > CurrentLog->ulRecordCount)        /* Finish up if we hit the end of the log */
            break;
    }

    /* Set remaining result flags if necessary */
    if (uiFirst == 1)
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM,
            true);

    if (uiLast == CurrentLog->ulRecordCount)
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);

    pRequest->FirstSequence = uiFirstSeq;

    return (iLen);
}


int TL_encode_entry(
    uint8_t * apdu,
    int iLog,
    int iEntry)
{
    int iLen = 0;
    TL_DATA_REC *pSource = NULL;
    BACNET_BIT_STRING TempBits;
    uint8_t ucCount = 0;
    BACNET_DATE_TIME TempTime;

    /* Convert from BACnet 1 based to 0 based array index and then
     * handle wrap around of the circular buffer */

    if (LogInfo[iLog].ulRecordCount < TL_MAX_ENTRIES)
        pSource = &Logs[iLog][(iEntry - 1) % TL_MAX_ENTRIES];
    else
        pSource =
            &Logs[iLog][(LogInfo[iLog].iIndex + iEntry - 1) % TL_MAX_ENTRIES];

    iLen = 0;
    /* First stick the time stamp in with tag [0] */
    TL_Local_Time_To_BAC(&TempTime, pSource->tTimeStamp);
    iLen += bacapp_encode_context_datetime(apdu, 0, &TempTime);

    /* Next comes the actual entry with tag [1] */
    iLen += encode_opening_tag(&apdu[iLen], 1);
    /* The data entry is tagged individually [0] - [10] to indicate which type */
    switch (pSource->ucRecType) {
        case TL_TYPE_STATUS:
            /* Build bit string directly from the stored octet */
            bitstring_init(&TempBits);
            bitstring_set_bits_used(&TempBits, 1, 5);
            bitstring_set_octet(&TempBits, 0, pSource->Datum.ucLogStatus);
            iLen +=
                encode_context_bitstring(&apdu[iLen], pSource->ucRecType,
                &TempBits);
            break;

        case TL_TYPE_BOOL:
            iLen +=
                encode_context_boolean(&apdu[iLen], pSource->ucRecType,
                pSource->Datum.ucBoolean);
            break;

        case TL_TYPE_REAL:
            iLen +=
                encode_context_real(&apdu[iLen], pSource->ucRecType,
                pSource->Datum.fReal);
            break;

        case TL_TYPE_ENUM:
            iLen +=
                encode_context_enumerated(&apdu[iLen], pSource->ucRecType,
                pSource->Datum.ulEnum);
            break;

        case TL_TYPE_UNSIGN:
            iLen +=
                encode_context_unsigned(&apdu[iLen], pSource->ucRecType,
                pSource->Datum.ulUValue);
            break;

        case TL_TYPE_SIGN:
            iLen +=
                encode_context_signed(&apdu[iLen], pSource->ucRecType,
                pSource->Datum.lSValue);
            break;

        case TL_TYPE_BITS:
            /* Rebuild bitstring directly from stored octets - which we
             * have limited to 32 bits maximum as allowed by the standard
             */
            bitstring_init(&TempBits);
            bitstring_set_bits_used(&TempBits,
                (pSource->Datum.Bits.ucLen >> 4) & 0x0F,
                pSource->Datum.Bits.ucLen & 0x0F);
            for (ucCount = pSource->Datum.Bits.ucLen >> 4; ucCount > 0;
                ucCount--)
                bitstring_set_octet(&TempBits, ucCount - 1,
                    pSource->Datum.Bits.ucStore[ucCount - 1]);

            iLen +=
                encode_context_bitstring(&apdu[iLen], pSource->ucRecType,
                &TempBits);
            break;

        case TL_TYPE_NULL:
            iLen += encode_context_null(&apdu[iLen], pSource->ucRecType);
            break;

        case TL_TYPE_ERROR:
            iLen += encode_opening_tag(&apdu[iLen], TL_TYPE_ERROR);
            iLen +=
                encode_application_enumerated(&apdu[iLen],
                pSource->Datum.Error.usClass);
            iLen +=
                encode_application_enumerated(&apdu[iLen],
                pSource->Datum.Error.usCode);
            iLen += encode_closing_tag(&apdu[iLen], TL_TYPE_ERROR);
            break;

        case TL_TYPE_DELTA:
            iLen +=
                encode_context_real(&apdu[iLen], pSource->ucRecType,
                pSource->Datum.fTime);
            break;

        case TL_TYPE_ANY:
            /* Should never happen as we don't support this at the moment */
            break;
    }

    iLen += encode_closing_tag(&apdu[iLen], 1);
    /* Check if status bit string is required and insert with tag [2] */
    if ((pSource->ucStatus & 128) == 128) {
        bitstring_init(&TempBits);
        bitstring_set_bits_used(&TempBits, 1, 4);
        /* only insert the 1st 4 bits */
        bitstring_set_octet(&TempBits, 0, (pSource->ucStatus & 0x0F));
        iLen += encode_context_bitstring(&apdu[iLen], 2, &TempBits);
    }

    return (iLen);
}

static int local_read_property(
    uint8_t * value,
    uint8_t * status,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * Source,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    if (value != NULL) {
        /* configure our storage */
        rpdata.application_data = value;
        rpdata.application_data_len = MAX_APDU;
        rpdata.object_type = Source->objectIdentifier.type;
        rpdata.object_instance = Source->objectIdentifier.instance;
        rpdata.object_property = Source->propertyIdentifier;
        rpdata.array_index = Source->arrayIndex;
        /* Try to fetch the required property */
        len = Device_Read_Property(&rpdata);
        if (len < 0) {
            *error_class = rpdata.error_class;
            *error_code = rpdata.error_code;
        }
    }

    if ((len >= 0) && (status != NULL)) {
        /* Fetch the status flags if required */
        rpdata.application_data = status;
        rpdata.application_data_len = MAX_APDU;
        rpdata.object_property = PROP_STATUS_FLAGS;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Device_Read_Property(&rpdata);
        if (len < 0) {
            *error_class = rpdata.error_class;
            *error_code = rpdata.error_code;
        }
    }

    return (len);
}

/****************************************************************************
 * Attempt to fetch the logged property and store it in the Trend Log       *
 ****************************************************************************/

void TL_fetch_property(
    int iLog)
{
    uint8_t ValueBuf[MAX_APDU]; /* This is a big buffer in case someone selects the device object list for example */
    uint8_t StatusBuf[3];       /* Should be tag, bits unused in last octet and 1 byte of data */
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
    int iLen;
    uint8_t ucCount;
    TL_LOG_INFO *CurrentLog;
    TL_DATA_REC TempRec;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    BACNET_BIT_STRING TempBits;

    CurrentLog = &LogInfo[iLog];

    /* Record the current time in the log entry and also in the info block
     * for the log so we can figure out when the next reading is due */
    TempRec.tTimeStamp = get_current_time();
    CurrentLog->tLastDataTime = TempRec.tTimeStamp;
    TempRec.ucStatus = 0;

    iLen =
        local_read_property(ValueBuf, StatusBuf, &LogInfo[iLog].Source,
        &error_class, &error_code);
    if (iLen < 0) { 
        /* Insert error code into log */
        TempRec.Datum.Error.usClass = error_class;
        TempRec.Datum.Error.usCode = error_code;
        TempRec.ucRecType = TL_TYPE_ERROR;
    } else {
			
        /* Decode data returned and see if we can fit it into the log */
        iLen =
            decode_tag_number_and_value(ValueBuf, &tag_number,
            &len_value_type);
        switch (tag_number) {
            case BACNET_APPLICATION_TAG_NULL:
                TempRec.ucRecType = TL_TYPE_NULL;
                break;

            case BACNET_APPLICATION_TAG_BOOLEAN:
                TempRec.ucRecType = TL_TYPE_BOOL;
                TempRec.Datum.ucBoolean = decode_boolean(len_value_type);
                break;

            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                TempRec.ucRecType = TL_TYPE_UNSIGN;
                decode_unsigned(&ValueBuf[iLen], len_value_type,
                    &TempRec.Datum.ulUValue);
                break;

            case BACNET_APPLICATION_TAG_SIGNED_INT:
                TempRec.ucRecType = TL_TYPE_SIGN;
                decode_signed(&ValueBuf[iLen], len_value_type,
                    &TempRec.Datum.lSValue);
                break;

            case BACNET_APPLICATION_TAG_REAL:
                TempRec.ucRecType = TL_TYPE_REAL;
                decode_real_safe(&ValueBuf[iLen], len_value_type,
                    &TempRec.Datum.fReal);
                break;

            case BACNET_APPLICATION_TAG_BIT_STRING:
                TempRec.ucRecType = TL_TYPE_BITS;
                decode_bitstring(&ValueBuf[iLen], len_value_type, &TempBits);
                /* We truncate any bitstrings at 32 bits to conserve space */
                if (bitstring_bits_used(&TempBits) < 32) {
                    /* Store the bytes used and the bits free in the last byte */
                    TempRec.Datum.Bits.ucLen =
                        bitstring_bytes_used(&TempBits) << 4;
                    TempRec.Datum.Bits.ucLen |=
                        (8 - (bitstring_bits_used(&TempBits) % 8)) & 7;
                    /* Fetch the octets with the bits directly */
                    for (ucCount = 0;
                        ucCount < bitstring_bytes_used(&TempBits); ucCount++)
                        TempRec.Datum.Bits.ucStore[ucCount] =
                            bitstring_octet(&TempBits, ucCount);
                } else {
                    /* We will only use the first 4 octets to save space */
                    TempRec.Datum.Bits.ucLen = 4 << 4;
                    for (ucCount = 0; ucCount < 4; ucCount++)
                        TempRec.Datum.Bits.ucStore[ucCount] =
                            bitstring_octet(&TempBits, ucCount);
                }
                break;

            case BACNET_APPLICATION_TAG_ENUMERATED:
                TempRec.ucRecType = TL_TYPE_ENUM;
                decode_enumerated(&ValueBuf[iLen], len_value_type,
                    &TempRec.Datum.ulEnum);
                break;

            default:
                /* Fake an error response for any types we cannot handle */
                TempRec.Datum.Error.usClass = ERROR_CLASS_PROPERTY;
                TempRec.Datum.Error.usCode = ERROR_CODE_DATATYPE_NOT_SUPPORTED;
                TempRec.ucRecType = TL_TYPE_ERROR;
                break;
        }
        /* Finally insert the status flags into the record */
        iLen =
            decode_tag_number_and_value(StatusBuf, &tag_number,
            &len_value_type);
        decode_bitstring(&StatusBuf[iLen], len_value_type, &TempBits);
        TempRec.ucStatus = 128 | bitstring_octet(&TempBits, 0);
    }


    Logs[iLog][CurrentLog->iIndex++] = TempRec;
    if (CurrentLog->iIndex >= TL_MAX_ENTRIES)
        CurrentLog->iIndex = 0;

    CurrentLog->ulTotalRecordCount++;

    if (CurrentLog->ulRecordCount < TL_MAX_ENTRIES)
        CurrentLog->ulRecordCount++;
}

/****************************************************************************
 * Check each log to see if any data needs to be recorded.                  *
 ****************************************************************************/

void trend_log_timer(
    uint16_t uSeconds)
{
    TL_LOG_INFO *CurrentLog = NULL;
    int iCount = 0;
    time_t tNow = 0;

    /* unused parameter */
    uSeconds = uSeconds;
    /* use OS to get the current time */
    tNow = get_current_time();
    for (iCount = 0; iCount < MAX_TREND_LOGS; iCount++) {
        CurrentLog = &LogInfo[iCount];
        if (TL_Is_Enabled(iCount)) { 
            if (CurrentLog->LoggingType == LOGGING_TYPE_POLLED) {
                /* For polled logs we first need to see if they are clock
                 * aligned or not.
                 */
                if (CurrentLog->bAlignIntervals == true) { 
                    /* Aligned logging so use the combination of the interval
                     * and the offset to decide when to log. Also log a reading if
                     * more than interval time has elapsed since last reading to ensure
                     * we don't miss a reading if we aren't called at the precise second
                     * when the match occurrs.
                     */
/*                if(((tNow % CurrentLog->ulLogInterval) >= (CurrentLog->ulIntervalOffset % CurrentLog->ulLogInterval)) && */
/*                   ((tNow - CurrentLog->tLastDataTime) >= CurrentLog->ulLogInterval)) { */
                    if ((tNow % CurrentLog->ulLogInterval) ==
                        (CurrentLog->ulIntervalOffset %
                            CurrentLog->ulLogInterval)) {
                        /* Record value if time synchronised trigger condition is met
                         * and at least one period has elapsed.
                         */
                        TL_fetch_property(iCount);
                    } else if ((tNow - CurrentLog->tLastDataTime) >
                        CurrentLog->ulLogInterval) {
                        /* Also record value if we have waited more than a period
                         * since the last reading. This ensures we take a reading as
                         * soon as possible after a power down if we have been off for
                         * more than a single period.
                         */ 
                        TL_fetch_property(iCount);
                    }
                } else if (((tNow - CurrentLog->tLastDataTime) >=
                        CurrentLog->ulLogInterval) ||
                    (CurrentLog->bTrigger == true)) {
                    /* If not aligned take a reading when we have either waited long
                     * enough or a trigger is set.
                     */
                    TL_fetch_property(iCount);
                }

                CurrentLog->bTrigger = false;   /* Clear this every time */
            } else if (CurrentLog->LoggingType == LOGGING_TYPE_TRIGGERED) {
                /* Triggered logs take a reading when the trigger is set and
                 * then reset the trigger to wait for the next event
                 */
                if (CurrentLog->bTrigger == true) { 
                    TL_fetch_property(iCount); 
                    CurrentLog->bTrigger = false;
                }
            }
        }
    }
}



#endif

