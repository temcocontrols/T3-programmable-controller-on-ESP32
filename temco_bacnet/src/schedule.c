
/* Schedule Objects customize -- writen by chesea*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacnet.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"

#include "schedule.h"

#if BAC_SCHEDULE
/*
reliable have following property  

-present_value
-description
-list_of_object_property_references
-weeky_schedule
-out_if_service

// to be added more

-status_flag
-effectieve_period
-exception_schedule
-schedule_default
-local_date
-local_time
-reliabiltity
-prioritoy_for_writing
*/

uint8_t  SCHEDULES;
BACNET_DATE Start_Date;
BACNET_DATE End_Date;

static
#if ARM
 const 
#endif	
int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_EFFECTIVE_PERIOD,
    PROP_SCHEDULE_DEFAULT,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
    PROP_PRIORITY_FOR_WRITING,
    PROP_STATUS_FLAGS,
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,
		PROP_EXCEPTION_SCHEDULE,
    -1
};
static
#if ARM
 const 
#endif	
int Properties_Optional[] = {
    PROP_WEEKLY_SCHEDULE,
    -1
};
static
#if ARM
 const 
#endif	
int Properties_Proprietary[] = {
    -1
};


void Schedule_Property_Lists(const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
#if ASIX
	Properties_Required[0] = PROP_OBJECT_IDENTIFIER;
	Properties_Required[1] = PROP_OBJECT_NAME;
	Properties_Required[2] = PROP_OBJECT_TYPE;
	Properties_Required[3] = PROP_PRESENT_VALUE;
	Properties_Required[4] = PROP_EFFECTIVE_PERIOD;
	Properties_Required[5] = PROP_SCHEDULE_DEFAULT;
	Properties_Required[6] = PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES;
	Properties_Required[7] = PROP_PRIORITY_FOR_WRITING;
	Properties_Required[8] = PROP_STATUS_FLAGS;
	Properties_Required[9] = PROP_RELIABILITY;
	Properties_Required[10] = PROP_OUT_OF_SERVICE;
	Properties_Required[11] = PROP_EXCEPTION_SCHEDULE;
	Properties_Optional[0] = -1;
	Properties_Proprietary[0] = -1;
#endif
    if (pRequired)
        *pRequired = Properties_Required;
    if (pOptional)
        *pOptional = Properties_Optional;
    if (pProprietary)
        *pProprietary = Properties_Proprietary;
		
}




/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Schedule_Valid_Instance(uint32_t object_instance)
{
    if ((object_instance < MAX_SCHEDULES + OBJECT_BASE)/* && (object_instance >= OBJECT_BASE)*/)
        return true;
		else
			return false;
}

/* we simply have 0-n object instances. */
unsigned Schedule_Count(void)
{	
		return SCHEDULES;
}

/* we simply have 0-n object instances. */
uint32_t Schedule_Index_To_Instance(unsigned index)
{
    return index + OBJECT_BASE;
}

/* we simply have 0-n object instances. */
unsigned Schedule_Instance_To_Index(
    uint32_t object_instance)
{

	return object_instance - OBJECT_BASE; 
}


bool Schedule_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name)
{
    bool status = false;
		unsigned index = 0;

    index = Schedule_Instance_To_Index(object_instance);
	
    if (object_instance < MAX_SCHEDULES) {
        status = characterstring_init_ansi(object_name, get_label(SCHEDULE,index));
    }

    return status;
}

/* return apdu length, or -1 on error */
/* assumption - object has already exists */
// read
int Schedule_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING far  bit_string;
    BACNET_CHARACTER_STRING far  char_string;
    unsigned  far object_index;

		int  far len = 0;
	  int far i;
	  int far day;
		object_index = Schedule_Instance_To_Index(object_instance);
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = 
                encode_application_object_id(&apdu[0], OBJECT_SCHEDULE,
                object_index);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different.
               Note that Object-Name must be unique in this device */
        case PROP_OBJECT_NAME:
						characterstring_init_ansi(&char_string, get_label(SCHEDULE,object_index));
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
						characterstring_init_ansi(&char_string,get_description(SCHEDULE,object_index));
						apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_SCHEDULE);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len =
                encode_application_boolean(&apdu[0], Get_bacnet_value_from_buf(SCHEDULE,0,object_index)/*AI_Present_Value[object_index]*/);
            break;       
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(&apdu[0], Get_Out_Of_Service(SCHEDULE,object_index));
            break;
        case PROP_WEEKLY_SCHEDULE:	
				{
						BACNET_TIME_VALUE array;
            if(array_index == 0)       /* count, always 7 */
						{
               // apdu_len = encode_application_unsigned(&apdu[0], 7);
							apdu_len = 0;
						}
            else if (array_index == BACNET_ARRAY_ALL) { /* full array */
						
                for (day = 0; day < 7; day++) { 
									apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                    for (i = 0; i < Get_TV_count(object_index,day); i++) 
									  {											
											array =  Get_Time_Value(object_index,day,i);										
											
											// to be fixed, array->value is not correct, have to define tag and Enumerated again.
											// ????????
											#if 1
											array.Value.tag = 9;
											array.Value.type.Enumerated = Get_WR_ON_OFF(object_index,day,i);//(i + 1) % 2;
											
                       apdu_len +=
                            bacapp_encode_time_value(&apdu[apdu_len],
                            &array/*Get_Time_Value(object_index,day,i)*/);
											#endif
											
											#if 0// new 
											apdu_len +=
                            bacapp_encode_time_value(&apdu[apdu_len],
                           array);
											#endif
                    }
                    apdu_len += encode_closing_tag(&apdu[apdu_len], 0);

                }
            } 
						else if (array_index <= 7) {      /* some array element */
							
                int day = array_index - 1; 
                apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                for (i = 0; i < Get_TV_count(object_index,day)/*CurrentSC->Weekly_Schedule[day].TV_Count*/; i++) {
                   
									array =  Get_Time_Value(object_index,day,i);
									#if 1
											array.Value.tag = 9;
											array.Value.type.Enumerated = Get_WR_ON_OFF(object_index,day,i);//(i + 1) % 2;
											
                       apdu_len +=
                            bacapp_encode_time_value(&apdu[apdu_len],
                            &array/*Get_Time_Value(object_index,day,i)*/);
									#endif
									
								#if 0 // new 
									apdu_len +=
                        bacapp_encode_time_value(&apdu[apdu_len],
                        &array/*Get_Time_Value(object_index,day,i)*//*&CurrentSC->Weekly_Schedule[day].Time_Values[i]*/);
                #endif
								}
								
                apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
            } else {    /* out of bounds */
						
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }
					}
            break;
        case PROP_EFFECTIVE_PERIOD:
				{
// to be fixed, encode_bacnet_date cant run corrctly,?????????????
					
//					apdu_len = encode_bacnet_date(&apdu[0], &Start_Date);
//          apdu_len +=  encode_bacnet_date(&apdu[apdu_len], &End_Date);
					apdu[0] = 0xa4;
					apdu[1] = 0xff;
					apdu[2] = 0xff;
					apdu[3] = 0xff;
					apdu[4] = 0xff;
					
					apdu[5] = 0xa4;
					apdu[6] = 0xff;
					apdu[7] = 0xff;
					apdu[8] = 0xff;
					apdu[9] = 0xff;
					
					apdu_len = 10;
//					  if (Start_Date.year >= 1900) {
//								apdu[0] = (uint8_t) (Start_Date.year - 1900);
//						} else if (Start_Date.year < 0x100) {
//								apdu[0] = (uint8_t) Start_Date.year;

//						} else {
//								/*
//								 ** Don't try and guess what the user meant here. Just fail
//								 */
//								apdu_len = BACNET_STATUS_ERROR;
//						}
//						apdu[1] = Start_Date.month;
//						apdu[2] = Start_Date.day;
//						apdu[3] = Start_Date.wday;
//					
//					  if (End_Date.year >= 1900) {
//								apdu[4] = (uint8_t) (End_Date.year - 1900);
//						} else if (End_Date.year < 0x100) {
//								apdu[4] = (uint8_t) End_Date.year;

//						} else {
//								/*
//								 ** Don't try and guess what the user meant here. Just fail
//								 */
//								apdu_len = BACNET_STATUS_ERROR;
//						}
//						apdu[5] = End_Date.month;
//						apdu[6] = End_Date.day;
//						apdu[7] = End_Date.wday;
//						apdu_len = 8;
            break;
				}
	      case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
//            for(i = 0; i < 1; i++) {
//                apdu_len +=
//                    bacapp_encode_device_obj_property_ref(&apdu[apdu_len],
//                   /* &CurrentSC->Object_Property_References[i]*/
//								Get_Object_Property_References(i));
//            }
				// to be fixed, added by chelsea
						apdu_len = 0;
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
				case PROP_RELIABILITY:
            apdu_len =
                encode_application_enumerated(&apdu[0],
                RELIABILITY_NO_FAULT_DETECTED);
            break;
				case PROP_SCHEDULE_DEFAULT:
					apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
				case PROP_EXCEPTION_SCHEDULE:  // fixed me
					
					apdu_len = 0;
					break;
				case PROP_PRIORITY_FOR_WRITING:
						apdu_len =
                encode_application_unsigned(&apdu[0],10);
					break;
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (array_index != BACNET_ARRAY_ALL)) {
			if(property != PROP_WEEKLY_SCHEDULE)  // added by chelsa
			{
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = -1;
			}
    }
	
    return apdu_len;
}


/* returns true if successful */
// write
bool Schedule_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    unsigned int  object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE far value;
		BACNET_TIME_VALUE far time_value;
    /* decode the some of the request */
	
	  if(!IS_CONTEXT_SPECIFIC(*wp_data->application_data))
		{
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
			if ((wp_data->object_property != PROP_EVENT_TIME_STAMPS) &&
					(wp_data->array_index != BACNET_ARRAY_ALL)) {
					wp_data->error_class = ERROR_CLASS_PROPERTY;
					wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;						
					return false;
			}
		}
		
		object_index = Schedule_Instance_To_Index(wp_data->object_instance);
    switch ((int) wp_data->object_property) {
        case PROP_PRESENT_VALUE:
			if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN/*BACNET_APPLICATION_TAG_REAL*/) {

				wirte_bacnet_value_to_buf(SCHEDULE,wp_data->priority,object_index,value.type.Boolean);

                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }

            break;
				// add it by chelsea
				case PROP_WEEKLY_SCHEDULE:   
				{		
					U16_T i = 0; // data_index
					U8_T week = 0; // 0e 0f
					U8_T len = 0;
					week = 0;
					
					for(i = 0;i < wp_data->application_data_len;)
					{
						if(wp_data->application_data[i] == 0x0E)  // start
						{		
							len = 0;	
							Clear_Time_Value(object_index,week);
							i++;
						}
						else if(wp_data->application_data[i] == 0x0F)  
						{
							i++;
							// set tv count
							if(len % 7 == 0) // it is end
							{
//								Check_wr_time_on_off(object_index,week,1);
								week++;
								len = 0;
							}
						}
						else
						{
							if(len % 7 == 0)
							{
								bacapp_decode_time_value(&wp_data->application_data[i],&time_value);
#if ARM
								write_Time_Value(object_index,week,len / 7,time_value.Time.hour,time_value.Time.min,wp_data->application_data[i + 6]);	
#else
								write_Time_Value(object_index,week,len / 7,time_value,wp_data->application_data[i + 6]);	
#endif
								
								len += 7;
								i += 7;
							}
						}							
					}
			
          status = true;
				}
				break;
				case PROP_OBJECT_NAME:	
				if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {

					write_bacnet_name_to_buf(SCHEDULE,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
				break;
				case PROP_DESCRIPTION:
								if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {


					write_bacnet_description_to_buf(SCHEDULE,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
				break;		
						
        case PROP_OUT_OF_SERVICE:
				if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {

					write_Out_Of_Service(SCHEDULE,object_index,value.type.Boolean);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }				
				
            break; 
        case PROP_EFFECTIVE_PERIOD: 
					
						len = decode_application_date(wp_data->application_data,&Start_Date);
						len = decode_application_date(&wp_data->application_data[len],&End_Date);
				
						if(len > 0)
							status = true;
						
            break;
	      case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
						status = true;
				
//            for (i = 0; i < 1; i++) {
//                apdu_len +=
//                    bacapp_encode_device_obj_property_ref(&apdu[apdu_len],
//                    Get_Object_Property_References(i));
//            }
            break;
				case PROP_EXCEPTION_SCHEDULE:// to be added

						break;
				case PROP_PRIORITY_FOR_WRITING:
					
					break;
      	case PROP_OBJECT_IDENTIFIER:         
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        
        case PROP_RELIABILITY: 
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}

#endif