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
#include "wp.h"
/* demo objects */
#include "device.h"
#include "av.h"
#include "bv.h"
#include "ai.h"
#include "ao.h"
#include "bo.h"	
#include "bi.h"
#include "proprietary.h"
#include "bacfile.h"


#if BAC_COMMON

int Binary_Output_Read_Property(
        BACNET_READ_PROPERTY_DATA  *  rpdata);
bool Analog_Output_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);
bool Binary_Output_Write_Property(
        BACNET_WRITE_PROPERTY_DATA  *  wp_data);

		
/* too big to reside on stack frame for PIC */
BACNET_WRITE_PROPERTY_DATA far wp_data;

void handler_write_property(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data,uint8_t protocal)
{
    int far len = 0;
    int far pdu_len = 0;
    BACNET_NPDU_DATA far npdu_data;
    BACNET_ERROR_CLASS far error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE far error_code = ERROR_CODE_UNKNOWN_OBJECT;
    int far bytes_sent = 0;
    BACNET_ADDRESS far my_address;
			   
    /* decode the service request only */
    len = wp_decode_service_request(service_request, service_len, (BACNET_WRITE_PROPERTY_DATA  *)&wp_data);
	/* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address,protocal);
		npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[protocal][0], src, &my_address,
        &npdu_data);

    /* bad decoding or something we didn't understand - send an abort */
    if (len <= 0) {
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
    } else if (service_data->segmented_message) {

        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
    } else {

        switch (wp_data.object_type) {
            case OBJECT_DEVICE:
							//if(wp_data.object_property != PROP_PROPERTY_LIST)
							{
								if (Device_Write_Property(&wp_data, &error_class, &error_code)) {
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else {
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
							}
//							else
//							{			
//								const int *pRequired = NULL, *pOptional = NULL, *pProprietary = NULL;	
//								BACNET_APPLICATION_DATA_VALUE far value;
//								len = bacapp_decode_application_data(wp_data.application_data,
//											wp_data.application_data_len, &value);
//										Device_Property_Lists(&pRequired, &pOptional, &pProprietary);
//										len =
//                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
//                        service_data->invoke_id,
//                        SERVICE_CONFIRMED_WRITE_PROPERTY);
//								
//							}
                
                break;
            case OBJECT_ANALOG_VALUE:

                if (Analog_Value_Write_Property(&wp_data, &error_class,
                        &error_code)) {

                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else {

                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
                break;
		// added by chelsea

			case OBJECT_ANALOG_INPUT:
                if (Analog_Input_Write_Property(&wp_data)) {
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else {
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
                break;
#if BAC_SCHEDULE
			case OBJECT_SCHEDULE:
                if (Schedule_Write_Property(&wp_data)) { 
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else {
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
                break;
#endif
			case OBJECT_ANALOG_OUTPUT:
                if (Analog_Output_Write_Property(&wp_data)) {
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else {
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
                break;
			case OBJECT_BINARY_OUTPUT:

                if (Binary_Output_Write_Property(&wp_data)) {

                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else {

                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
                break;
#if BAC_BI
			case OBJECT_BINARY_INPUT:
                if (Binary_Input_Write_Property(&wp_data)) {
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else {
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
                break;
#endif
#if BAC_BV
            case OBJECT_BINARY_VALUE:
                if (Binary_Value_Write_Property(&wp_data, &error_class,
                        &error_code)) {
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else {
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
                break;
#endif
#if BAC_CALENDAR
						case OBJECT_CALENDAR:
                if (Calendar_Write_Property(&wp_data)) { 
									
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else { 
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
								
                break;
#endif
#if BAC_TRENDLOG
						case OBJECT_TRENDLOG:
                if (Trend_Log_Write_Property(&wp_data)) { 
									
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else { 
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
								
                break;
#endif
#if BAC_PROPRIETARY
	case PROPRIETARY_BACNET_OBJECT_TYPE:
						  if (TemcoVars_Write_Property(&wp_data, &error_class,
                        &error_code)) {
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else { 
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
								break;
#endif
#if BAC_MSV
	case OBJECT_MULTI_STATE_VALUE:
						  if (Multistate_Value_Write_Property(&wp_data, &error_class,
                        &error_code)) {
									
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else { 
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
#endif

#if BAC_FILE
	case OBJECT_FILE:
						  if (bacfile_write_property(&wp_data, &error_class,
                        &error_code)) {
									
                    len =
                        encode_simple_ack(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY);
                } else { 
                    len =
                        bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                        service_data->invoke_id,
                        SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                        error_code);
                }
#endif
            default:
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],
                    service_data->invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
                    error_class, error_code);
                break;
        }
    }
    pdu_len += len;
    bytes_sent =
        datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[protocal][0],
        pdu_len,protocal);

    return;
}


bool WPValidateArgType(
    BACNET_APPLICATION_DATA_VALUE * pValue,
    uint8_t ucExpectedTag,
    BACNET_ERROR_CLASS * pErrorClass,
    BACNET_ERROR_CODE * pErrorCode)
{
    bool bResult;

    /*
     * start out assuming success and only set up error
     * response if validation fails.
     */
    bResult = true;
    if (pValue->tag != ucExpectedTag) {
        bResult = false;
        *pErrorClass = ERROR_CLASS_PROPERTY;
        *pErrorCode = ERROR_CODE_INVALID_DATA_TYPE;
    }

    return (bResult);
}


#endif