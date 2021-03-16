/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2009 Steve Karg

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



#include <stdint.h>


// for temoc private application
#include "bacnet.h"



uint8_t invoke_id;
uint8_t header_len;
uint16_t transfer_len;

//uint16_t get_ptransfer_len(void)
//{
//	return transfer_len + header_len;
//}

//void handler_private_transfer( 	
//	uint8_t * apdu,
//  unsigned apdu_len,
//	BACNET_ADDRESS * src,uint8_t protocal)
//	{}
		
#if BAC_PRIVATE
typedef struct
{
	 uint16_t  		total_length;        /*	total length to be received or sent	*/
	 uint8_t		command;
	 uint8_t		point_start_instance;
	 uint8_t		point_end_instance;
	 uint16_t		entitysize;
//	 uint8_t		entitysize;
//	 uint8_t		codeindex;
}Str_user_data_header;

void responseCmd(u8 type, u8* pData);
void internalDeal(u8 type,  u8 *pData);
u16 crc16(u8 *p, u8 length);
uint8_t Get_modbus_address(void);
extern uint8_t 	bacnet_to_modbus[300];
void Get_Pkt_Bac_to_Modbus(Str_user_data_header * header)
{  
	uint8_t buf[300];
	u16 len;
	u16 crc_check; 
	//  for read command
	buf[0] = Get_modbus_address();
	if(header->command == 94)
	{
		buf[1] = 0x03;
		buf[4] = HIGH_BYTE(header->entitysize);		//0x00;
		buf[5] = LOW_BYTE(header->entitysize);   	// len		
		len = 6;
	}
	else if(header->command == 194)
	{
		if(header->total_length - 7 > 2) // muti-write
		{
			buf[1] = 0x10;
			buf[4] = HIGH_BYTE(header->entitysize);		//0x00;
			buf[5] = LOW_BYTE(header->entitysize);   	// len	
			buf[6] = header->total_length - 7;
			memcpy(&buf[7],bacnet_to_modbus,header->total_length - 7);
			len = header->total_length;
//			if(len > 100)  // error
//				return;
		}
		else
		{
			buf[1] = 0x06;
			buf[4] = bacnet_to_modbus[0];
			buf[5] = bacnet_to_modbus[1];
			len = 6;
		}
	}
	
	buf[2] = header->point_end_instance;  // high_byte(start_addr)
	buf[3] = header->point_start_instance;		// low_byte(start_addr)
	crc_check = crc16(buf, len);
	buf[len] = HIGH_BYTE(crc_check);
	buf[len + 1] = LOW_BYTE(crc_check);
	
	responseCmd(4/*BAC_TO_MODBUS*/,buf);	// get bacnet_to_modbus
	internalDeal(4, buf);

}


#if ARM
extern  unsigned char  Handler_Transmit_Buffer[2][MAX_PDU];
#endif

uint8_t temp[480] = {0};

void handler_private_transfer( 	
	uint8_t * apdu,
  unsigned apdu_len,
	BACNET_ADDRESS * src,uint8_t protocal)
{	
   BACNET_APPLICATION_DATA_VALUE data_value = { 0 };
   BACNET_APPLICATION_DATA_VALUE rec_data_value = { 0 };
    BACNET_PRIVATE_TRANSFER_DATA private_data = { 0 };
    BACNET_PRIVATE_TRANSFER_DATA rec_data = { 0 };
		Str_user_data_header	private_header;	
//		BACNET_OCTET_STRING Temp_CS;	
		uint8_t* ptr = NULL;
//		uint8_t test_value[480] = { 0 };
		int len = 0;
		int private_data_len = 0;
		int property_len = 0;
		//    BACNET_NPDU_DATA npdu_data;
		int bytes_sent = 0;
		bool status = false;
		uint8_t command = 0;	
		

	
	 U8_T j;

	 int iLen;   /* Index to current location in data */
   int tag_len;
	 uint8_t tag_number;
	 uint32_t len_value_type;

//   decode ptransfer
	len = ptransfer_decode_apdu(&apdu[0], apdu_len, &invoke_id, &rec_data);
	iLen = 0;
	
    /* Error code is returned for read and write operations */

	tag_len =
			decode_tag_number_and_value(&rec_data.serviceParameters[iLen],
			&tag_number, &len_value_type);
	iLen += tag_len;

	if(tag_number == BACNET_APPLICATION_TAG_OCTET_STRING)
	{
		command = rec_data.serviceParameters[iLen + 2];
		private_header.command = command;
		private_header.total_length = rec_data.serviceParameters[iLen + 1] * 256 + rec_data.serviceParameters[iLen];
		private_header.point_start_instance = rec_data.serviceParameters[iLen + 3];
		private_header.point_end_instance = rec_data.serviceParameters[iLen + 4];		
		private_header.entitysize = rec_data.serviceParameters[iLen + 6] * 256	+ rec_data.serviceParameters[iLen + 5];
	}
		
	private_data.vendorID =  rec_data.vendorID;
	private_data.serviceNumber = rec_data.serviceNumber;

	header_len = 7;//USER_DATA_HEADER_LEN;

	if((private_header.command != 94) && (private_header.command != 194))
		return ;
//ok
	if(command > 100)   // write command
	{
		transfer_len = 0;		
		{
			switch(command)
			{
				case 194:
					ptr = (uint8_t *)(&bacnet_to_modbus);		
					break;
				default:
					break;					
			} 
			// write
			if(ptr != NULL)	
			{	 
				if(command == 194)
				{
					//memcpy(ptr,&Temp_CS.value[header_len],private_header.total_length - header_len);
					memcpy(ptr,&rec_data.serviceParameters[iLen + 7],private_header.total_length - header_len);
					// WRITE_BACNET_TO_MDOBUS
					Get_Pkt_Bac_to_Modbus(&private_header);
				}
				else if(private_header.total_length  == private_header.entitysize * (private_header.point_end_instance - private_header.point_start_instance + 1) + header_len)
				{
					// to be add more command 
					// ....
					
				}
			}
		}
	}
	else  // read
	{
		if(private_header.command == 94)
		{
			transfer_len = private_header.entitysize * 2; 
		}
		
		if((transfer_len >= 0) && (transfer_len <= 500 ))
		{
			temp[1] = (uint8_t)(transfer_len >> 8);
			temp[0] = (uint8_t)transfer_len;
			temp[2] = private_header.command;
			temp[3] = private_header.point_start_instance;
			temp[4] = private_header.point_end_instance;
			temp[5] = (uint8_t)private_header.entitysize ; 				
			temp[6] = (uint8_t)(private_header.entitysize >> 8); 
		}		

		switch(command)
		{
			case 94:
			// get packet (transfer bacnet to modbus )	
				Get_Pkt_Bac_to_Modbus(&private_header);
				ptr = (uint8_t *)(&bacnet_to_modbus[0]);			
				break;

			default:
				break;
		}

		if(ptr != NULL)	
		{
			memcpy(&temp[header_len],ptr,transfer_len);
		}
		
		status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,	temp, &data_value);

	} 
#if 1	
  if(status == true)
	{	
	   	memset(temp,0,480);
	    private_data_len = 
	        bacapp_encode_application_data(&temp[0],&data_value);
		
	    private_data.serviceParameters = &temp[0];
	    private_data.serviceParametersLen = private_data_len;
   		len = uptransfer_encode_apdu(&apdu[0], &private_data);
			
	}
	
	//Send_UnconfirmedPrivateTransfer(src,&private_data,protocal);

	{
		int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
//    if (!dcc_communication_enabled())	   tbd:
//        return;
    datalink_get_my_address(&my_address,protocal);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[protocal][0], src, &my_address,
        &npdu_data);

    /* encode the APDU portion of the packet */
    len =
        ptransfer_ack_encode_apdu(&Handler_Transmit_Buffer[protocal][pdu_len],invoke_id,
        &private_data);
    pdu_len += len;
   	datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[protocal][0],pdu_len,protocal);	
		
	}
#endif
  return;

}


int ptransfer_decode_apdu(
    uint8_t * apdu,
    unsigned apdu_len,
    uint8_t * invoke_id,
    BACNET_PRIVATE_TRANSFER_DATA * private_data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -1;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    /* invoke id - filled in by net layer */
    *invoke_id = apdu[2];
    if (apdu[3] != SERVICE_CONFIRMED_PRIVATE_TRANSFER)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len =
            ptransfer_decode_service_request(&apdu[offset], apdu_len - offset,
            private_data);
    }

    return len;
}

int uptransfer_decode_apdu(
    uint8_t * apdu,
    unsigned apdu_len,
    BACNET_PRIVATE_TRANSFER_DATA * private_data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu) {
        return -1;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) {
        return -1;
    }
    if (apdu[1] != SERVICE_UNCONFIRMED_PRIVATE_TRANSFER) {
        return -1;
    }
    offset = 2;
    if (apdu_len > offset) {
        len =
		  ptransfer_decode_service_request(&apdu[offset], apdu_len - offset,
            private_data);
    }
    return len;
}





/* encode service */
static int pt_encode_apdu(
    uint8_t * apdu,
    uint16_t max_apdu,
    BACNET_PRIVATE_TRANSFER_DATA * private_data)
{
    int len = 0;        /* length of each encoding */
    int apdu_len = 0;   /* total length of the apdu, return value */
/*
        Unconfirmed/ConfirmedPrivateTransfer-Request ::= SEQUENCE {
        vendorID               [0] Unsigned,
        serviceNumber          [1] Unsigned,
        serviceParameters      [2] ABSTRACT-SYNTAX.&Type OPTIONAL
    }
*/
    /* unused parameter */
    max_apdu = max_apdu;
    if (apdu) {
        len =
            encode_context_unsigned(&apdu[apdu_len], 0,
            private_data->vendorID);
        apdu_len += len;
        len =
            encode_context_unsigned(&apdu[apdu_len], 1,
            private_data->serviceNumber);
        apdu_len += len;
        len = encode_opening_tag(&apdu[apdu_len], 2);
        apdu_len += len;
        for (len = 0; len < private_data->serviceParametersLen; len++) {
            apdu[apdu_len] = private_data->serviceParameters[len];
            apdu_len++;
        }
        len = encode_closing_tag(&apdu[apdu_len], 2);
        apdu_len += len;
    }

    return apdu_len;
}

int ptransfer_encode_apdu(
    uint8_t * apdu,
    uint8_t invoke_id,
    BACNET_PRIVATE_TRANSFER_DATA * private_data)
{
    int apdu_len = 0;   /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_PRIVATE_TRANSFER;
        apdu_len = 4;
        len =
            pt_encode_apdu(&apdu[apdu_len], (uint16_t) (MAX_APDU - apdu_len),
            private_data);
        apdu_len += len;
    }

    return apdu_len;
}

int uptransfer_encode_apdu(
    uint8_t * apdu,
    BACNET_PRIVATE_TRANSFER_DATA * private_data)
{
    int apdu_len = 0;   /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_PRIVATE_TRANSFER;
        apdu_len = 2;
        len =
            pt_encode_apdu(&apdu[apdu_len], (uint16_t) (MAX_APDU - apdu_len),
            private_data);
        apdu_len += len;
    }

    return apdu_len;
}

/* decode the service request only */
int ptransfer_decode_service_request(
    uint8_t * apdu,
    unsigned apdu_len,
    BACNET_PRIVATE_TRANSFER_DATA * private_data)
{
    int len = 0;        /* return value */
    int decode_len = 0; /* return value */
    uint32_t unsigned_value = 0;

    /* check for value pointers */
    if (apdu_len && private_data) {
        /* Tag 0: vendorID */
        decode_len = decode_context_unsigned(&apdu[len], 0, &unsigned_value);
        if (decode_len < 0) {
            return -1;
        }
        len = decode_len;
        private_data->vendorID = (uint16_t) unsigned_value;
        /* Tag 1: serviceNumber */
        decode_len = decode_context_unsigned(&apdu[len], 1, &unsigned_value);
        if (decode_len < 0) {
            return -1;
        }
        len += decode_len;
        private_data->serviceNumber = unsigned_value;
        /* Tag 2: serviceParameters */
        if (decode_is_opening_tag_number(&apdu[len], 2)) {
            /* a tag number of 2 is not extended so only one octet */
            len++;
            /* don't decode the serviceParameters here */
            private_data->serviceParameters = &apdu[len];
            private_data->serviceParametersLen =
                (int) apdu_len - len - 1 /*closing tag */ ;
            /* len includes the data and the closing tag */
            len = (int) apdu_len;
        } else {
            return -1;
        }
    }

    return len;
}

int ptransfer_error_encode_apdu(
    uint8_t * apdu,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code,
    BACNET_PRIVATE_TRANSFER_DATA * private_data)
{
    int apdu_len = 0;   /* total length of the apdu, return value */
    int len = 0;        /* length of the part of the encoding */

    if (apdu) {
        apdu[0] = PDU_TYPE_ERROR;
        apdu[1] = invoke_id;
        apdu[2] = SERVICE_CONFIRMED_PRIVATE_TRANSFER;
        apdu_len = 3;
        /* service parameters */
/*
        ConfirmedPrivateTransfer-Error ::= SEQUENCE {
        errorType       [0] Error,
        vendorID        [1] Unsigned,
        serviceNumber [2] Unsigned,
        errorParameters [3] ABSTRACT-SYNTAX.&Type OPTIONAL
    }
*/
        len = encode_opening_tag(&apdu[apdu_len], 0);
        apdu_len += len;
        len = encode_application_enumerated(&apdu[apdu_len], error_class);
        apdu_len += len;
        len = encode_application_enumerated(&apdu[apdu_len], error_code);
        apdu_len += len;
        len = encode_closing_tag(&apdu[apdu_len], 0);
        apdu_len += len;
        len =
            encode_context_unsigned(&apdu[apdu_len], 1,
            private_data->vendorID);
        apdu_len += len;
        len =
            encode_context_unsigned(&apdu[apdu_len], 2,
            private_data->serviceNumber);
        apdu_len += len;
        len = encode_opening_tag(&apdu[apdu_len], 3);
        apdu_len += len;
        for (len = 0; len < private_data->serviceParametersLen; len++) {
            apdu[apdu_len] = private_data->serviceParameters[len];
            apdu_len++;
        }
        len = encode_closing_tag(&apdu[apdu_len], 3);
        apdu_len += len;
    }

    return apdu_len;
}

/* decode the service request only */
int ptransfer_error_decode_service_request(
    uint8_t * apdu,
    unsigned apdu_len,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code,
    BACNET_PRIVATE_TRANSFER_DATA * private_data)
{
    int len = 0;        /* return value */
    int decode_len = 0; /* return value */
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint32_t unsigned_value = 0;

    /* check for value pointers */
    if (apdu_len && private_data) {
        /* Tag 0: Error */
        if (decode_is_opening_tag_number(&apdu[len], 0)) {
            /* a tag number of 0 is not extended so only one octet */
            len++;
            /* error class */
            decode_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += decode_len;
            if (tag_number != BACNET_APPLICATION_TAG_ENUMERATED) {
                return 0;
            }
            decode_len =
                decode_enumerated(&apdu[len], len_value_type, &unsigned_value);
            len += decode_len;
            if (error_class) {
                *error_class = (BACNET_ERROR_CLASS) unsigned_value;
            }
            /* error code */
            decode_len =
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value_type);
            len += decode_len;
            if (tag_number != BACNET_APPLICATION_TAG_ENUMERATED) {
                return 0;
            }
            decode_len =
                decode_enumerated(&apdu[len], len_value_type, &unsigned_value);
            len += decode_len;
            if (error_code) {
                *error_code = (BACNET_ERROR_CODE) unsigned_value;
            }
            if (decode_is_closing_tag_number(&apdu[len], 0)) {
                /* a tag number of 0 is not extended so only one octet */
                len++;
            } else {
                return 0;
            }
        }
        /* Tag 1: vendorID */
        decode_len = decode_context_unsigned(&apdu[len], 1, &unsigned_value);
        if (decode_len < 0) {
            return -1;
        }
        len += decode_len;
        private_data->vendorID = (uint16_t) unsigned_value;
        /* Tag 2: serviceNumber */
        decode_len = decode_context_unsigned(&apdu[len], 2, &unsigned_value);
        if (decode_len < 0) {
            return -1;
        }
        len += decode_len;
        private_data->serviceNumber = unsigned_value;
        /* Tag 3: serviceParameters */
        if (decode_is_opening_tag_number(&apdu[len], 3)) {
            /* a tag number of 2 is not extended so only one octet */
            len++;
            /* don't decode the serviceParameters here */
            private_data->serviceParameters = &apdu[len];
            private_data->serviceParametersLen =
                (int) apdu_len - len - 1 /*closing tag */ ;
        } else {
            return -1;
        }
        /* we could check for a closing tag of 3 */
    }

    return len;
}

int ptransfer_ack_encode_apdu(
    uint8_t * apdu,
    uint8_t invoke_id,
    BACNET_PRIVATE_TRANSFER_DATA * private_data)
{
    int len = 0;        /* length of each encoding */
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id;    /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_PRIVATE_TRANSFER;   /* service choice */
        apdu_len = 3;
        /* service ack follows */
/*
        ConfirmedPrivateTransfer-ACK ::= SEQUENCE {
        vendorID               [0] Unsigned,
        serviceNumber          [1] Unsigned,
        resultBlock            [2] ABSTRACT-SYNTAX.&Type OPTIONAL
    }
*/
				
				 len =
            encode_context_unsigned(&apdu[apdu_len], 0,
            private_data->vendorID);
        apdu_len += len;
			
        len =
            encode_context_unsigned(&apdu[apdu_len], 1,
            private_data->serviceNumber);
        apdu_len += len;
				
        len = encode_opening_tag(&apdu[apdu_len], 2);
        apdu_len += len;
        for (len = 0; len < private_data->serviceParametersLen; len++) {
            apdu[apdu_len] = private_data->serviceParameters[len];
            apdu_len++;
        }
				
        len = encode_closing_tag(&apdu[apdu_len], 2);
        apdu_len += len;
				
    }

    return apdu_len;
}

/* ptransfer_ack_decode_service_request() is the same as
       ptransfer_decode_service_request */
#endif

