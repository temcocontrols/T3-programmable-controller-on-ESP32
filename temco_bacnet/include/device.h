/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef DEVICE_H
#define DEVICE_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacenum.h"
#include "wp.h"
#include "readrange.h"
#include "av.h"
#include "bv.h"
#include "ai.h"
#include "ao.h"
#include "bo.h"	
#include "bi.h"

#include "rpm.h"




typedef unsigned (
    *object_count_function) (
    void);

//typedef char *(
//    *object_name_function)
//     (
//    uint32_t object_instance);
typedef bool(
    *object_name_function)
     (
    uint32_t object_instance,
		BACNET_CHARACTER_STRING * object_name);

typedef void (
    *object_init_function) (
    void);

/** Maps an object index position to its corresponding BACnet object instance number.
 * @ingroup ObjHelpers
 * @param index [in] The index of the object, in the array of objects of its type.
 * @return The BACnet object instance number to be used in a BACNET_OBJECT_ID.
 */
typedef uint32_t(
    *object_index_to_instance_function)
        (
    unsigned index);


/** Look in the table of objects of this type, and see if this is a valid
 *  instance number.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 * @return True if the object instance refers to a valid object of this type.
 */
typedef bool(
    *object_valid_instance_function) (
    uint32_t object_instance);

 rr_info_function Device_Objects_RR_Info(
        BACNET_OBJECT_TYPE object_type);


/** Look in the table of objects of this type, and get the COV Value List.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 * @param [out] The value list
 * @return True if the object instance supports this feature, and has changed.
 */
typedef bool(
    *object_value_list_function) (
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE * value_list);

/** Look in the table of objects for this instance to see if value changed.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 * @return True if the object instance has changed.
 */
typedef bool(
    *object_cov_function) (
    uint32_t object_instance);

/** Look in the table of objects for this instance to clear the changed flag.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 */
typedef void (
    *object_cov_clear_function) (
    uint32_t object_instance);

/** Intrinsic Reporting funcionality.
 * @ingroup ObjHelpers
 * @param [in] Object instance.
 */
typedef void (
    *object_intrinsic_reporting_function) (
    uint32_t object_instance);


bool Device_Value_List_Supported(
    BACNET_OBJECT_TYPE object_type);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Device_Object_Function_Set(
        BACNET_OBJECT_TYPE object_type,
        object_count_function count_function,
        object_index_to_instance_function index_function,
        object_name_function name_function);

    void Device_Init(
        void);
		void Set_Object_Name(char * name);
		
    void Device_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    uint32_t Device_Object_Instance_Number(
        void);
    bool Device_Set_Object_Instance_Number(
        uint32_t object_id);
		void Store_MASTER_To_Eeprom(uint8_t master);
		void Store_Instance_To_Eeprom(uint32_t Instance);

    bool Device_Valid_Object_Instance_Number(
        uint32_t object_id);
    unsigned Device_Object_List_Count(
        void);
    bool Device_Object_List_Identifier(
        unsigned array_index,
        int *object_type,
        uint32_t * instance);

    BACNET_DEVICE_STATUS Device_System_Status(
        void);
    void Device_Set_System_Status(
        BACNET_DEVICE_STATUS status);

    const char *Device_Vendor_Name(
        void);

    uint16_t Device_Vendor_Identifier(
        void);

    const char *Device_Model_Name(
        void);
    bool Device_Set_Model_Name(
        const char *name,
        size_t length);

    const char *Device_Firmware_Revision(
        void);

    const char *Device_Application_Software_Version(
        void);
    bool Device_Set_Application_Software_Version(
        const char *name,
        size_t length);

    bool Device_Set_Object_Name(
        const char *name,
        size_t length);
//    char *Device_Object_Name(
//        void);
	bool Device_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name);

    const char *Device_Description(
        void);
    bool Device_Set_Description(
        const char *name,
        size_t length);

    const char *Device_Location(
        void);
    bool Device_Set_Location(
        const char *name,
        size_t length);

    /* some stack-centric constant values - no set methods */
    uint8_t Device_Protocol_Version(
        void);
    uint8_t Device_Protocol_Revision(
        void);
    BACNET_SEGMENTATION Device_Segmentation_Supported(
        void);

    uint8_t Device_Database_Revision(
        void);
    void Device_Set_Database_Revision(
        uint8_t revision);

//    bool Device_Valid_Object_Name(
//        const char *object_name,
//        int *object_type,
//        uint32_t * object_instance);
    bool Device_Valid_Object_Id(
        int object_type,
        uint32_t object_instance);

    int Device_Encode_Property_APDU(
        uint8_t * apdu,
        uint32_t object_instance,
        BACNET_PROPERTY_ID property,
        uint32_t array_index,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);
		
		bool Device_Write_Property_Local(
    BACNET_WRITE_PROPERTY_DATA * wp_data);

    bool Device_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);

    bool DeviceGetRRInfo(
        uint32_t Object,        /* Which particular object - obviously not important for device object */
        BACNET_PROPERTY_ID Property,    /* Which property */
        RR_PROP_INFO * pInfo,   /* Where to put the information */
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);


		int Device_Read_Property(
    BACNET_READ_PROPERTY_DATA * rpdata);
		
		void Device_Objects_Property_List(
    BACNET_OBJECT_TYPE object_type,
    struct special_property_list_t *pPropertyList);
		
		
	unsigned Device_Count( void);
		uint32_t Device_Index_To_Instance(
    unsigned index);
	bool Device_Object_Name(
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name);	
	
	bool Device_Object_Name_Copy(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name);
	
	bool Device_Valid_Object_Name(
    BACNET_CHARACTER_STRING * object_name1,
    int *object_type,
    uint32_t * object_instance);
		
		bool Device_Encode_Value_List(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE * value_list);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
