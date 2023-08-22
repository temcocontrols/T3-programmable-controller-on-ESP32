/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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

/* Analog Output Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include "bacnet.h"
#include <stdio.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "wp.h"
#include "ao.h"
#include "handlers.h"




#if 1//BAC_COMMON

/* we choose to have a NULL level in our system represented by */
/* a particular value.  When the priorities are not in use, they */
/* will be relinquished (i.e. set to the NULL level). */
#define AO_LEVEL_NULL 255
/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define AO_RELINQUISH_DEFAULT 0
/* Here is our Priority Array.  They are supposed to be Real, but */
/* we don't have that kind of memory, so we will use a single byte */
/* and load a Real for returning the value when asked. */
//float far AO_Present_Value[MAX_AOS][BACNET_MAX_PRIORITY];
uint8_t  AOS;

/* Writable out-of-service allows others to play with our Present Value */
/* without changing the physical output */
static bool Analog_Output_Out_Of_Service[MAX_AOS];

/* we need to have our arrays initialized before answering any calls */
static bool Analog_Output_Initialized = false;

/* These three arrays are used by the ReadPropertyMultiple handler */
static
#if ARM
 const 
#endif
int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_UNITS,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    -1
};
static
#if ARM
 const 
#endif
int Properties_Optional[] = {
    -1
};

static
#if ARM
 const 
#endif
int Properties_Proprietary[] = {
    -1
};

void Analog_Output_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
#if ASIX

	Properties_Required[0] = PROP_OBJECT_IDENTIFIER;
	Properties_Required[1] = PROP_OBJECT_NAME;
	Properties_Required[2] = PROP_OBJECT_TYPE;
	Properties_Required[3] = PROP_PRESENT_VALUE;
	Properties_Required[4] = PROP_STATUS_FLAGS;
	Properties_Required[5] = PROP_EVENT_STATE;
	Properties_Required[6] = PROP_OUT_OF_SERVICE;
	Properties_Required[7] = PROP_UNITS;
	Properties_Required[8] = PROP_PRIORITY_ARRAY;
	Properties_Required[9] = PROP_RELINQUISH_DEFAULT;
	Properties_Required[10] = -1;
	
	Properties_Optional[0] = -1;
	Properties_Proprietary[0] = -1;
#endif
    if (pRequired)
        *pRequired = Properties_Required;
    if (pOptional)
        *pOptional = Properties_Optional;
    if (pProprietary)
        *pProprietary = Properties_Proprietary;

    return;
}

//void Analog_Output_Init(void)
//{
//   unsigned i,j;

////    if (!Analog_Output_Initialized) {
////        Analog_Output_Initialized = true;

//        /* initialize all the analog output priority arrays to NULL */
//        for (i = 0; i < MAX_AOS; i++) {
//            for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
//                AO_Present_Value[i][j] = AO_LEVEL_NULL;
//            }
//        }
//		
////    }

//    return;
//}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Output_Valid_Instance(
    uint32_t object_instance)
{
    if ((object_instance < MAX_AOS + OBJECT_BASE)/* && (object_instance >= OBJECT_BASE)*/)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Analog_Output_Count(
    void)
{
	return AOS;

}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Analog_Output_Index_To_Instance(
    unsigned index)
{
#ifdef T3_CON
		return AO_Index_To_Instance[index] + OBJECT_BASE;
#else
    return index + OBJECT_BASE;
#endif
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Analog_Output_Instance_To_Index(
    uint32_t object_instance)
{
#ifdef T3_CON
		return AO_Instance_To_Index[object_instance - OBJECT_BASE];
#else
    return object_instance - OBJECT_BASE;
#endif
}

#if EXTERNAL_IO
uint8_t Get_AOx_by_index(uint8_t index,uint8_t *bo_index);

float Analog_Output_Present_Value(
    uint32_t object_instance)
{
    float value = AO_RELINQUISH_DEFAULT;
    uint8_t index = 0;
    unsigned i = 0;
    //index = object_instance;//Analog_Output_Instance_To_Index(object_instance);
	
  Get_AOx_by_index(object_instance,&index);  
	value = Get_Output_Relinguish(AO,index);
	if (index < MAX_AOS) {
//	value = AO_Present_Value[index];
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (/*AO_Present_Value[index][i]*/Get_bacnet_value_from_buf(AO,i,index) != AO_LEVEL_NULL) {

                value = Get_bacnet_value_from_buf(AO,i,index);//AO_Present_Value[index][i];
                break;
            }
        }
    }

    return value;
}
#endif



float Analog_Output_Present_Value1(
    uint32_t object_instance)
{
    float value = AO_RELINQUISH_DEFAULT;
    unsigned index = 0;
    unsigned i = 0;
    index = object_instance;//Analog_Output_Instance_To_Index(object_instance);
	
		value = Get_Output_Relinguish(AO,index);
	
    if (index < MAX_AOS) {
//	value = AO_Present_Value[index];
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (/*AO_Present_Value[index][i]*/Get_bacnet_value_from_buf(AO,i,index) != AO_LEVEL_NULL) {

                value = Get_bacnet_value_from_buf(AO,i,index);//AO_Present_Value[index][i];
                break;
            }
        }
    }

    return value;
}

unsigned Analog_Output_Present_Value_Priority(
    uint32_t object_instance)
{
    unsigned index = 0; /* instance to index conversion */
    unsigned i = 0;     /* loop counter */
    unsigned priority = 0;      /* return value */

    index = object_instance;//Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_AOS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (/*AO_Present_Value[index][i]*/Get_bacnet_value_from_buf(AO,i,index) != AO_LEVEL_NULL) {
                priority = i + 1;
                break;
            }
        }
    }

    return priority;
}

bool Analog_Output_Present_Value_Set(
    uint32_t object_instance,
    float value,
    unsigned priority)
{
    unsigned index = 0;
    bool status = false;
		 index = object_instance;//	index = Analog_Output_Instance_To_Index(object_instance);
	
//	if (index < MAX_AOS)
//		if(value >= 0.0 && value <= 100.0)   // 0.0 - 100.0
//		{
//			AO_Present_Value[index] = value;
//			wirte_bacnet_value_to_buf(AO,index);
//		}
    
    if (index < MAX_AOS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ ) /*&&
            (value >= 0) && (value <= 1000)*/) {
            //AO_Present_Value[index][priority - 1] = (uint8_t) value;
						wirte_bacnet_value_to_buf(AO,priority - 1,index,value);
            /* Note: you could set the physical output here to the next
               highest priority, or to the relinquish default if no
               priorities are set.
               However, if Out of Service is TRUE, then don't set the
               physical output.  This comment may apply to the
               main loop (i.e. check out of service before changing output) */
            status = true;
        }
    }

    return status;
}

bool Analog_Output_Present_Value_Relinquish(
    uint32_t object_instance,
    unsigned priority)
{
    unsigned index = 0;
    bool status = false;

		index = object_instance;
 //   index = Analog_Output_Instance_To_Index(object_instance);
    if (index < MAX_AOS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ )) {
           // AO_Present_Value[index][priority - 1] = AO_LEVEL_NULL;
							wirte_bacnet_value_to_buf(AO,priority - 1,index,AO_LEVEL_NULL);
            /* Note: you could set the physical output here to the next
               highest priority, or to the relinquish default if no
               priorities are set.
               However, if Out of Service is TRUE, then don't set the
               physical output.  This comment may apply to the
               main loop (i.e. check out of service before changing output) */
            status = true;
        }
    }

    return status;
}

/* note: the object name must be unique within this device */
bool Analog_Output_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name)
{
    bool status = false;
		unsigned index = 0;

    index = Analog_Output_Instance_To_Index(object_instance);
	
    if (object_instance < MAX_AOS) {
        status = characterstring_init_ansi(object_name, get_label(AO,index));
    }

    return status;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
// read
int Analog_Output_Encode_Property_APDU
	(uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)

{
    int len = 0;
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING far bit_string;
    BACNET_CHARACTER_STRING far char_string;
    float real_value = (float) 1.414;
    unsigned object_index = 0;
    unsigned i = 0;
    bool state = false;
//    uint8_t *apdu = NULL;
//
//    if ((rpdata == NULL) || ( application_data == NULL) ||
//        ( application_data_len == 0)) {
//        return 0;
//    }
//    apdu =  application_data;
	
	  object_index = Analog_Output_Instance_To_Index( object_instance);
	
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_ANALOG_OUTPUT,
                object_index);
            break;
			 case PROP_OBJECT_NAME:
						characterstring_init_ansi(&char_string, get_label(AO,object_index));
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
						characterstring_init_ansi(&char_string,get_description(AO,object_index));
						apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ANALOG_OUTPUT);
            break;
        case PROP_PRESENT_VALUE:
            real_value = Analog_Output_Present_Value1(object_index);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:

            state = Get_Out_Of_Service(AO,object_index);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_UNITS:
						apdu_len = encode_application_enumerated(&apdu[0], get_range(AO,object_index));

            break;
        case PROP_PRIORITY_ARRAY:
            /* Array element zero is the number of elements in the array */
            if (array_index == 0)
                apdu_len = encode_application_unsigned(&apdu[0], BACNET_MAX_PRIORITY);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet. */
            else if (array_index == BACNET_ARRAY_ALL) {
			
                for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
                    /* FIXME: check if we have room before adding it to APDU */
                    if (Get_bacnet_value_from_buf(AO,i,object_index)/*AO_Present_Value[object_index][i]*/ == AO_LEVEL_NULL)
                        len = encode_application_null(&apdu[apdu_len]);
                    else {
                        real_value = Get_bacnet_value_from_buf(AO,i,object_index)/*AO_Present_Value[object_index][i]*/;
                        len =
                            encode_application_real(&apdu[apdu_len],
                            real_value);
                    }
                    /* add it if we have room */
                    if ((apdu_len + len) < MAX_APDU)
                        apdu_len += len;
                    else {
                         *error_class = ERROR_CLASS_SERVICES;
                         *error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
                        apdu_len = BACNET_STATUS_ERROR;
                        break;
                    }
                }
            } else {

                 if ( array_index <= BACNET_MAX_PRIORITY) {
                    if (Get_bacnet_value_from_buf(AO,i,object_index)/*AO_Present_Value[object_index][ array_index - 1]*/
											== AO_LEVEL_NULL)
                        apdu_len = encode_application_null(&apdu[0]);
                    else {
                        real_value = Get_bacnet_value_from_buf(AO,i,object_index)/*AO_Present_Value[object_index][ array_index - 1]*/;
                        apdu_len =
                            encode_application_real(&apdu[0], real_value);
                    }
                } else {
                     *error_class = ERROR_CLASS_PROPERTY;
                     *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;

        case PROP_RELINQUISH_DEFAULT:
            real_value = Get_Output_Relinguish(AO,object_index);//AO_RELINQUISH_DEFAULT;
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        default:
             *error_class = ERROR_CLASS_PROPERTY;
             *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && ( property != PROP_PRIORITY_ARRAY) &&
        ( array_index != BACNET_ARRAY_ALL)) {
         *error_class = ERROR_CLASS_PROPERTY;
         *error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Analog_Output_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    unsigned int far object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE far value;

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
    /*  only array properties can have array options */
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
				
		object_index = Analog_Output_Instance_To_Index(wp_data->object_instance);
		
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                status =
                    Analog_Output_Present_Value_Set(object_index,
                    value.type.Real, wp_data->priority);
                if (wp_data->priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                status =
                    WPValidateArgType(&value, BACNET_APPLICATION_TAG_NULL,
                    &wp_data->error_class, &wp_data->error_code);
                if (status) {
                    status =
                        Analog_Output_Present_Value_Relinquish
                        (object_index, wp_data->priority);
                    if (!status) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }	 
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BOOLEAN,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
//                Analog_Output_Out_Of_Service[object_index] =
//                    value.type.Boolean;
							//   save  auto_manual
							//write_bacent_AM_to_buf
							write_Out_Of_Service(AO,object_index,value.type.Boolean);
            }
            break;
				case PROP_OBJECT_NAME:
				if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                
					write_bacnet_name_to_buf(AO,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }								
				break;
				case PROP_UNITS:
				if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                
					write_bacnet_unit_to_buf(AO,wp_data->priority,object_index,value.type.Enumerated);

                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }					
				break;
				case PROP_DESCRIPTION:
					if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
							write_bacnet_description_to_buf(AO,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
	
				break; 
				case PROP_RELINQUISH_DEFAULT:
					if (value.tag == BACNET_APPLICATION_TAG_REAL) {						
					write_Output_Relinguish(AO,object_index,value.type.Real);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						 
					break;
					break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        case PROP_PRIORITY_ARRAY:
       
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}


#endif
