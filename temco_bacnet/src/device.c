/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
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
#include "bacnet.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacstr.h"
#include "bacenum.h"
#include "apdu.h"
#include "dcc.h"
#include "dlmstp.h"
//#include "rs485.h"
#include "version.h"
//#include "stack.h"
/* objects */
#include "device.h"	
#include "string.h"
#include "proplist.h"

#if BAC_SCHEDULE
#include "schedule.h"
#endif

#if BAC_CALENDAR
#include "calendar.h"
#endif

#if BAC_TRENDLOG
#include "trendlog.h"
#endif

#if BAC_PROPRIETARY
#include "proprietary.h"
#endif

#if BAC_FILE
#include "bacfile.h"
#endif



//#include "user_data.h"
#include "bi.h"
#include "ai.h"
//#include "bo.h"
//#include "ao.h"
int Binary_Output_Read_Property(
        BACNET_READ_PROPERTY_DATA xdata *  rpdata);
bool Analog_Output_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);
bool Binary_Output_Write_Property(
        BACNET_WRITE_PROPERTY_DATA  *  wp_data);
bool Binary_Output_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
bool Analog_Output_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);

/* note: you really only need to define variables for
   properties that are writable or that may change.
   The properties that are constant can be hard coded
   into the read-property encoding. */
uint32_t Object_Instance_Number = 260001;
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
//char text_string[20]; /* okay for single thread */
char far object_name[20];

#define MAX_REG 50
extern char reg_name[MAX_REG][15];//char* reg_name[MAX_REG];
uint16_t get_reg_value(uint16_t index);

static struct my_object_functions {
    BACNET_OBJECT_TYPE Object_Type;
    object_init_function Object_Init;
    object_count_function Object_Count;
    object_index_to_instance_function Object_Index_To_Instance;
    object_valid_instance_function Object_Valid_Instance;
    object_name_function Object_Name;
		read_property_function Object_Read_Property;
    write_property_function Object_Write_Property;
    rpm_property_lists_function Object_RPM_List;
		rr_info_function Object_RR_Info;
		object_value_list_function Object_Value_List;
    object_cov_function Object_COV;
    object_cov_clear_function Object_COV_Clear;
    object_intrinsic_reporting_function Object_Intrinsic_Reporting;
		} Object_Table[] = {
		{  // device
        OBJECT_DEVICE,
				NULL,    /* don't init - recursive! */
				Device_Count, 
			  Device_Index_To_Instance,
				Device_Valid_Object_Instance_Number, 
				Device_Object_Name,
				Device_Encode_Property_APDU,/*Device_Read_Property_Local*/
				/*Device_Write_Property,*/Device_Write_Property_Local,
				Device_Property_Lists,NULL,
				NULL,
				NULL,
				NULL,
				NULL}, 
		{  // AI			
				OBJECT_ANALOG_INPUT, NULL, Analog_Input_Count,
				Analog_Input_Index_To_Instance, Analog_Input_Valid_Instance,
				Analog_Input_Object_Name, Analog_Input_Encode_Property_APDU,
				NULL/*Binary_Output_Write_Property*/, 
				Analog_Input_Property_Lists,NULL,
				Analog_Input_Encode_Value_List,
				NULL,
				NULL,
				NULL  }, 
#if BAC_BI
		{  // BI			
				OBJECT_BINARY_INPUT, NULL, Binary_Input_Count,
				Binary_Input_Index_To_Instance, Binary_Input_Valid_Instance,
				Binary_Input_Object_Name, Binary_Input_Read_Property,
				NULL/*Binary_Output_Write_Property*/, 
				Binary_Input_Property_Lists,NULL ,
				Binary_Input_Encode_Value_List,
				NULL,
				NULL,
				NULL  		}, 
#endif
		{  // AO
				OBJECT_ANALOG_OUTPUT,  NULL,/*Analog_Output_Init,*/ Analog_Output_Count,
				Analog_Output_Index_To_Instance, Analog_Output_Valid_Instance,
				Analog_Output_Object_Name, Analog_Output_Encode_Property_APDU,
				NULL/*Binary_Output_Write_Property*/, 
				Analog_Output_Property_Lists,NULL,
				NULL,
				NULL,
				NULL,
				NULL	}, 
		{  // AV
				OBJECT_ANALOG_VALUE, NULL, Analog_Value_Count,
				Analog_Value_Index_To_Instance, Analog_Value_Valid_Instance,
				Analog_Value_Object_Name, Analog_Value_Encode_Property_APDU,
				Analog_Value_Write_Property/*Binary_Output_Write_Property*/, 
				Analog_Value_Property_Lists,NULL,
				Analog_Value_Encode_Value_List,
				NULL,
				NULL,
				NULL	},
#if BAC_BV		
		{  // BV
				OBJECT_BINARY_VALUE, NULL, Binary_Value_Count,
				Binary_Value_Index_To_Instance, Binary_Value_Valid_Instance,
				Binary_Value_Object_Name, Binary_Value_Encode_Property_APDU,
				Binary_Value_Write_Property/*Binary_Output_Write_Property*/, 
				Binary_Value_Property_Lists,NULL},
#endif
		{  // BO
				OBJECT_BINARY_OUTPUT, NULL,/*Binary_Output_Init,*/ Binary_Output_Count,
				Binary_Output_Index_To_Instance, Binary_Output_Valid_Instance,
				Binary_Output_Object_Name, Binary_Output_Encode_Property_APDU,
				NULL/*Binary_Output_Write_Property*/, 
				Binary_Output_Property_Lists,NULL}, 
#if BAC_CALENDAR
		{  // CALENDAR
				OBJECT_CALENDAR, NULL, Calendar_Count,
				Calendar_Index_To_Instance, Calendar_Valid_Instance,
				Calendar_Object_Name, Calendar_Encode_Property_APDU,
				NULL/*Calendar_Write_Property*/, 
				Calendar_Property_Lists,NULL}, 
#endif
				
#if BAC_SCHEDULE
		{  // OBJECT_SCHEDULE
				OBJECT_SCHEDULE, NULL, Schedule_Count,
				Schedule_Instance_To_Index, Schedule_Valid_Instance,
				Schedule_Object_Name, Schedule_Encode_Property_APDU,
				NULL/*Calendar_Write_Property*/, 
				Schedule_Property_Lists,NULL}, 
#endif
				
#if  BAC_TRENDLOG
    {	// OBJECT TRENDLOG
				OBJECT_TRENDLOG, 
				Trend_Log_Init,  
				Trend_Log_Count,
				Trend_Log_Index_To_Instance, 
				Trend_Log_Valid_Instance,
				Trend_Log_Object_Name,   
				Trend_Log_Encode_Property_APDU,//Trend_Log_Read_Property,
				NULL,//Trend_Log_Write_Property,
				Trend_Log_Property_Lists,
				TrendLogGetRRInfo                                                                                                                                                               
		},					
#endif
	
#if  BAC_PROPRIETARY
    {	// OBJECT PROPRIETARY
				PROPRIETARY_BACNET_OBJECT_TYPE, 
				NULL,//TemcoVars_Init,  
				TemcoVars_Count,
        TemcoVars_Index_To_Instance, 
				TemcoVars_Valid_Instance,
        NULL,//TemcoVars_Name,   
				TemcoVars_Encode_Property_APDU,//Trend_Log_Read_Property,
				TemcoVars_Write_Property,
				TemcoVars_Property_Lists,
				NULL                                                                                                                                                               
		},					
#endif
		
#if  BAC_MSV
    {	// OBJECT PROPRIETARY
				OBJECT_MULTI_STATE_VALUE, 
				NULL,//TemcoVars_Init,  
				Multistate_Value_Count,
        Multistate_Value_Index_To_Instance, 
				Multistate_Value_Valid_Instance,
        NULL,//TemcoVars_Name,   
				Multistate_Value_Read_Property,//Trend_Log_Read_Property,
				Multistate_Value_Write_Property,
				Multistate_Value_Property_Lists,
				NULL                                                                                                                                                               
		},					
#endif
		
#if  BAC_FILE
    {	// OBJECT FILE
				OBJECT_FILE, 
				NULL,//TemcoVars_Init,  
				bacfile_count,
        bacfile_index_to_instance, 
				bacfile_valid_instance,
        NULL,//TemcoVars_Name,   
				bacfile_read_property,//Trend_Log_Read_Property,
				bacfile_write_property,
				BACfile_Property_Lists,
				NULL                                                                                                                                                               
		},					
#endif
		{
			MAX_BACNET_OBJECT_TYPE, NULL, NULL, NULL, NULL, NULL, NULL, NULL,NULL}
};

static
#if ARM
 const 
#endif
int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_SYSTEM_STATUS,
    PROP_VENDOR_NAME,
    PROP_VENDOR_IDENTIFIER,
    PROP_MODEL_NAME,
    PROP_FIRMWARE_REVISION,
    PROP_APPLICATION_SOFTWARE_VERSION,
    PROP_PROTOCOL_VERSION,
    PROP_PROTOCOL_REVISION,
    PROP_PROTOCOL_SERVICES_SUPPORTED,
    PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
   	PROP_OBJECT_LIST, 
    PROP_MAX_APDU_LENGTH_ACCEPTED,
    PROP_SEGMENTATION_SUPPORTED,
    PROP_APDU_TIMEOUT,
    PROP_NUMBER_OF_APDU_RETRIES,
    PROP_DEVICE_ADDRESS_BINDING,
    PROP_DATABASE_REVISION,
	PROP_LOCAL_DATE,
	PROP_LOCAL_TIME,
	//TEMCO_REG1,
	//TEMCO_REG2,
	//TEMCO_REG3,
    -1
};

static
#if ARM
 const 
#endif
int Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_LOCATION,
    PROP_MAX_MASTER,
    PROP_MAX_INFO_FRAMES,	
		
    -1
};

static
#if ARM
 const 
#endif
int Properties_Proprietary[] = {
		TEMCO_REG1,
		TEMCO_REG2,
		TEMCO_REG3,
    -1
};


//void Set_Object_Name(char * name)
//{
//	memcpy(Object_Name,name,20);
//}

void Device_Init(
    void)
{
	U8_T i;
	//Object_Instance_Number = 260001;
    /* Reinitialize_State = BACNET_REINIT_IDLE; */
    /* dcc_set_status_duration(COMMUNICATION_ENABLE, 0); */
    /* FIXME: Get the data from the eeprom */
    /* I2C_Read_Block(EEPROM_DEVICE_ADDRESS,
       (char *)&Object_Instance_Number,
       sizeof(Object_Instance_Number),
       EEPROM_BACNET_ID_ADDR); */
	System_Status = STATUS_OPERATIONAL;
	
#if ASIX
	i= 0;
	System_Status = STATUS_OPERATIONAL;
	Object_Table[i].Object_Type = OBJECT_DEVICE;
	Object_Table[i].Object_Init = NULL;
	Object_Table[i].Object_Count = Device_Count;
	Object_Table[i].Object_Index_To_Instance = Device_Index_To_Instance;
	Object_Table[i].Object_Valid_Instance = Device_Valid_Object_Instance_Number;
	Object_Table[i].Object_Name = Device_Object_Name;
	Object_Table[i].Object_Read_Property = Device_Encode_Property_APDU;
	Object_Table[i].Object_Write_Property = Device_Write_Property;
	Object_Table[i].Object_RPM_List = Device_Property_Lists;
	
	i++;
	Object_Table[i].Object_Type = OBJECT_ANALOG_INPUT;
	Object_Table[i].Object_Init = NULL;
	Object_Table[i].Object_Count = Analog_Input_Count;
	Object_Table[i].Object_Index_To_Instance = Analog_Input_Index_To_Instance;
	Object_Table[i].Object_Valid_Instance = Analog_Input_Valid_Instance;
	Object_Table[i].Object_Name = NULL;
	Object_Table[i].Object_Read_Property = Analog_Input_Encode_Property_APDU;
	Object_Table[i].Object_Write_Property = NULL;
	Object_Table[i].Object_RPM_List = Analog_Input_Property_Lists;
#if BAC_BI
	i++;
	Object_Table[i].Object_Type = OBJECT_BINARY_INPUT;
	Object_Table[i].Object_Init = NULL;
	Object_Table[i].Object_Count = Binary_Input_Count;
	Object_Table[i].Object_Index_To_Instance = Binary_Input_Index_To_Instance;
	Object_Table[i].Object_Valid_Instance = Binary_Input_Valid_Instance;
	Object_Table[i].Object_Name = NULL;
	Object_Table[i].Object_Read_Property = Binary_Input_Read_Property;//Binary_Input_Encode_Property_APDU;
	Object_Table[i].Object_Write_Property = NULL;
	Object_Table[i].Object_RPM_List = Binary_Input_Property_Lists;
#endif	
	i++;
	Object_Table[i].Object_Type = OBJECT_ANALOG_OUTPUT;
	Object_Table[i].Object_Init = NULL;//Analog_Output_Init;
	Object_Table[i].Object_Count = Analog_Output_Count;
	Object_Table[i].Object_Index_To_Instance = Analog_Output_Index_To_Instance;
	Object_Table[i].Object_Valid_Instance = Analog_Output_Valid_Instance;
	Object_Table[i].Object_Name = NULL;
	Object_Table[i].Object_Read_Property = Analog_Output_Encode_Property_APDU;
	Object_Table[i].Object_Write_Property = NULL;
	Object_Table[i].Object_RPM_List = Analog_Output_Property_Lists;

	i++;
	Object_Table[i].Object_Type = OBJECT_ANALOG_VALUE;
	Object_Table[i].Object_Init = NULL;//Analog_Value_Init;
	Object_Table[i].Object_Count = Analog_Value_Count;
	Object_Table[i].Object_Index_To_Instance = Analog_Value_Index_To_Instance;
	Object_Table[i].Object_Valid_Instance = Analog_Value_Valid_Instance;
	Object_Table[i].Object_Name = NULL;
	Object_Table[i].Object_Read_Property = Analog_Value_Encode_Property_APDU;
	Object_Table[i].Object_Write_Property = NULL;
	Object_Table[i].Object_RPM_List = Analog_Value_Property_Lists;

#if BAC_BV	
	i++;
	Object_Table[i].Object_Type = OBJECT_BINARY_VALUE;
	Object_Table[i].Object_Init = NULL;//Analog_Value_Init;
	Object_Table[i].Object_Count = Binary_Value_Count;
	Object_Table[i].Object_Index_To_Instance = Binary_Value_Index_To_Instance;
	Object_Table[i].Object_Valid_Instance = Binary_Value_Valid_Instance;
	Object_Table[i].Object_Name = NULL;
	Object_Table[i].Object_Read_Property = Binary_Value_Encode_Property_APDU;
	Object_Table[i].Object_Write_Property = NULL;
	Object_Table[i].Object_RPM_List = Binary_Value_Property_Lists;
#endif	
	i++;
	Object_Table[i].Object_Type = OBJECT_BINARY_OUTPUT;
	Object_Table[i].Object_Init = NULL;// Binary_Output_Init;
	Object_Table[i].Object_Count = Binary_Output_Count;
	Object_Table[i].Object_Index_To_Instance = Binary_Output_Index_To_Instance;
	Object_Table[i].Object_Valid_Instance = Binary_Output_Valid_Instance;
	Object_Table[i].Object_Name = NULL;
	Object_Table[i].Object_Read_Property = Binary_Output_Encode_Property_APDU;
	Object_Table[i].Object_Write_Property = NULL;
	Object_Table[i].Object_RPM_List = Binary_Output_Property_Lists;

	i++;
	Object_Table[i].Object_Type = OBJECT_CALENDAR;
	Object_Table[i].Object_Init = NULL;
	Object_Table[i].Object_Count = Calendar_Count;
	Object_Table[i].Object_Index_To_Instance = Calendar_Index_To_Instance;
	Object_Table[i].Object_Valid_Instance = Calendar_Valid_Instance;
	Object_Table[i].Object_Name = NULL;
	Object_Table[i].Object_Read_Property = Calendar_Encode_Property_APDU;
	Object_Table[i].Object_Write_Property = NULL;
	Object_Table[i].Object_RPM_List = NULL;

	i++;
	Object_Table[i].Object_Type = OBJECT_SCHEDULE;
	Object_Table[i].Object_Init = NULL;
	Object_Table[i].Object_Count = Schedule_Count;
	Object_Table[i].Object_Index_To_Instance = Schedule_Instance_To_Index;
	Object_Table[i].Object_Valid_Instance = Schedule_Valid_Instance;
	Object_Table[i].Object_Name = NULL;
	Object_Table[i].Object_Read_Property = Schedule_Encode_Property_APDU;
	Object_Table[i].Object_Write_Property = NULL;
	Object_Table[i].Object_RPM_List = Schedule_Property_Lists;
	
	i++;
	Object_Table[i].Object_Type = MAX_BACNET_OBJECT_TYPE;
	Object_Table[i].Object_Init = NULL;
	Object_Table[i].Object_Count = NULL;
	Object_Table[i].Object_Index_To_Instance = NULL;
	Object_Table[i].Object_Valid_Instance = NULL;
	Object_Table[i].Object_Name = NULL;
	Object_Table[i].Object_Read_Property = NULL;
	Object_Table[i].Object_Write_Property = NULL;
	Object_Table[i].Object_RPM_List = NULL;

//	confirmed_service_supported[0] = SERVICE_SUPPORTED_ACKNOWLEDGE_ALARM;
//	confirmed_service_supported[1] = SERVICE_SUPPORTED_CONFIRMED_COV_NOTIFICATION;
//	confirmed_service_supported[2] = SERVICE_SUPPORTED_CONFIRMED_EVENT_NOTIFICATION;
//	confirmed_service_supported[3] = SERVICE_SUPPORTED_GET_ALARM_SUMMARY;
//	confirmed_service_supported[4] = SERVICE_SUPPORTED_GET_ENROLLMENT_SUMMARY;
//	confirmed_service_supported[5] = SERVICE_SUPPORTED_SUBSCRIBE_COV;
//	confirmed_service_supported[6] = SERVICE_SUPPORTED_ATOMIC_READ_FILE;
//	confirmed_service_supported[7] = SERVICE_SUPPORTED_ATOMIC_WRITE_FILE;
//	confirmed_service_supported[8] = SERVICE_SUPPORTED_ADD_LIST_ELEMENT;
//	confirmed_service_supported[9] = SERVICE_SUPPORTED_REMOVE_LIST_ELEMENT;
//	confirmed_service_supported[10] = SERVICE_SUPPORTED_CREATE_OBJECT;
//	confirmed_service_supported[11] = SERVICE_SUPPORTED_DELETE_OBJECT;
	confirmed_service_supported[12] = SERVICE_SUPPORTED_READ_PROPERTY;
//	confirmed_service_supported[13] = SERVICE_SUPPORTED_READ_PROP_CONDITIONAL;
	confirmed_service_supported[14] = SERVICE_SUPPORTED_READ_PROP_MULTIPLE;
	confirmed_service_supported[15] = SERVICE_SUPPORTED_WRITE_PROPERTY;
	confirmed_service_supported[16] = SERVICE_SUPPORTED_WRITE_PROP_MULTIPLE;
//	confirmed_service_supported[17] = SERVICE_SUPPORTED_DEVICE_COMMUNICATION_CONTROL;
//	confirmed_service_supported[18] = SERVICE_SUPPORTED_PRIVATE_TRANSFER;
//	confirmed_service_supported[19] = SERVICE_SUPPORTED_TEXT_MESSAGE;
//	confirmed_service_supported[20] = SERVICE_SUPPORTED_REINITIALIZE_DEVICE;
//	confirmed_service_supported[21] = SERVICE_SUPPORTED_VT_OPEN;
//	confirmed_service_supported[22] = SERVICE_SUPPORTED_VT_CLOSE;
//	confirmed_service_supported[23] = SERVICE_SUPPORTED_VT_DATA;	
//	confirmed_service_supported[24] = SERVICE_SUPPORTED_AUTHENTICATE;
//	confirmed_service_supported[25] = SERVICE_SUPPORTED_REQUEST_KEY;
//	confirmed_service_supported[26] = SERVICE_SUPPORTED_READ_RANGE;
//	confirmed_service_supported[27] = SERVICE_SUPPORTED_LIFE_SAFETY_OPERATION;	
//	confirmed_service_supported[28] = SERVICE_SUPPORTED_SUBSCRIBE_COV_PROPERTY;
//	confirmed_service_supported[29] = SERVICE_SUPPORTED_GET_EVENT_INFORMATION;
	
	
//	unconfirmed_service_supported[0] = SERVICE_SUPPORTED_I_AM;
//	unconfirmed_service_supported[1] = SERVICE_SUPPORTED_I_HAVE;
//	unconfirmed_service_supported[2] = SERVICE_SUPPORTED_UNCONFIRMED_COV_NOTIFICATION;
//	unconfirmed_service_supported[3] = SERVICE_SUPPORTED_UNCONFIRMED_EVENT_NOTIFICATION;
//	unconfirmed_service_supported[4] = SERVICE_SUPPORTED_UNCONFIRMED_PRIVATE_TRANSFER;
//	unconfirmed_service_supported[5] = SERVICE_SUPPORTED_UNCONFIRMED_TEXT_MESSAGE;
//	unconfirmed_service_supported[6] = SERVICE_SUPPORTED_TIME_SYNCHRONIZATION;
//	unconfirmed_service_supported[7] = SERVICE_SUPPORTED_WHO_HAS;
//	unconfirmed_service_supported[8] = SERVICE_SUPPORTED_WHO_IS;
//	unconfirmed_service_supported[9] = SERVICE_SUPPORTED_UTC_TIME_SYNCHRONIZATION;

#endif
}



extern uint32_t far Instance;

/* methods to manipulate the data */
uint32_t Device_Object_Instance_Number(
    void)
{
    return Object_Instance_Number;
}

bool Device_Set_Object_Instance_Number(
    uint32_t object_id)
{
    bool status = true; /* return value */
     if (object_id <= BACNET_MAX_INSTANCE) {
        Object_Instance_Number = object_id;
			 Instance = Object_Instance_Number;
        /* FIXME: Write the data to the eeprom */
        /* I2C_Write_Block(
           EEPROM_DEVICE_ADDRESS,
           (char *)&Object_Instance_Number,
           sizeof(Object_Instance_Number),
           EEPROM_BACNET_ID_ADDR); */
    } else
        status = false;

    return status;
}

bool Device_Valid_Object_Instance_Number(
    uint32_t object_id)
{
    /* BACnet allows for a wildcard instance number */

    return (Object_Instance_Number == object_id);
}

uint16_t Device_Vendor_Identifier(
    void)
{
    return Get_Vendor_ID();
			//BACNET_VENDOR_ID;
}


#if 1

unsigned Device_Object_List_Count( void)
{
    unsigned count = 1; /* at least 1 for device object */

    /* FIXME: add objects as needed */
    count += Analog_Value_Count();
#if BAC_BV	
		count += Binary_Value_Count();
#endif
		count += Analog_Input_Count();
		count += Analog_Output_Count();
		count += Binary_Output_Count();
#if BAC_BI
		count += Binary_Input_Count();
#endif
#if BAC_SCHEDULE
	count += Schedule_Count();
#endif
#if BAC_CALENDAR
	count += Calendar_Count();
#endif
#if BAC_TRENDLOG
	count += Trend_Log_Count();
#endif
#if BAC_PROPRIETARY
	count += TemcoVars_Count();
#endif
#if BAC_MSV
	count += Multistate_Value_Count();
#endif
#if BAC_FILE
	count += bacfile_count();
#endif
    return count;
}

bool Device_Object_List_Identifier(
    unsigned array_index,
    int *object_type,
    uint32_t * instance)
{
    bool status = false;
    unsigned object_index = 0;
    unsigned object_count = 0;

    /* device object */
    if (array_index == 1) {
        *object_type = OBJECT_DEVICE;
        *instance = Object_Instance_Number;
        status = true;
    }
    /* normalize the index since
       we know it is not the previous objects */
    /* array index starts at 1 */
    object_index = array_index - 1;
    /* 1 for the device object */
    object_count = 1;
    /* FIXME: add objects as needed */
    /* analog value objects */
    if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Analog_Value_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_ANALOG_VALUE;
            *instance = Analog_Value_Index_To_Instance(object_index);
            status = true;
        }
    }
#if BAC_BV			
		    /* binary value objects */
    if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Binary_Value_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_BINARY_VALUE;
            *instance = Binary_Value_Index_To_Instance(object_index);
            status = true;
        }
    }
#endif		
	/* analog input objects */
    if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Analog_Input_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_ANALOG_INPUT;
            *instance = Analog_Input_Index_To_Instance(object_index);
            status = true;
        }
    }
	/* analog ouput objects */
    if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Analog_Output_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_ANALOG_OUTPUT;
            *instance = Analog_Output_Index_To_Instance(object_index);
            status = true;
        }
    }
	/* binary ouput objects */
    if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Binary_Output_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_BINARY_OUTPUT;
            *instance = Binary_Output_Index_To_Instance(object_index);
            status = true;
        }
    }
#if BAC_BI
	/* binary input objects */
    if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Binary_Input_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_BINARY_INPUT;
            *instance = Binary_Input_Index_To_Instance(object_index);
            status = true;
        }
    }
#endif
// SCHEDULE		
#if BAC_SCHEDULE
		if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Schedule_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_SCHEDULE;
            *instance = Schedule_Index_To_Instance(object_index);
            status = true;
        }
    }
#endif

#if BAC_CALENDAR		
// CALENDAR		
		if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Calendar_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_CALENDAR;
            *instance = Calendar_Index_To_Instance(object_index);
            status = true;
        }
    }
#endif
		
#if BAC_TRENDLOG		
// TRENDLOG			
		if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Trend_Log_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_TRENDLOG;
            *instance = Trend_Log_Index_To_Instance(object_index);
            status = true;
        }
    }
#endif
		
#if BAC_PROPRIETARY		
// PROPRIETARY			
		if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = TemcoVars_Count();
        if (object_index < object_count) {
            *object_type = PROPRIETARY_BACNET_OBJECT_TYPE;
            *instance = TemcoVars_Index_To_Instance(object_index);
            status = true;
        }
    }
#endif
		
#if BAC_MSV		
		if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = TemcoVars_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_MULTI_STATE_VALUE;
            *instance = Multistate_Value_Index_To_Instance(object_index);
            status = true;
        }
    }
#endif
		
#if BAC_FILE	
		if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = bacfile_count();
        if (object_index < object_count) {
            *object_type = OBJECT_FILE;
            *instance = bacfile_index_to_instance(object_index);
            status = true;
        }
    }
#endif
    return status;
}

#if 0//BAC_TIMESYNC
static void Update_Current_Time(
    void)
{
    UN_Time *tblock = NULL;
#if defined(_MSC_VER)
    uint16_t tTemp;
#else
    UN_Time tv;
#endif
/*
struct tm

int    tm_sec   Seconds [0,60].
int    tm_min   Minutes [0,59].
int    tm_hour  Hour [0,23].
int    tm_mday  Day of month [1,31].
int    tm_mon   Month of year [0,11].
int    tm_year  Years since 1900.
int    tm_wday  Day of week [0,6] (Sunday =0).
int    tm_yday  Day of year [0,365].
int    tm_isdst Daylight Savings flag.
*/
#if defined(_MSC_VER)
    time(&tTemp);
    tblock = localtime(&tTemp);
#else
    //if (gettimeofday(&tv, NULL) == 0) {
        tblock = &Rtc;//localtime(&tv->Clk.sec);
    //}
#endif

    if (tblock) {
        datetime_set_date(&Local_Date, (uint16_t) tblock->Clk.year + 1900,
            (uint8_t) tblock->Clk.mon + 1, (uint8_t) tblock->Clk.day);
#if !defined(_MSC_VER)
        datetime_set_time(/*Get_Local_Time()*/&Local_Time, (uint8_t) tblock->Clk.hour,
            (uint8_t) tblock->Clk.min, (uint8_t) tblock->Clk.sec,
            0);
#else
        datetime_set_time(/*Get_Local_Time()*/&Local_Time, (uint8_t) tblock->Clk.hour,
            (uint8_t) tblock->Clk.min, (uint8_t) tblock->Clk.sec, 0);
#endif
        if (tblock->Clk.is_dst) {
            Set_Daylight_Saving_Status(true);//Daylight_Savings_Status = true;
        } else {
            Set_Daylight_Saving_Status(false);//Daylight_Savings_Status = false;
        }
        /* note: timezone is declared in <time.h> stdlib. */
        Set_UTC_OFFset();//UTC_Offset = timezone / 60;
    } else {
        datetime_date_wildcard_set(/*Get_Local_Date()*/&Local_Date);
        datetime_time_wildcard_set(/*Get_Local_Time()*/&Local_Time);
        Set_Daylight_Saving_Status(false);//Daylight_Savings_Status = false;
    }
}

void Device_getCurrentDateTime(
    BACNET_DATE_TIME * DateTime)
{
    Update_Current_Time();
		memcpy( &DateTime->date,/*Get_Local_Date()*/&Local_Date,sizeof(BACNET_DATE));
		memcpy( &DateTime->time,/*Get_Local_Time()*/&Local_Time,sizeof(BACNET_TIME));
//    DateTime->date = &Local_Date;
//    DateTime->time = &Local_Time;
}

int32_t Device_UTC_Offset(void)
{
    Update_Current_Time();

    return Get_UTC_Offset();
}

bool Device_Daylight_Savings_Status(void)
{
    return Get_Daylight_Savings_Status();
}
#endif




/* return the length of the apdu encoded or -1 for error */
// read
int Device_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int far apdu_len = 0;   /* return value */
    int far len = 0;        /* apdu len intermediate value */
    BACNET_BIT_STRING  far bit_string;
    BACNET_CHARACTER_STRING  far char_string;
	  struct my_object_functions *pObject = NULL;
    unsigned far i = 0;
    int far object_type = 0;
    uint32_t far instance = 0;
    unsigned far count = 0;
    object_instance = object_instance;
    /* FIXME: change the hardcoded names to suit your application */
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_DEVICE,
                Object_Instance_Number);
            break;
        case PROP_OBJECT_NAME:
            characterstring_init_ansi(&char_string, Get_Object_Name());
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_DEVICE);
            break;
        case PROP_SYSTEM_STATUS:
            apdu_len = encode_application_enumerated(&apdu[0], System_Status);
            break;
        case PROP_VENDOR_NAME:
            characterstring_init_ansi(&char_string, Get_Vendor_Name());
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_VENDOR_IDENTIFIER: 
            apdu_len =
                encode_application_unsigned(&apdu[0],Device_Vendor_Identifier());
            break;
        case PROP_MODEL_NAME:
            characterstring_init_ansi(&char_string, Get_Vendor_Product());
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FIRMWARE_REVISION:
            characterstring_init_ansi(&char_string, BACNET_VERSION_TEXT);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_APPLICATION_SOFTWARE_VERSION:
            characterstring_init_ansi(&char_string, "1.0");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_PROTOCOL_VERSION:	
            apdu_len =
                encode_application_unsigned(&apdu[0], BACNET_PROTOCOL_VERSION);
            break;
        case PROP_PROTOCOL_REVISION: 
            apdu_len =
                encode_application_unsigned(&apdu[0],
                BACNET_PROTOCOL_REVISION);
            break;
        case PROP_PROTOCOL_SERVICES_SUPPORTED:
            /* Note: list of services that are executed, not initiated. */
            bitstring_init(&bit_string);
            for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++) {
                /* automatic lookup based on handlers set */
                bitstring_set_bit(&bit_string, (uint8_t) i,
                    apdu_service_supported((BACNET_SERVICES_SUPPORTED) i));
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
            /* Note: this is the list of objects that can be in this device,
               not a list of objects that this device can access */
            bitstring_init(&bit_string);
            /* must have the bit string as big as it can be */
            for (i = 0; i < MAX_ASHRAE_OBJECT_TYPE; i++) {
                /* initialize all the object types to not-supported */
                bitstring_set_bit(&bit_string, (uint8_t) i, false);
            }
						
						pObject = Object_Table;
            while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
                if ((pObject->Object_Count) && (pObject->Object_Count() > 0)) {
                    bitstring_set_bit(&bit_string, pObject->Object_Type, true);
                }
                pObject++;
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
						
            /* FIXME: indicate the objects that YOU support */						
//            bitstring_set_bit(&bit_string, OBJECT_DEVICE, true);
//            bitstring_set_bit(&bit_string, OBJECT_ANALOG_VALUE, true);
//            bitstring_set_bit(&bit_string, OBJECT_ANALOG_INPUT, true);
//						bitstring_set_bit(&bit_string, OBJECT_ANALOG_OUTPUT, true);
//            bitstring_set_bit(&bit_string, OBJECT_BINARY_OUTPUT, true);
//            bitstring_set_bit(&bit_string, OBJECT_BINARY_INPUT, true);
//            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OBJECT_LIST:	
            count = Device_Object_List_Count();
            /* Array element zero is the number of objects in the list */
            if (array_index == 0)
						{
                apdu_len = encode_application_unsigned(&apdu[0], count);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet.  Note that more than likely you will have */
            /* to return an error if the number of encoded objects exceeds */
            /* your maximum APDU size. */
						}
            else if (array_index == BACNET_ARRAY_ALL) {
                for (i = 1; i <= count; i++) {
                    Device_Object_List_Identifier(i, &object_type, &instance);
                    len =
                        encode_application_object_id(&apdu[apdu_len],
                        object_type, instance);
                    apdu_len += len;
                    /* assume next one is the same size as this one */
                    /* can we all fit into the APDU? */
                    if ((apdu_len + len) >= MAX_APDU) { 
                        /* Abort response */
                        *error_code =
                            ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                        apdu_len = BACNET_STATUS_ABORT;
                        break;
                    }
                }
            } else { 
                if (Device_Object_List_Identifier(array_index, &object_type,
                        &instance))
								{
                    apdu_len =
                        encode_application_object_id(&apdu[0], object_type,
                        instance);
								}
                else { 
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        case PROP_MAX_APDU_LENGTH_ACCEPTED:
            apdu_len = encode_application_unsigned(&apdu[0], MAX_APDU);
            break;
        case PROP_SEGMENTATION_SUPPORTED:
            apdu_len =
                encode_application_enumerated(&apdu[0], SEGMENTATION_NONE);
            break;
        case PROP_APDU_TIMEOUT:
            apdu_len = encode_application_unsigned(&apdu[0], 60000);
            break;
        case PROP_NUMBER_OF_APDU_RETRIES:
            apdu_len = encode_application_unsigned(&apdu[0], 0);
            break;
        case PROP_DEVICE_ADDRESS_BINDING:
            /* FIXME: encode the list here, if it exists */
						apdu_len = address_list_encode(&apdu[0], MAX_APDU);
            break;
		case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string, "Bacnet Product");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DATABASE_REVISION:
            apdu_len = encode_application_unsigned(&apdu[0], 0);
            break;
#if 0//BAC_TIMESYNC
			  case PROP_LOCAL_DATE: 
						apdu_len = encode_application_date(&apdu[0],&Local_Date);
						break;
				case PROP_LOCAL_TIME:
						apdu_len = encode_application_time(&apdu[0],&Local_Time);
						break;
#endif
#if 1
        case PROP_MAX_INFO_FRAMES:
            apdu_len =
                encode_application_unsigned(&apdu[0],
                dlmstp_max_info_frames());
            break;

        case PROP_MAX_MASTER:
            apdu_len = 
                encode_application_unsigned(&apdu[0], dlmstp_max_master());	// tbd: changed by chelsea
            break;
        case TEMCO_REG1:  // REGISTER NAME
        	for(i = 0; i <= MAX_REG; i++)
			{
        		//characterstring_init_ansi(&char_string, Get_Object_Name());
        		//apdu_len = encode_application_character_string(&apdu[0], &char_string);


        		characterstring_init_ansi(&char_string, reg_name[i]);
        		len = encode_application_character_string(&apdu[apdu_len],&char_string);
        		apdu_len += len;
				/* assume next one is the same size as this one */
				/* can we all fit into the APDU? */
				if ((apdu_len + len) >= MAX_APDU) {
						/* Abort response */
						*error_code =
								ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
						apdu_len = BACNET_STATUS_ABORT;
						break;
				}
			}
        	break;
        case TEMCO_REG2:  // VALUE
       // case TEMCO_REG3:
			for (i = 0; i <= MAX_REG; i++)
			{
				len = encode_application_unsigned(&apdu[apdu_len],
								get_reg_value(i));
				apdu_len += len;
				/* assume next one is the same size as this one */
				/* can we all fit into the APDU? */
				if ((apdu_len + len) >= MAX_APDU) {
						/* Abort response */
						*error_code =
								ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
						apdu_len = BACNET_STATUS_ABORT;
						break;
				}
			}
            break;
        /*case TEMCO_REG2:
            apdu_len = 
							encode_application_unsigned(&apdu[0], Test[1]);	// tbd: changed by chelsea
            break;*/
        case TEMCO_REG3:
            apdu_len = 
							encode_application_unsigned(&apdu[0], Test[19]);   // tbd: changed by chelsea
            break;
#endif			
	/*	cas#e PROP_PRESENT_VALUE:
			apdu_len = encode_application_real(&apdu[0], 33.3);
			break;	*/
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (property != PROP_OBJECT_LIST) &&
        (array_index != BACNET_ARRAY_ALL)) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }
    return apdu_len;
}

bool Device_Write_Property_Local(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value - false=error */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    uint8_t max_master = 0;

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
    if ((wp_data->object_property != PROP_OBJECT_LIST) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch ((int) wp_data->object_property) {
       case PROP_OBJECT_IDENTIFIER:
            if (value.tag == BACNET_APPLICATION_TAG_OBJECT_ID) {
                if ((value.type.Object_Id.type == OBJECT_DEVICE) &&
                    (Device_Set_Object_Instance_Number(value.type.Object_Id.instance))) {
							#if ARM
															Store_MASTER_To_Eeprom(value.type.Object_Id.instance);
							#endif
                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
//        case PROP_MAX_INFO_FRAMES:
//            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
//                if (value.type.Unsigned_Int <= 255) {
//                    dlmstp_set_max_info_frames(value.type.Unsigned_Int);
//                    status = true;
//                } else {
//                    wp_data->error_class = ERROR_CLASS_PROPERTY;
//                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
//                }
//            } else {
//                wp_data->error_class = ERROR_CLASS_PROPERTY;
//                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
//            }
//            break;

        case PROP_MAX_MASTER:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if ((value.type.Unsigned_Int > 0) &&
                    (value.type.Unsigned_Int <= 254)) {
                    max_master = value.type.Unsigned_Int;
                    dlmstp_set_max_master(max_master);
                    //eeprom_bytes_write(NV_EEPROM_MAX_MASTER, &max_master, 1);
											
											Store_MASTER_To_Eeprom(max_master);

                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

				case PROP_VENDOR_NAME:
					if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING)
					{
						Set_Vendor_Name((char *)value.type.Character_String.value);
						status = true;
					}
					break;
				case PROP_VENDOR_IDENTIFIER:
					if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT)
					{
						Set_Vendor_ID(value.type.Unsigned_Int);
						status = true;
					}
					break;
				
				case PROP_MODEL_NAME:
					if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING)
					{
						Set_Vendor_Product((char *)value.type.Character_String.value);
						status = true;
					}
					break;
			case PROP_OBJECT_NAME:
					if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
						Set_Object_Name((char *)value.type.Character_String.value);
						status = true;
//							status =
//									bacnet_name_write_unique(NV_EEPROM_DEVICE_NAME,
//									wp_data->object_type, wp_data->object_instance,
//									&value.type.Character_String, &wp_data->error_class,
//									&wp_data->error_code);
					} else {
							wp_data->error_class = ERROR_CLASS_PROPERTY;
							wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
					}
					break;
//			case PROP_DESCRIPTION:
//					if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
//							status =
//									bacnet_name_write(NV_EEPROM_DEVICE_DESCRIPTION,
//									&value.type.Character_String, &wp_data->error_class,
//									&wp_data->error_code);
//					} else {
//							wp_data->error_class = ERROR_CLASS_PROPERTY;
//							wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
//					}
//					break;
//        case PROP_LOCATION:
//            if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
//                status =
//                    bacnet_name_write(NV_EEPROM_DEVICE_LOCATION,
//                    &value.type.Character_String, &wp_data->error_class,
//                    &wp_data->error_code);
//            } else {
//                wp_data->error_class = ERROR_CLASS_PROPERTY;
//                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
//            }
//            break;
//        case 9600:
//            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
//                if ((value.type.Unsigned_Int <= 115200) &&
//                    (rs485_baud_rate_set(value.type.Unsigned_Int))) {
//                    status = true;
//                } else {
//                    wp_data->error_class = ERROR_CLASS_PROPERTY;
//                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
//                }
//            } else {
//                wp_data->error_class = ERROR_CLASS_PROPERTY;
//                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
//            }
//            break;
//        case PROP_OBJECT_TYPE:
//        case PROP_FIRMWARE_REVISION:
//        case PROP_APPLICATION_SOFTWARE_VERSION:
//        case PROP_DAYLIGHT_SAVINGS_STATUS:
//        case PROP_PROTOCOL_VERSION:
//        case PROP_PROTOCOL_REVISION:
//        case PROP_PROTOCOL_SERVICES_SUPPORTED:
//        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
//        case PROP_OBJECT_LIST:
//        case PROP_MAX_APDU_LENGTH_ACCEPTED:
//        case PROP_SEGMENTATION_SUPPORTED:
//        case PROP_DEVICE_ADDRESS_BINDING:
//        case PROP_DATABASE_REVISION:
//        case 512:
//        case 513:
//            wp_data->error_class = ERROR_CLASS_PROPERTY;
//            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
//            break;
#if 0//BAC_TIMESYNC
			  case PROP_LOCAL_DATE:
						if (value.tag == BACNET_APPLICATION_TAG_DATE) {

                status =
                    write_Local_Date(&value.type.Date);
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
						break;
				case PROP_LOCAL_TIME:
						if (value.tag == BACNET_APPLICATION_TAG_TIME) {
                status =
                    write_Local_Time(&value.type.Time);
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
						break;
#endif
				case TEMCO_REG1:
					break;
				case TEMCO_REG2:
					if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT)
					{
						status = true;
					}
					break;
				case TEMCO_REG3:
					if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT)
					{
						status = true;
					}
					
					break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}




bool Device_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool far status = false;        /* return value */
    int  far len = 0;
    BACNET_APPLICATION_DATA_VALUE far value;

    switch (wp_data->object_type) {
        case OBJECT_ANALOG_INPUT:
            if (Analog_Input_Valid_Instance(wp_data->object_instance)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        case OBJECT_ANALOG_VALUE:
            if (Analog_Value_Valid_Instance(wp_data->object_instance)) {
                status = Analog_Value_Write_Property(wp_data,error_class,error_code);
            }
            break;
#if BAC_BV	
				 case OBJECT_BINARY_VALUE:
            if (Binary_Value_Valid_Instance(wp_data->object_instance)) {
                status = Binary_Value_Write_Property(wp_data,error_class,error_code);
            }
            break;
#endif	
#if BAC_BI						
        case OBJECT_BINARY_INPUT:
            if (Binary_Input_Valid_Instance(wp_data->object_instance)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
#endif
				case OBJECT_BINARY_OUTPUT:
            if (Binary_Output_Valid_Instance(wp_data->object_instance)) {
               status = Binary_Output_Write_Property(wp_data);
            }
            break;
			 case OBJECT_ANALOG_OUTPUT:
            if (Analog_Output_Valid_Instance(wp_data->object_instance)) {
               status = Analog_Output_Write_Property(wp_data);
            }
            break;

        case OBJECT_DEVICE:
            if (Device_Valid_Object_Instance_Number(wp_data->object_instance)) {
                status = Device_Write_Property_Local(wp_data);
            }
            break;
				
        default:
            break;
    }

    return (status);
}

unsigned Device_Count( void)
{
    return 1;
}

uint32_t Device_Index_To_Instance(
    unsigned index)
{
    index = index;
    return Object_Instance_Number;
}


bool Device_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name)
{
    bool status = false;
		
    if (object_instance == Object_Instance_Number) {
//        status = characterstring_copy(object_name,(BACNET_CHARACTER_STRING *)Get_Object_Name());
       status = characterstring_init_ansi(object_name, Get_Object_Name());
		}
    return status;
}

void Device_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
	
#if ASIX
	
	  Properties_Required[0] = PROP_OBJECT_IDENTIFIER;
    Properties_Required[1] = PROP_OBJECT_NAME;
    Properties_Required[2] = PROP_OBJECT_TYPE;
    Properties_Required[3] = PROP_SYSTEM_STATUS;
    Properties_Required[4] = PROP_VENDOR_NAME;
    Properties_Required[5] = PROP_VENDOR_IDENTIFIER;
    Properties_Required[6] = PROP_MODEL_NAME;
    Properties_Required[7] = PROP_FIRMWARE_REVISION;
    Properties_Required[8] = PROP_APPLICATION_SOFTWARE_VERSION;
    Properties_Required[9] = PROP_PROTOCOL_VERSION;
    Properties_Required[10] = PROP_PROTOCOL_REVISION;
    Properties_Required[11] = PROP_PROTOCOL_SERVICES_SUPPORTED;
    Properties_Required[12] = PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED;
    Properties_Required[13] = PROP_OBJECT_LIST;
    Properties_Required[14] = PROP_MAX_APDU_LENGTH_ACCEPTED;
    Properties_Required[15] = PROP_SEGMENTATION_SUPPORTED;
    Properties_Required[16] = PROP_APDU_TIMEOUT;
    Properties_Required[17] = PROP_NUMBER_OF_APDU_RETRIES;
    Properties_Required[18] = PROP_DEVICE_ADDRESS_BINDING;
    Properties_Required[19] = PROP_DATABASE_REVISION;
		Properties_Required[20] = PROP_LOCAL_DATE;
		Properties_Required[21] = PROP_LOCAL_TIME;
    Properties_Required[22] = -1;

		Properties_Optional[0] = PROP_DESCRIPTION;
		Properties_Optional[1] =  PROP_LOCATION;
		Properties_Optional[2] =  PROP_MAX_MASTER;
		Properties_Optional[3] =  PROP_MAX_INFO_FRAMES;
		Properties_Optional[4] =  -1;

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

 /*unsigned property_list_count(
    const int *pList)
{
    unsigned property_count = 0;

    if (pList) {
        while (*pList != -1) {
            property_count++;
            pList++;
        }
    }
    return property_count;
}*/



static struct my_object_functions *Device_Objects_Find_Functions(
    BACNET_OBJECT_TYPE Object_Type)
{
    struct my_object_functions *pObject = NULL;

    pObject = &Object_Table[0];
    while (pObject->Object_Type < MAX_BACNET_OBJECT_TYPE) {
        /* handle each object type */
        if (pObject->Object_Type == Object_Type) {
            return (pObject);
        }

        pObject++;
    }

    return (NULL);
}



static int Read_Property_Common(
    struct my_object_functions *pObject,
    BACNET_READ_PROPERTY_DATA * rpdata)
{
    int far apdu_len = BACNET_STATUS_ERROR;
    BACNET_CHARACTER_STRING far char_string;
    uint8_t far *apdu = NULL;
		apdu_len = BACNET_STATUS_ERROR;
    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            /*  only array properties can have array options */
            if (rpdata->array_index != BACNET_ARRAY_ALL) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
                apdu_len = BACNET_STATUS_ERROR;
            } else {
                /* Device Object exception: requested instance
                   may not match our instance if a wildcard */
                if (rpdata->object_type == OBJECT_DEVICE) {
                    rpdata->object_instance = Object_Instance_Number;
                }
                apdu_len =
                    encode_application_object_id(&apdu[0], rpdata->object_type,
                    rpdata->object_instance);
            }
            break;
//        case PROP_OBJECT_NAME:
//            /*  only array properties can have array options */
//            if (rpdata->array_index != BACNET_ARRAY_ALL) {
//                rpdata->error_class = ERROR_CLASS_PROPERTY;
//                rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
//                apdu_len = BACNET_STATUS_ERROR;
//            } else {
//							if(pObject->Object_Type == )
//                characterstring_init_ansi(&char_string, "MINI");
//                if (pObject->Object_Name) {
//                    (void) pObject->Object_Name(rpdata->object_instance,
//                        &char_string);
//                }
//                apdu_len =
//                    encode_application_character_string(&apdu[0],
//                    &char_string);
//            }
//            break;
        case PROP_OBJECT_TYPE:
            /*  only array properties can have array options */
            if (rpdata->array_index != BACNET_ARRAY_ALL) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
                apdu_len = BACNET_STATUS_ERROR;
            } else {
                apdu_len =
                    encode_application_enumerated(&apdu[0],
                    rpdata->object_type);
            }
            break;
        default:
			
            if (pObject->Object_Read_Property) {
                //apdu_len = pObject->Object_Read_Property(rpdata);
							rpdata->error_class = ERROR_CLASS_PROPERTY;
              rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
#if ASIX
							if(pObject->Object_Type == OBJECT_DEVICE)
							apdu_len = Device_Encode_Property_APDU(&apdu[0],
                    rpdata->object_instance, rpdata->object_property,
                    rpdata->array_index,  &rpdata->error_class, &rpdata->error_code);
					
							if(pObject->Object_Type == OBJECT_ANALOG_INPUT)
							apdu_len = Analog_Input_Encode_Property_APDU(&apdu[0],
                    rpdata->object_instance, rpdata->object_property,
                    rpdata->array_index,  &rpdata->error_class, &rpdata->error_code);
							
							if(pObject->Object_Type == OBJECT_ANALOG_OUTPUT)
							apdu_len = Analog_Output_Encode_Property_APDU(&apdu[0],
                    rpdata->object_instance, rpdata->object_property,
                    rpdata->array_index,  &rpdata->error_class, &rpdata->error_code);
							
							if(pObject->Object_Type == OBJECT_ANALOG_VALUE)
							apdu_len = Analog_Value_Encode_Property_APDU(&apdu[0],
                    rpdata->object_instance, rpdata->object_property,
                    rpdata->array_index,  &rpdata->error_class, &rpdata->error_code);
#if BAC_BV								
							if(pObject->Object_Type == OBJECT_BINARY_VALUE)
							apdu_len = Binary_Value_Encode_Property_APDU(&apdu[0],
                    rpdata->object_instance, rpdata->object_property,
                    rpdata->array_index,  &rpdata->error_class, &rpdata->error_code);
#endif							
							
							if(pObject->Object_Type == OBJECT_BINARY_OUTPUT)
							apdu_len = Binary_Output_Encode_Property_APDU(&apdu[0],
                    rpdata->object_instance, rpdata->object_property,
                    rpdata->array_index,  &rpdata->error_class, &rpdata->error_code);
							
							if(pObject->Object_Type == OBJECT_CALENDAR)
							apdu_len = Calendar_Encode_Property_APDU(&apdu[0],
                    rpdata->object_instance, rpdata->object_property,
                    rpdata->array_index,  &rpdata->error_class, &rpdata->error_code);
							
							if(pObject->Object_Type == OBJECT_SCHEDULE)
							apdu_len = Schedule_Encode_Property_APDU(&apdu[0],
                    rpdata->object_instance, rpdata->object_property,
                    rpdata->array_index,  &rpdata->error_class, &rpdata->error_code);
						
#endif
							
#if ARM
							apdu_len = pObject->Object_Read_Property(&apdu[0],
                    rpdata->object_instance, rpdata->object_property,
                    rpdata->array_index,  
									&rpdata->error_class, 
									&rpdata->error_code);
#endif
            }
            break;
    }

    return apdu_len;
}

int Device_Read_Property(
    BACNET_READ_PROPERTY_DATA * rpdata)
{
    int far apdu_len = BACNET_STATUS_ERROR;
    struct my_object_functions far *pObject = NULL;

    /* initialize the default return values */
    pObject = Device_Objects_Find_Functions(rpdata->object_type);
    if (pObject) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(rpdata->object_instance)) {
            apdu_len = Read_Property_Common(pObject, rpdata);
        } else {
            rpdata->error_class = ERROR_CLASS_OBJECT;
            rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
    } else {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return apdu_len;
}

///* for a given object type, returns the special property list */
void Device_Objects_Property_List(
    BACNET_OBJECT_TYPE object_type,
    struct special_property_list_t *pPropertyList)
{
    struct my_object_functions *pObject = NULL;

    pPropertyList->Required.pList = NULL;
    pPropertyList->Optional.pList = NULL;
    pPropertyList->Proprietary.pList = NULL;

    /* If we can find an entry for the required object type
     * and there is an Object_List_RPM fn ptr then call it
     * to populate the pointers to the individual list counters.
     */
    pObject = Device_Objects_Find_Functions(object_type);
    if ((pObject != NULL) && (pObject->Object_RPM_List != NULL)) { 
			// FIX ME, added by chelsea
#if ARM
        pObject->Object_RPM_List(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, 
					&pPropertyList->Proprietary.pList);
#endif
	
#if ASIX
					if(pObject->Object_Type == OBJECT_DEVICE)
							Device_Property_Lists(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, 
					&pPropertyList->Proprietary.pList);
					
							if(pObject->Object_Type == OBJECT_ANALOG_INPUT)
							Analog_Input_Property_Lists(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, 
					&pPropertyList->Proprietary.pList);
							
							if(pObject->Object_Type == OBJECT_ANALOG_OUTPUT)
							Analog_Output_Property_Lists(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, 
					&pPropertyList->Proprietary.pList);
							
							if(pObject->Object_Type == OBJECT_ANALOG_VALUE)
							Analog_Value_Property_Lists(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, 
					&pPropertyList->Proprietary.pList);
#if BAC_BV								
							if(pObject->Object_Type == OBJECT_BINARY_VALUE)
							Binary_Value_Property_Lists(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, 
					&pPropertyList->Proprietary.pList);
#endif							
							if(pObject->Object_Type == OBJECT_BINARY_OUTPUT)
							Binary_Output_Property_Lists(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, 
					&pPropertyList->Proprietary.pList);
							
//							if(pObject->Object_Type == OBJECT_CALENDAR)
//							Analog_Input_Property_Lists(&pPropertyList->Required.pList,
//            &pPropertyList->Optional.pList, 
//					&pPropertyList->Proprietary.pList);
							
							if(pObject->Object_Type == OBJECT_SCHEDULE)
							Schedule_Property_Lists(&pPropertyList->Required.pList,
            &pPropertyList->Optional.pList, 
					&pPropertyList->Proprietary.pList);		
			
			
#endif
    }

    /* Fetch the counts if available otherwise zero them */
    pPropertyList->Required.count =
        pPropertyList->Required.pList ==
        NULL ? 0 : property_list_count(pPropertyList->Required.pList);
    pPropertyList->Optional.count =
        pPropertyList->Optional.pList ==
        NULL ? 0 : property_list_count(pPropertyList->Optional.pList);

    pPropertyList->Proprietary.count =
        pPropertyList->Proprietary.pList ==
        NULL ? 0 : property_list_count(pPropertyList->Proprietary.pList);

    return;
}

/** Try to find a rr_info_function helper function for the requested object type.
 * @ingroup ObjIntf
 *
 * @param object_type [in] The type of BACnet Object the handler wants to access.
 * @return Pointer to the object helper function that implements the
 *         ReadRangeInfo function, Object_RR_Info, for this type of Object on
 *         success, else a NULL pointer if the type of Object isn't supported
 *         or doesn't have a ReadRangeInfo function.
 */
rr_info_function Device_Objects_RR_Info(
    BACNET_OBJECT_TYPE object_type)
{
    struct my_object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    return (pObject != NULL ? pObject->Object_RR_Info : NULL);
}


bool Device_Object_Name_Copy(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name)
{
    struct my_object_functions *pObject = NULL;
    bool found = false;

    pObject = Device_Objects_Find_Functions(object_type);
    if ((pObject != NULL) && (pObject->Object_Name != NULL)) {
        found = pObject->Object_Name(object_instance, object_name);
    }

    return found;
}


bool Device_Valid_Object_Name(
    BACNET_CHARACTER_STRING * object_name1,
    int *object_type,
    uint32_t * object_instance)
{
    bool found = false;
    int type = 0;
    uint32_t instance;
    unsigned max_objects = 0, i = 0;
    bool check_id = false;
    BACNET_CHARACTER_STRING object_name2;
    struct my_object_functions *pObject = NULL;

    max_objects = Device_Object_List_Count();
		
    for (i = 1; i <= max_objects; i++) {
        check_id = Device_Object_List_Identifier(i, &type, &instance);
        if (check_id) { 				
            pObject = Device_Objects_Find_Functions((BACNET_OBJECT_TYPE) type);
										
            if ((pObject != NULL) && (pObject->Object_Name != NULL) &&
                (pObject->Object_Name(instance, &object_name2) &&
                    characterstring_same(object_name1, &object_name2))) {
                found = true;
                if (object_type) {
                    *object_type = type;
                }
                if (object_instance) {
                    *object_instance = instance;
                }
                break;
            }
        }
    }

    return found;
}

#if COV

/** Looks up the requested Object, and fills the Property Value list.
 * If the Object or Property can't be found, returns false.
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @param [in] The object instance number to be looked up.
 * @param [out] The value list
 * @return True if the object instance supports this feature and value changed.
 */
bool Device_Encode_Value_List(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE * value_list)
{
    bool status = false;        /* Ever the pessamist! */
    struct my_object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(object_instance)) {
            if (pObject->Object_Value_List) {
                status =
                    pObject->Object_Value_List(object_instance, value_list);
            }
        }
    }

    return (status);
}

/** Checks the COV flag in the requested Object
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @param [in] The object instance to be looked up.
 * @return True if the COV flag is set
 */
bool Device_COV(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    bool status = false;        /* Ever the pessamist! */
    struct my_object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(object_instance)) {
            if (pObject->Object_COV) {
                status = pObject->Object_COV(object_instance);
            }
        }
    }

    return (status);
}

/** Clears the COV flag in the requested Object
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @param [in] The object instance to be looked up.
 */
void Device_COV_Clear(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    struct my_object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Valid_Instance &&
            pObject->Object_Valid_Instance(object_instance)) {
            if (pObject->Object_COV_Clear) {
                pObject->Object_COV_Clear(object_instance);
            }
        }
    }
}

#if defined(INTRINSIC_REPORTING)
void Device_local_reporting(
    void)
{
    struct object_functions *pObject;
    uint32_t objects_count;
    uint32_t object_instance;
    int object_type;
    uint32_t idx;

    objects_count = Device_Object_List_Count();

    /* loop for all objects */
    for (idx = 1; idx < objects_count; idx++) {
        Device_Object_List_Identifier(idx, &object_type, &object_instance);

        pObject = Device_Objects_Find_Functions(object_type);
        if (pObject != NULL) {
            if (pObject->Object_Valid_Instance &&
                pObject->Object_Valid_Instance(object_instance)) {
                if (pObject->Object_Intrinsic_Reporting) {
                    pObject->Object_Intrinsic_Reporting(object_instance);
                }
            }
        }
    }
}
#endif

/** Looks up the requested Object to see if the functionality is supported.
 * @ingroup ObjHelpers
 * @param [in] The object type to be looked up.
 * @return True if the object instance supports this feature.
 */
bool Device_Value_List_Supported(
    BACNET_OBJECT_TYPE object_type)
{
    bool status = false;        /* Ever the pessamist! */
    struct my_object_functions *pObject = NULL;

    pObject = Device_Objects_Find_Functions(object_type);
    if (pObject != NULL) {
        if (pObject->Object_Value_List) {
            status = true;
        }
    }

    return (status);
}
#endif
/* returns the name or NULL if not found */
// modify by chelsea ??????????
bool Device_Valid_Object_Id(
    int object_type,
    uint32_t object_instance)
{
    BACNET_CHARACTER_STRING *name = NULL;  /* return value */
    struct my_object_functions *pObject = NULL;
		bool status = false;
	
    pObject = Device_Objects_Find_Functions((BACNET_OBJECT_TYPE) object_type);
//    if ((pObject) && (pObject->Object_Name)) {
//        status = pObject->Object_Name(object_instance,name);
//    } modify by chelsea
	
		if ((pObject) && (pObject->Object_Valid_Instance(object_instance)))
		{
			status = true;
		}

    return status;
}

///** Looks up the requested Object to see if the functionality is supported.
// * @ingroup ObjHelpers
// * @param [in] The object type to be looked up.
// * @return True if the object instance supports this feature.
// */
//bool Device_Value_List_Supported(
//    BACNET_OBJECT_TYPE object_type)
//{
//    bool status = false;        /* Ever the pessamist! */
//    struct my_object_functions *pObject = NULL;

//    pObject = Device_Objects_Find_Functions((BACNET_OBJECT_TYPE)object_type);
//    if (pObject != NULL) {
//        if (pObject->Object_Value_List) {
//            status = true;
//        }
//    }

//    return (status);
//}
#endif
