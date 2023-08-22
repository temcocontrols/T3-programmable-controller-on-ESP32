/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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

/* Analog Value Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include "bacnet.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "wp.h"
#include "av.h"


#if 1//BAC_COMMON

#define AV_LEVEL_NULL 255
/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define AV_RELINQUISH_DEFAULT 0
//extern  char text_string[20];
//#if (MAX_AVS > 10)
//#error Modify the Analog_Value_Name to handle multiple digits
//#endif

//float far AV_Present_Value[MAX_AVS][BACNET_MAX_PRIORITY];
uint8_t AVS;

static
#if ARM
 const 
#endif	
int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
//    PROP_STATUS_FLAGS,
//    PROP_EVENT_STATE,
//    PROP_OUT_OF_SERVICE,
    PROP_UNITS,
//    PROP_PRIORITY_ARRAY,
//    PROP_RELINQUISH_DEFAULT,
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





void Analog_Value_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
#if ASIX
	Properties_Required[0] = PROP_OBJECT_IDENTIFIER;
	Properties_Required[1] = PROP_OBJECT_NAME;
	Properties_Required[2] = PROP_OBJECT_TYPE;
	Properties_Required[3] = PROP_PRESENT_VALUE;
	Properties_Required[4] = PROP_UNITS;
	Properties_Required[5] = -1;
	
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

bool Analog_Value_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name)
{
    bool status = false;
		unsigned index = 0;

    index = Analog_Value_Instance_To_Index(object_instance);
	
    if (object_instance < MAX_AVS) {
        status = characterstring_init_ansi(object_name, get_label(AV,index));
    }

    return status;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Value_Valid_Instance(
    uint32_t object_instance)
{
    if ((object_instance < MAX_AVS + OBJECT_BASE) /*&& (object_instance >= OBJECT_BASE)*/)
        return true;
		
    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Analog_Value_Count(
    void)
{
	 return AVS;
	
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Analog_Value_Index_To_Instance(
    unsigned index)
{

	return AV_Index_To_Instance[index] + OBJECT_BASE;

}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Analog_Value_Instance_To_Index(
    uint32_t object_instance)
{   
	return AV_Instance_To_Index[object_instance - OBJECT_BASE];

}

float Analog_Value_Present_Value(
    uint32_t object_instance)
{
    float value = AV_RELINQUISH_DEFAULT;
    unsigned index = 0;
    unsigned i = 0;

    index = object_instance;//Analog_Value_Instance_To_Index(object_instance);
    if (index < MAX_AVS) {
//	value = AO_Present_Value[index];
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (/*AO_Present_Value[index][i]*/Get_bacnet_value_from_buf(AV,i,index) != AV_LEVEL_NULL) {
                value = Get_bacnet_value_from_buf(AV,i,index);//AO_Present_Value[index][i];
                break;
            }
        }
    }

    return value;
}

unsigned Analog_Value_Present_Value_Priority(
    uint32_t object_instance)
{
    unsigned index = 0; /* instance to index conversion */
    unsigned i = 0;     /* loop counter */
    unsigned priority = 0;      /* return value */

    index = object_instance;//Analog_Value_Instance_To_Index(object_instance);
    if (index < MAX_AVS) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
            if (/*AV_Present_Value[index][i]*/Get_bacnet_value_from_buf(AV,i,index) != AV_LEVEL_NULL) {
                priority = i + 1;
                break;
            }
        }
    }

    return priority;
}

bool Analog_Value_Present_Value_Set(
		uint32_t object_instance,
		float value,
		uint8_t priority)
{
    unsigned index = 0;
    bool status = false;
		 index = object_instance;//	index = Analog_Value_Instance_To_Index(object_instance);
	
//	if (index < MAX_AOS)
//		if(value >= 0.0 && value <= 100.0)   // 0.0 - 100.0
//		{
//			AO_Present_Value[index] = value;
//			wirte_bacnet_value_to_buf(AO,index);
//		}
    
    if (index < MAX_AVS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ )/* &&
            (value >= 0) && (value <= 1000)*/) {
            //AO_Present_Value[index][priority - 1] = (uint8_t) value;
						wirte_bacnet_value_to_buf(AV,priority,index,value);
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

bool Analog_Value_Present_Value_Relinquish(
    uint32_t object_instance,
    unsigned priority)
{
    unsigned index = 0;
    bool status = false;

		index = object_instance;
//    index = Analog_Value_Instance_To_Index(object_instance);
    if (index < MAX_AVS) {
        if (priority && (priority <= BACNET_MAX_PRIORITY) &&
            (priority != 6 /* reserved */ )) {
							wirte_bacnet_value_to_buf(AV,priority,index,AV_LEVEL_NULL);
           // AV_Present_Value[index][priority - 1] = AV_LEVEL_NULL;
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

/* return apdu len, or -1 on error */
// READ
int Analog_Value_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING far bit_string;
    BACNET_CHARACTER_STRING far char_string;
		unsigned far object_index;
		unsigned i = 0;
		int len = 0;
	  float real_value = (float) 1.414;
		bool state = false;
	
		object_index = Analog_Value_Instance_To_Index(object_instance);
	
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_ANALOG_VALUE, object_index);
            break;
        case PROP_OBJECT_NAME:
						characterstring_init_ansi(&char_string, get_label(AV,object_index));
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
						characterstring_init_ansi(&char_string,get_description(AV,object_index));
						apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_ANALOG_VALUE);
            break;
        case PROP_PRESENT_VALUE:         
            apdu_len = encode_application_real(&apdu[0], Analog_Value_Present_Value(object_index)
				/*AV_Present_Value[object_index]*/);
						
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
            apdu_len = encode_application_boolean(&apdu[0],  Get_Out_Of_Service(AV,object_index));
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0], get_range(AV,object_index));
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
                    if (Get_bacnet_value_from_buf(AV,i,object_index)/*AV_Present_Value[object_index][i]*/ == AV_LEVEL_NULL)
                        len = encode_application_null(&apdu[apdu_len]);
                    else {
                        real_value = Get_bacnet_value_from_buf(AV,i,object_index)/*AV_Present_Value[object_index][i]*/;
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
                    if (Get_bacnet_value_from_buf(AV,i,object_index)/*AV_Present_Value[object_index][ array_index - 1]*/
											== AV_LEVEL_NULL)
                        apdu_len = encode_application_null(&apdu[0]);
                    else {
                        real_value = Get_bacnet_value_from_buf(AV,i,object_index)/*AV_Present_Value[object_index][ array_index - 1]*/;
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
            real_value = AV_RELINQUISH_DEFAULT;
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (array_index != BACNET_ARRAY_ALL)) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = -1;    
    }

    return apdu_len;
}

/* returns true if successful */
bool Analog_Value_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int far object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE far value;

	object_index = Analog_Value_Instance_To_Index(wp_data->object_instance);
    if (!Analog_Value_Valid_Instance(object_index)) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */

    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);

    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
				
		
		
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                
                //AV_Present_Value[object_index] = value.type.Real; 
						status =
                    Analog_Value_Present_Value_Set(object_index,
                    value.type.Real, wp_data->priority);
//				wirte_bacnet_value_to_buf(AV,wp_data->priority,object_index,value.type.Real);

                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
				case PROP_OBJECT_NAME:
					if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
					write_bacnet_name_to_buf(AV,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }		
				break;
				case PROP_DESCRIPTION:
					if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
							write_bacnet_description_to_buf(AV,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
				break;	
				case PROP_UNITS:
				if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
               
					write_bacnet_unit_to_buf(AV,wp_data->priority,object_index,value.type.Enumerated);

                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }					
				break;
				case PROP_OUT_OF_SERVICE:
		
				if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
              				
					write_Out_Of_Service(AV,object_index,value.type.Boolean);

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
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}



/* returns true if value has changed */
bool Analog_Value_Encode_Value_List(
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE * value_list)
{
    bool status = false;
		unsigned object_index;
		object_index = Analog_Value_Instance_To_Index(object_instance);
    if (value_list) {
        value_list->propertyIdentifier = PROP_PRESENT_VALUE;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_REAL;
        value_list->value.type.Real =
						Get_bacnet_value_from_buf(AV,0,object_index);
            //Analog_Input_Present_Value(object_instance);
        value_list->priority = BACNET_NO_PRIORITY;
        value_list = value_list->next;
    }
    if (value_list) {
        value_list->propertyIdentifier = PROP_STATUS_FLAGS;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
        bitstring_init(&value_list->value.type.Bit_String);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_IN_ALARM, false);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_FAULT, false);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OVERRIDDEN, false);
//        if (Binary_Input_Out_Of_Service(object_instance)) {
//            bitstring_set_bit(&value_list->value.type.Bit_String,
//                STATUS_FLAG_OUT_OF_SERVICE, true);
//        } else {
//            bitstring_set_bit(&value_list->value.type.Bit_String,
//                STATUS_FLAG_OUT_OF_SERVICE, false);
//        }
        value_list->priority = BACNET_NO_PRIORITY;
    }
    status = Analog_Value_Change_Of_Value(object_instance);
    return status;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testAnalog_Value(
    Test * pTest)
{
    uint8_t far apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_ANALOG_VALUE;
    uint32_t decoded_instance = 0;
    uint32_t instance = 123;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;

    len =
        Analog_Value_Encode_Property_APDU(&apdu[0], instance,
        PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL, &error_class, &error_code);
    ct_test(pTest, len != 0);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    ct_test(pTest, tag_number == BACNET_APPLICATION_TAG_OBJECT_ID);
    len =
        decode_object_id(&apdu[len], (int *) &decoded_type, &decoded_instance);
    ct_test(pTest, decoded_type == OBJECT_ANALOG_VALUE);
    ct_test(pTest, decoded_instance == instance);

    return;
}

#ifdef TEST_ANALOG_VALUE
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Analog Value", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testAnalog_Value);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_ANALOG_VALUE */
#endif /* TEST */

#endif
