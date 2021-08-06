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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "bacnet.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacerror.h"
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "rp.h"
/* device object has custom handler for all objects */
#include "device.h"
#include "av.h"
#include "bv.h"
#include "ai.h"
#include "ao.h"
#include "bo.h"	
#include "bi.h"
#include "bacfile.h"
//#include "handlers.h"
#include "proprietary.h"
#include "proplist.h"


#if BAC_COMMON

/* Encodes the property APDU and returns the length,
   or sets the error, and returns -1 */
int Encode_Property_APDU(
    uint8_t * apdu,
    BACNET_READ_PROPERTY_DATA * rp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = -1;
    /* handle each object type */
    switch (rp_data->object_type) {
        case OBJECT_DEVICE:
            /* Test for case of indefinite Device object instance */
            if (rp_data->object_instance == BACNET_MAX_INSTANCE) {
                rp_data->object_instance = Device_Object_Instance_Number();
            }
            if (Device_Valid_Object_Instance_Number(rp_data->object_instance)) {
							if(rp_data->object_property != PROP_PROPERTY_LIST)
							{
                apdu_len =
                    Device_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
							}
							else
							{
								const int *pRequired = NULL, *pOptional = NULL, *pProprietary = NULL;	
								Device_Property_Lists(&pRequired, &pOptional, &pProprietary);
#ifdef T3_CON
								apdu_len = property_list_encode(&apdu[0],rp_data, pRequired, pOptional, pProprietary);
#endif
							}
            }
            break;
        case OBJECT_ANALOG_VALUE:
            if (Analog_Value_Valid_Instance(rp_data->object_instance)) {
                apdu_len =
                    Analog_Value_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
            }
            break;
#if BAC_BV
        case OBJECT_BINARY_VALUE:
            if (Binary_Value_Valid_Instance(rp_data->object_instance)) {
                apdu_len =
                    Binary_Value_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
            }
            break;
#endif
		// added by chelsea
				case OBJECT_ANALOG_INPUT:
			
            if (Analog_Input_Valid_Instance(rp_data->object_instance)) {
				apdu_len =
                    Analog_Input_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
            }
            break;
        case OBJECT_ANALOG_OUTPUT:
            if (Analog_Output_Valid_Instance(rp_data->object_instance)) {
				apdu_len =
				Analog_Output_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
            }
            break;
		case OBJECT_BINARY_OUTPUT:
			
            if (Binary_Output_Valid_Instance(rp_data->object_instance)) {
  				apdu_len =
                    Binary_Output_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
            }
            break;
#if BAC_BI
		case OBJECT_BINARY_INPUT:
						if (Binary_Input_Valid_Instance(rp_data->object_instance)) {
				apdu_len =
                    Binary_Input_Read_Property(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
            }
            break;
#endif
#if BAC_SCHEDULE
		case OBJECT_SCHEDULE:
						if (Schedule_Valid_Instance(rp_data->object_instance)) {
								apdu_len =
								Schedule_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
            }
						break;
#endif
#if BAC_CALENDAR
	case OBJECT_CALENDAR:
						if (Calendar_Valid_Instance(rp_data->object_instance)) {
								apdu_len =
								Calendar_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
							
            }
						
            break;
#endif
#if BAC_TRENDLOG
	case OBJECT_TRENDLOG:
						if (Trend_Log_Valid_Instance(rp_data->object_instance)) {
								apdu_len =
								Trend_Log_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
							
            }
						
            break;
#endif
#if BAC_PROPRIETARY
	case PROPRIETARY_BACNET_OBJECT_TYPE: 
						if (TemcoVars_Valid_Instance(rp_data->object_instance)) {
								apdu_len =
								TemcoVars_Encode_Property_APDU(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
							
            }
						
            break;
#endif
#if BAC_MSV
	case OBJECT_MULTI_STATE_VALUE: 
						if (Multistate_Value_Valid_Instance(rp_data->object_instance)) {
								apdu_len =
								Multistate_Value_Read_Property(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
							
            }
						
            break;
#endif
#if BAC_FILE
	case OBJECT_FILE: 
						if (bacfile_valid_instance(rp_data->object_instance)) {
								apdu_len =
								bacfile_read_property(&apdu[0],
                    rp_data->object_instance, rp_data->object_property,
                    rp_data->array_index, error_class, error_code);
							
            }
						
            break;
#endif
        default:
            *error_class = ERROR_CLASS_OBJECT;
            *error_code = ERROR_CODE_UNKNOWN_OBJECT;
            break;
    }

    return apdu_len;
}

void handler_read_property(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data,
		uint8_t protocal)
{
    BACNET_READ_PROPERTY_DATA far dat;
    int far len = 0;
    int far ack_len = 0;
    int far property_len = 0;
    int far pdu_len = 0;
    BACNET_NPDU_DATA far npdu_data;
    int far bytes_sent = 0;
    BACNET_ERROR_CLASS far error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE  far error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_ADDRESS  far my_address;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address,protocal);
		
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[protocal][0], src, &my_address,
        &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
        goto RP_ABORT;
    }
    len = rp_decode_service_request(service_request, service_len, &dat);
    if (len < 0) {
        /* bad decoding - send an abort */
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
       goto RP_ABORT;
    }
    /* most cases will be error */
    ack_len =
        rp_ack_encode_apdu_init(&Handler_Transmit_Buffer[protocal][pdu_len],
        service_data->invoke_id, &dat);
    /* FIXME: add buffer len as passed into function or use smart buffer */
		
    property_len =
        Encode_Property_APDU(&Handler_Transmit_Buffer[protocal][pdu_len + ack_len],
        &dat, &error_class, &error_code);
    if (property_len >= 0) { 
        len =
            rp_ack_encode_apdu_object_property_end(&Handler_Transmit_Buffer[protocal]
            [pdu_len + property_len + ack_len]);
        len += ack_len + property_len;
    } else {
        switch (property_len) {
                /* BACnet APDU too small to fit data, so proper response is Abort */
            case -2:
                len =
                    abort_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                    service_data->invoke_id,
                    ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
                break;
            default:
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                    service_data->invoke_id, SERVICE_CONFIRMED_READ_PROPERTY,
                    error_class, error_code);
                break;
        }
    }
  	RP_ABORT:
    pdu_len += len;
    bytes_sent =
        datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[protocal][0],
        pdu_len,protocal);
    return;
}

#endif