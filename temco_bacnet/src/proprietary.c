
/* proprietary Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include "bacnet.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "wp.h"
#include "proprietary.h"


#if BAC_COMMON

uint8_t TemcoVars;

static
#if ARM
 const 
#endif	
int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    -1
};




void TemcoVars_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
#if ASIX
	Properties_Required[0] = PROP_OBJECT_IDENTIFIER;
	Properties_Required[1] = PROP_OBJECT_NAME;
	Properties_Required[2] = PROP_OBJECT_TYPE;
	Properties_Required[3] = PROP_PRESENT_VALUE;
	Properties_Required[4] = -1;
	
#endif
    if (pRequired)
        *pRequired = Properties_Required;

    return;
}


/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool TemcoVars_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_TEMCOVARS + OBJECT_BASE)
        return true;

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned TemcoVars_Count(
    void)
{
	 return TemcoVars;
	
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t TemcoVars_Index_To_Instance(
    unsigned index)
{
    return index + OBJECT_BASE;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned TemcoVars_Instance_To_Index(
    uint32_t object_instance)
{   

    return object_instance - OBJECT_BASE;
}

float TemcoVars_Present_Value(
    uint32_t object_instance)
{
    float value = 0.0;
    unsigned index = 0;
    unsigned i = 0;

    index = object_instance;//Analog_Value_Instance_To_Index(object_instance);
    if (index < MAX_TEMCOVARS) {
	value = Get_bacnet_value_from_buf(TEMCOAV,0,index);//AO_Present_Value[index][i];
    }

    return value;
}


bool TemcoVars_Present_Value_Set(
		uint32_t object_instance,
		float value,
		uint8_t priority)
{
    unsigned index = 0;
    bool status = false;
		index = object_instance;//	index = Analog_Value_Instance_To_Index(object_instance);
	
		if(index < MAX_TEMCOVARS)
		{
			wirte_bacnet_value_to_buf(TEMCOAV,0,index,value);
			return  true;
		}

		return false;
}


/* return apdu len, or -1 on error */
// READ
int TemcoVars_Encode_Property_APDU(
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
	
		object_index = TemcoVars_Instance_To_Index(object_instance);
	  
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], PROPRIETARY_BACNET_OBJECT_TYPE, object_index);
            break;
        case PROP_OBJECT_NAME:
						characterstring_init_ansi(&char_string, get_label(TEMCOAV,object_index));
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], PROPRIETARY_BACNET_OBJECT_TYPE);
            break;
        case PROP_PRESENT_VALUE:         
					//if(object_index < 4)
							apdu_len = encode_application_real(&apdu[0], TemcoVars_Present_Value(object_index));						
						/*else if(object_index == 4)
						{
							characterstring_init_ansi(&char_string, Get_temcovars_string_from_buf(object_index));
							apdu_len = encode_application_character_string(&apdu[0], &char_string);	
						}*/
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
bool TemcoVars_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    unsigned int far object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE far value;

		object_index = TemcoVars_Instance_To_Index(wp_data->object_instance);
	
    if (!TemcoVars_Valid_Instance(object_index)) {
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
						//if(object_index < 4)
						{						
							if (value.tag == BACNET_APPLICATION_TAG_REAL) {
									
							status =
											TemcoVars_Present_Value_Set(object_index,
											value.type.Real, wp_data->priority);

									status = true;
							}
							else {
									*error_class = ERROR_CLASS_PROPERTY;
									*error_code = ERROR_CODE_INVALID_DATA_TYPE;
							}
						}
						/*if(object_index == 4)
						{						
							if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
									
							status =
											Write_temcovars_string_to_buf(object_index,
											value.type.Character_String.value);

									status = true;
							}
							else {
									*error_class = ERROR_CLASS_PROPERTY;
									*error_code = ERROR_CODE_INVALID_DATA_TYPE;
							}
						}*/
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
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }
    return status;
}


#endif
