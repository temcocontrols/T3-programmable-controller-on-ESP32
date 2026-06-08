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

#include <stdint.h>     /* for standard integer types uint8_t etc. */
#include <stdbool.h>    /* for the standard bool type. */
#include "bacdcode.h"
#include "bacint.h"
#include "bacnet.h"
#include "bip.h"
#include "bvlc.h"
//#include "uip.h"
//#include "tcpip.h"
#include "net.h"        /* custom per port */	  
#include "types.h"
#include <string.h>
//#if PRINT_ENABLED
#include <stdio.h>      /* for standard i/o, like printing */
//#endif
#include "datalink.h"
#include "handlers.h"



/* NAMING CONSTANT DECLARATIONS */
#define BIP_MAX_CONNS			4
#define BIP_NO_NEW_CONN			0xFF

#define BIP_STATE_FREE			0
#define	BIP_STATE_WAIT			1
#define	BIP_STATE_CONNECTED		2


//int socklen_t;

//extern xSemaphoreHandle sembip;
//xQueueHandle qBip;

/** @file bip.c  Configuration and Operations for BACnet/IP */

static int BIP_Socket = -1;
/* port to use - stored in network byte order */
static uint16_t BIP_Port = 0;   /* this will force initialization in demos */
/* IP Address - stored in network byte order */
static struct in_addr BIP_Address;
/* Broadcast Address - stored in network byte order */
static struct in_addr BIP_Broadcast_Address;

/** Setter for the BACnet/IP socket handle.
 *
 * @param sock_fd [in] Handle for the BACnet/IP socket.
 */
extern uint8_t * bip_Data;
extern U8_T Send_bip_address[6];
//extern uint16_t  bip_len;
uint16_t Get_bip_len(void);
void bip_set_socket(
    int sock_fd)
{
    BIP_Socket = sock_fd;
}

void bip_set_port( uint16_t port)
{
	BIP_Port = port;
}

void bip_set_broadcast_addr( uint32_t net_address)
{       /* in network byte order */
    BIP_Broadcast_Address.s_addr = net_address;
}

void Set_broadcast_bip_address(uint32_t net_address)
{
	BIP_Port = 47808;
	Send_bip_address[0] = net_address/*BIP_Broadcast_Address.s_addr*/ >> 24;
	Send_bip_address[1] = net_address/*BIP_Broadcast_Address.s_addr*/ >> 16;
	Send_bip_address[2] = net_address/*BIP_Broadcast_Address.s_addr*/ >> 8;
	Send_bip_address[3] = net_address/*BIP_Broadcast_Address.s_addr*/;
	Send_bip_address[4] = BIP_Port >> 8;
	Send_bip_address[5] = BIP_Port;

}

uint8_t bip_send_buf[MAX_MPDU_IP] = { 0 };
int bip_send_len = 0;
static int bip_decode_bip_address(
    BACNET_ADDRESS * bac_addr,
    struct in_addr *address,    /* in network format */
    uint16_t * port)
{       /* in network format */
    int len = 0;

    if (bac_addr) {
        memcpy(&address->s_addr, &bac_addr->mac[0], 4);
        memcpy(port, &bac_addr->mac[4], 2);
        len = 6;
    }

    return len;
}

int bip_send_pdu(
    BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,      /* any data to be sent - may be null */
    unsigned pdu_len,uint8_t protocal
	)
{       /* number of bytes of data */  
    struct sockaddr_in bip_dest;
    
    int bytes_sent = 0;
    /* addr and port in host format */
    struct in_addr address;
    uint16_t port = 0;
	//	uint8_t far mtu[MAX_MPDU_IP] = { 0 };
 //   (void) npdu_data;
    /* assumes that the driver has already been initialized */

	/*if (BIP_Socket < 0)
	{
		return 0;//BIP_Socket;
	} */

    bip_send_buf[0] = BVLL_TYPE_BACNET_IP;
    bip_dest.sin_family = AF_INET;
    if ((dest->net == BACNET_BROADCAST_NETWORK) || ((dest->net > 0) &&
			(dest->len == 0)) || (dest->mac_len == 0)) {
        /* broadcast */
				
        address.s_addr = BIP_Broadcast_Address.s_addr;
        port = BIP_Port;
				bip_send_buf[1] = BVLC_ORIGINAL_BROADCAST_NPDU;
    } else if (dest->mac_len == 6) {
        bip_decode_bip_address(dest, &address, &port);
        bip_send_buf[1] = BVLC_ORIGINAL_UNICAST_NPDU;
    } else {

        return -1;

		// if GSM, do not return
    }
    bip_dest.sin_addr.s_addr = address.s_addr;
    bip_dest.sin_port = port;
    memset(&(bip_dest.sin_zero), '\0', 8);
    bip_send_len = 2;
    bip_send_len += encode_unsigned16(&bip_send_buf[bip_send_len],
        (uint16_t) (pdu_len + 4 /*inclusive */ ));
    memcpy(&bip_send_buf[bip_send_len], pdu, pdu_len);
    bip_send_len += pdu_len;

	/*if(protocal == BAC_IP)
	{
		uip_send((char *)mtu, mtu_len);		
	}*/
	

	bytes_sent = bip_send_len;
	//memset(mtu,0,MAX_MPDU_IP);
	//mtu_len = 0;
	return bytes_sent;	 
}



struct uip_udp_conn * bip_send_client_conn;
uint8_t bip_client_send_buf[MAX_MPDU_IP] = { 0 };
int bip_client_send_len = 0;
// for network points
int bip_send_pdu_client(
    BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,      /* any data to be sent - may be null */
    unsigned pdu_len,uint8_t protocal
	)
{       /* number of bytes of data */  

    struct sockaddr_in bip_dest;
    int mtu_len = 0;
    int bytes_sent = 0;
    /* addr and port in host format */
    struct in_addr address;
    uint16_t port = 0;
		uint8_t far mtu1[MAX_MPDU_IP] = { 0 };
 //   (void) npdu_data;
    /* assumes that the driver has already been initialized */

    mtu1[0] = BVLL_TYPE_BACNET_IP;
    bip_dest.sin_family = AF_INET;
    if ((dest->net == BACNET_BROADCAST_NETWORK) || ((dest->net > 0) &&
			(dest->len == 0)) || (dest->mac_len == 0)) {
        /* broadcast */
				
        address.s_addr = BIP_Broadcast_Address.s_addr;
        port = BIP_Port;				
				mtu1[1] = BVLC_ORIGINAL_BROADCAST_NPDU;
    } else if (dest->mac_len == 6) {
        bip_decode_bip_address(dest, &address, &port);
        mtu1[1] = BVLC_ORIGINAL_UNICAST_NPDU;
    } else {
        return -1;

    }
		
    bip_dest.sin_addr.s_addr = address.s_addr;
    bip_dest.sin_port = port;

    memset(&(bip_dest.sin_zero), '\0', 8);
    mtu_len = 2;
    mtu_len += encode_unsigned16(&mtu1[mtu_len],
        (uint16_t) (pdu_len + 4 /*inclusive */ ));
    memcpy(&mtu1[mtu_len], pdu, pdu_len);
    mtu_len += pdu_len;

    memcpy(bip_client_send_buf, mtu1, mtu_len);
    bip_client_send_len = mtu_len;
	bytes_sent = mtu_len;
	memset(mtu1,0,MAX_MPDU_IP);
	mtu_len = 0;

	return bytes_sent;

}
#if 0
// for COV
struct uip_udp_conn * bip_send_client_conn2;
int bip_send_pdu_client2(
    BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,      /* any data to be sent - may be null */
    unsigned pdu_len,uint8_t protocal
	)
{       /* number of bytes of data */  

    struct sockaddr_in bip_dest;
    int mtu_len = 0;
    int bytes_sent = 0;
    /* addr and port in host format */
    struct in_addr address;
    uint16_t port = 0;
		uint8_t far mtu1[MAX_MPDU_IP] = { 0 };
 //   (void) npdu_data;
    /* assumes that the driver has already been initialized */


    mtu1[0] = BVLL_TYPE_BACNET_IP;
    bip_dest.sin_family = AF_INET;
    if ((dest->net == BACNET_BROADCAST_NETWORK) || ((dest->net > 0) &&
			(dest->len == 0)) || (dest->mac_len == 0)) {
        /* broadcast */
				
        address.s_addr = BIP_Broadcast_Address.s_addr;
        port = BIP_Port;
        mtu1[1] = BVLC_ORIGINAL_BROADCAST_NPDU;
    } else if (dest->mac_len == 6) {
        bip_decode_bip_address(dest, &address, &port);
        mtu1[1] = BVLC_ORIGINAL_UNICAST_NPDU;
    } else {
        return -1;

    }
		
    bip_dest.sin_addr.s_addr = address.s_addr;
    bip_dest.sin_port = port;
		
    memset(&(bip_dest.sin_zero), '\0', 8);
    mtu_len = 2;
    mtu_len += encode_unsigned16(&mtu1[mtu_len],
        (uint16_t) (pdu_len + 4 /*inclusive */ ));
    memcpy(&mtu1[mtu_len], pdu, pdu_len);
    mtu_len += pdu_len;
		if(Send_bip_Flag2)
		{
			uip_ipaddr_t addr;
			if(bip_send_client_conn2 != NULL) 
			{		
				uip_udp_remove(bip_send_client_conn2);
			}
			Send_bip_count2 = MAX_RETRY_SEND_BIP;
			uip_ipaddr(addr,Send_bip_address2[0],Send_bip_address2[1],Send_bip_address2[2],Send_bip_address2[3]);	
			bip_send_client_conn2 = uip_udp_new(&addr, HTONS(Send_bip_address2[4] * 256 + Send_bip_address2[5])); // des port
			if(bip_send_client_conn2 != NULL) 
			{ 
				uip_udp_bind(bip_send_client_conn2,HTONS(UDP_BACNET_LPORT));  // src port	
			}
			memcpy(bip_bac_buf2.buf,&mtu1, mtu_len);			
			bip_bac_buf2.len = mtu_len;
			
		}
		else
			uip_send((char *)mtu, mtu_len);
		
	bytes_sent = mtu_len;
	memset(mtu1,0,MAX_MPDU_IP);
	mtu_len = 0;
	return bytes_sent;

}
#endif 


/** Implementation of the receive() function for BACnet/IP; receives one
 * packet, verifies its BVLC header, and removes the BVLC header from
 * the PDU data before returning.
 *
 * @param src [out] Source of the packet - who should receive any response.
 * @param pdu [out] A buffer to hold the PDU portion of the received packet,
 * 					after the BVLC portion has been stripped off.
 * @param max_pdu [in] Size of the pdu[] buffer.
 * @param timeout [in] The number of milliseconds to wait for a packet.
 * @return The number of octets (remaining) in the PDU, or zero on failure.
 */

extern U8_T BIP_src_addr[6];

uint16_t bip_receive(
    BACNET_ADDRESS * src,       /* source address */
    uint8_t * pdu,      /* PDU data */
    uint16_t max_pdu,   /* amount of space available in the PDU  */
    unsigned protocal)
{
    int received_bytes = 0;
    uint16_t pdu_len = 0;       /* return value */
//    fd_set read_fds;
 //   int max = 0;
 //   struct timeval select_timeout;
    struct sockaddr_in sin = {0};
	struct sockaddr_in original_sin = {0};
	struct sockaddr_in dest = {0};
  //  socklen_t sin_len = sizeof(sin);
	uint16_t npdu_len = 0;      /* return value */
    uint16_t i = 0;
    int function = 0;
	uint16_t time_to_live = 0;
	uint16_t result_code = 0;
	bool status = false;
    /* Make sure the socket is open */

	/*if (BIP_Socket < 0)
	{		
     return 0;
	}*/

	received_bytes = Get_bip_len();
	bip_Data = pdu;
	
    /* See if there is a problem */
	if((protocal == BAC_IP) )
	{
	    if (received_bytes <= 0) 
		{	
			return 0;
		}
	}
	received_bytes = 0;
  /* the signature of a BACnet/IP packet */

		if(pdu[0] != BVLL_TYPE_BACNET_IP) 
		{ 	
			return 0;
		}
		function = pdu[1]; 
    /* decode the length of the PDU - length is inclusive of BVLC */
    (void) decode_unsigned16(&pdu[2], &pdu_len);
		// get sin address, added by chelsea
		//????????????????
		sin.sin_addr.s_addr = 0;
		sin.sin_port = 0;
		original_sin.sin_addr.s_addr = 0;
		original_sin.sin_port = 0;
		dest.sin_addr.s_addr = 0;
		dest.sin_port = 0;
		
		// changed by chelsea
		// pdu[0]. ... 81 0b 00 18 01 20 ff ff ff 00 10 00, cant get sin.sin_addr

		if(protocal == BAC_IP_CLIENT)
		{
			memcpy(&sin.sin_addr.s_addr,BIP_src_addr,4);
			memcpy(&sin.sin_port,&BIP_src_addr[4],2);
		}


    /* subtract off the BVLC header */
		pdu_len -= 4;
		switch (function) {
#if 0
        case BVLC_RESULT:
            /* Upon receipt of a BVLC-Result message containing a result code
               of X'0000' indicating the successful completion of the
               registration, a foreign device shall start a timer with a value
               equal to the Time-to-Live parameter of the preceding Register-
               Foreign-Device message. At the expiration of the timer, the
               foreign device shall re-register with the BBMD by sending a BVLL
               Register-Foreign-Device message */
            /* Clients can now get this result */

            (void) decode_unsigned16(&pdu[4], &result_code);
            BVLC_Result_Code = (BACNET_BVLC_RESULT) result_code;
//            debug_printf("BVLC: Result Code=%d\n", BVLC_Result_Code);
            /* not an NPDU */
            npdu_len = 0;

            break;
        case BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE:
//            debug_printf("BVLC: Received Write-BDT.\n");
            /* Upon receipt of a BVLL Write-Broadcast-Distribution-Table
               message, a BBMD shall attempt to create or replace its BDT,
               depending on whether or not a BDT has previously existed.
               If the creation or replacement of the BDT is successful, the BBMD
               shall return a BVLC-Result message to the originating device with
               a result code of X'0000'. Otherwise, the BBMD shall return a
               BVLC-Result message to the originating device with a result code
               of X'0010' indicating that the write attempt has failed. */

						if(bbmd_en == 0)
						{
							 pdu_len = 0;
							break;
						}	

            status = bvlc_create_bdt(&pdu[4], pdu_len);
            if (status) {
                bvlc_send_result(&sin, BVLC_RESULT_SUCCESSFUL_COMPLETION);
            } else {
                bvlc_send_result(&sin,
                    BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK);
            }
            /* not an NPDU */
            pdu_len = 0;
            break;
        case BVLC_READ_BROADCAST_DIST_TABLE:
//            debug_printf("BVLC: Received Read-BDT.\n");
            /* Upon receipt of a BVLL Read-Broadcast-Distribution-Table
               message, a BBMD shall load the contents of its BDT into a BVLL
               Read-Broadcast-Distribution-Table-Ack message and send it to the
               originating device. If the BBMD is unable to perform the
               read of its BDT, it shall return a BVLC-Result message to the
               originating device with a result code of X'0020' indicating that
               the read attempt has failed. */

						if(bbmd_en == 0)
						{
							 pdu_len = 0;
							break;
						}	

            if (bvlc_send_bdt(&sin) <= 0) {
                bvlc_send_result(&sin,
                    BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK);
            }
            /* not an NPDU */
            pdu_len = 0;
            break;
//        case BVLC_READ_BROADCAST_DIST_TABLE_ACK:
////            debug_printf("BVLC: Received Read-BDT-Ack.\n");
//            /* FIXME: complete the code for client side read */
//            /* not an NPDU */
//            pdu_len = 0;
//            break;

        case BVLC_FORWARDED_NPDU:
            /* Upon receipt of a BVLL Forwarded-NPDU message, a BBMD shall
               process it according to whether it was received from a peer
               BBMD as the result of a directed broadcast or a unicast
               transmission. A BBMD may ascertain the method by which Forwarded-
               NPDU messages will arrive by inspecting the broadcast distribution
               mask field in its own BDT entry since all BDTs are required
               to be identical. If the message arrived via directed broadcast,
               it was also received by the other devices on the BBMD's subnet. In
               this case the BBMD merely retransmits the message directly to each
               foreign device currently in the BBMD's FDT. If the
               message arrived via a unicast transmission it has not yet been
               received by the other devices on the BBMD's subnet. In this case,
               the message is sent to the devices on the BBMD's subnet using the
               B/IP broadcast address as well as to each foreign device
               currently in the BBMD's FDT. A BBMD on a subnet with no other
               BACnet devices may omit the broadcast using the B/IP
               broadcast address. The method by which a BBMD determines whether
               or not other BACnet devices are present is a local matter. */
            /* decode the 4 byte original address and 2 byte port */

			if(bbmd_en == 0)
			{
				 pdu_len = 0;
				break;
			}	

            bvlc_decode_bip_address(&pdu[4], &original_sin.sin_addr,
                &original_sin.sin_port);
            pdu_len -= 6;
            /*  Broadcast locally if received via unicast from a BDT member */
            if (bvlc_bdt_member_mask_is_unicast(&sin)) {
                dest.sin_addr.s_addr = bip_get_broadcast_addr();
                dest.sin_port = bip_get_port();		
                bvlc_send_mpdu(&dest, &pdu[4 + 6], pdu_len);
            }
            /* use the original addr from the BVLC for src */
            dest.sin_addr.s_addr = original_sin.sin_addr.s_addr;
            dest.sin_port = original_sin.sin_port;
            bvlc_fdt_forward_npdu(&dest, &pdu[4 + 6], pdu_len);
//            debug_printf("BVLC: Received Forwarded-NPDU from %s:%04X.\n",
 //               inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
            bvlc_internet_to_bacnet_address(src, &dest);
            if (pdu_len < max_pdu) {
                /* shift the buffer to return a valid PDU */
                for (i = 0; i < pdu_len; i++) {
                    pdu[i] = pdu[4 + 6 + i];
                }
            } else {
                /* ignore packets that are too large */
                /* clients should check my max-apdu first */
                pdu_len = 0;
            }
            break;

        case BVLC_REGISTER_FOREIGN_DEVICE:
            /* Upon receipt of a BVLL Register-Foreign-Device message, a BBMD
               shall start a timer with a value equal to the Time-to-Live
               parameter supplied plus a fixed grace period of 30 seconds. If,
               within the period during which the timer is active, another BVLL
               Register-Foreign-Device message from the same device is received,
               the timer shall be reset and restarted. If the time expires
               without the receipt of another BVLL Register-Foreign-Device
               message from the same foreign device, the FDT entry for this
               device shall be cleared. */

				if(bbmd_en == 0)
				{
					 pdu_len = 0;
					 break;
				}	
				
            (void) decode_unsigned16(&pdu[4], &time_to_live);
						
            if (bvlc_register_foreign_device(&sin, time_to_live)) {
							
                bvlc_send_result(&sin, BVLC_RESULT_SUCCESSFUL_COMPLETION);
//                debug_printf("BVLC: Registered a Foreign Device.\n");
            } else {
                bvlc_send_result(&sin,
                    BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK);
//                debug_printf("BVLC: Failed to Register a Foreign Device.\n");
            }
            /* not an NPDU */
            pdu_len = 0;
            break;
        case BVLC_READ_FOREIGN_DEVICE_TABLE:
//            debug_printf("BVLC: Received Read-FDT.\n");
            /* Upon receipt of a BVLL Read-Foreign-Device-Table message, a
               BBMD shall load the contents of its FDT into a BVLL Read-
               Foreign-Device-Table-Ack message and send it to the originating
               device. If the BBMD is unable to perform the read of its FDT,
               it shall return a BVLC-Result message to the originating device
               with a result code of X'0040' indicating that the read attempt has
               failed. */

			if(bbmd_en == 0)
			{
				 pdu_len = 0;
				 break;
			}	
							
            if (bvlc_send_fdt(&sin) <= 0) {
                bvlc_send_result(&sin,
                    BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK);
            }
            /* not an NPDU */
            pdu_len = 0;
            break;

        case BVLC_READ_FOREIGN_DEVICE_TABLE_ACK:
//            debug_printf("BVLC: Received Read-FDT-Ack.\n");
            /* FIXME: complete the code for client side read */
            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY:
//            debug_printf("BVLC: Received Delete-FDT-Entry.\n");
            /* Upon receipt of a BVLL Delete-Foreign-Device-Table-Entry
               message, a BBMD shall search its foreign device table for an entry
               corresponding to the B/IP address supplied in the message. If an
               entry is found, it shall be deleted and the BBMD shall return a
               BVLC-Result message to the originating device with a result code
               of X'0000'. Otherwise, the BBMD shall return a BVLCResult
               message to the originating device with a result code of X'0050'
               indicating that the deletion attempt has failed. */
            if (bvlc_delete_foreign_device(&pdu[4])) {
                bvlc_send_result(&sin, BVLC_RESULT_SUCCESSFUL_COMPLETION);
            } else {
                bvlc_send_result(&sin,
                    BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK);
            }
            /* not an NPDU */
            npdu_len = 0;
            break;

        case BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK:
//          debug_printf
//               ("BVLC: Received Distribute-Broadcast-to-Network from %s:%04X.\n",
//                inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
            /* Upon receipt of a BVLL Distribute-Broadcast-To-Network message
               from a foreign device, the receiving BBMD shall transmit a
               BVLL Forwarded-NPDU message on its local IP subnet using the
               local B/IP broadcast address as the destination address. In
               addition, a Forwarded-NPDU message shall be sent to each entry
               in its BDT as described in the case of the receipt of a
               BVLL Original-Broadcast-NPDU as well as directly to each foreign
               device currently in the BBMD's FDT except the originating
               node. If the BBMD is unable to perform the forwarding function,
               it shall return a BVLC-Result message to the foreign device
               with a result code of X'0060' indicating that the forwarding
               attempt was unsuccessful */

						
            bvlc_forward_npdu(&sin, &pdu[4], pdu_len);
            bvlc_bdt_forward_npdu(&sin, &pdu[4], pdu_len);
            bvlc_fdt_forward_npdu(&sin, &pdu[4], pdu_len);
            /* not an NPDU */
            pdu_len = 0;
            break;
#endif
        case BVLC_ORIGINAL_UNICAST_NPDU:
//            debug_printf("BVLC: Received Original-Unicast-NPDU.\n");
            /* ignore messages from me */
/*
            if ((sin.sin_addr.s_addr == bip_get_addr()) &&
                (sin.sin_port == bip_get_port())) {
                pdu_len = 0;
            } else
*/
				{	
                bvlc_internet_to_bacnet_address(src, &sin);
                if (pdu_len < max_pdu) { 
                    /* shift the buffer to return a valid PDU */
                    for (i = 0; i < pdu_len + 4; i++) { 
											// FIXED by chelsea, + 4
                        pdu[i] = pdu[4 + i];
                    }
                } else { 
                    /* ignore packets that are too large */
                    /* clients should check my max-apdu first */
                    pdu_len = 0;
                }
            }
            break;
        case BVLC_ORIGINAL_BROADCAST_NPDU:
//            debug_printf("BVLC: Received Original-Broadcast-NPDU.\n");
            /* Upon receipt of a BVLL Original-Broadcast-NPDU message,
               a BBMD shall construct a BVLL Forwarded-NPDU message and
               send it to each IP subnet in its BDT with the exception
               of its own. The B/IP address to which the Forwarded-NPDU
               message is sent is formed by inverting the broadcast
               distribution mask in the BDT entry and logically ORing it
               with the BBMD address of the same entry. This process
               produces either the directed broadcast address of the remote
               subnet or the unicast address of the BBMD on that subnet
               depending on the contents of the broadcast distribution
               mask. See J.4.3.2.. In addition, the received BACnet NPDU
               shall be sent directly to each foreign device currently in
               the BBMD's FDT also using the BVLL Forwarded-NPDU message. */
            bvlc_internet_to_bacnet_address(src, &sin);
            if (pdu_len < max_pdu) { 
                /* shift the buffer to return a valid PDU */
                for (i = 0; i < pdu_len; i++) {
                    pdu[i] = pdu[4 + i];
                }
                /* if BDT or FDT entries exist, Forward the NPDU */
//								if(Modbus.bbmd_en)
//								{
//									bvlc_bdt_forward_npdu(&sin, &pdu[0], pdu_len);
//									bvlc_fdt_forward_npdu(&sin, &pdu[0], pdu_len);
//								}
            } else { 
                /* ignore packets that are too large */
                pdu_len = 0;
            }
            break;
        default:
            break;
    }
    return pdu_len;
}

#if 1
extern Byte	Station_NUM;

uint8_t flag_response_iam = 0;

uint32_t get_ip_addr();

void bip_get_my_address(
    BACNET_ADDRESS * my_address)
{
    int i = 0;
	uint32_t ip;
	uint16_t port;
	
	ip = get_ip_addr();
	
    if (my_address) {
        my_address->mac_len = 6;
        memcpy(&my_address->mac[0], &ip, 4);
        memcpy(&my_address->mac[4], &BIP_Socket, 2);
	
        my_address->net = get_network_number()/*Modbus.network_number*/;    /* local only, no routing */
		if(get_network_number() == 0xffff || get_network_number() == 0)
		{
			my_address->len = 0;
		}
		else 
		{
			//if(flag_response_iam == 1) // added by chelsea
			//	my_address->len = 0; 
			//else
				my_address->len = 6;    /* no SLEN */
			
			memcpy(&my_address->adr[0], &ip, 4);
			memcpy(&my_address->adr[4], &BIP_Socket, 2);
			
		}
    }

    return;
}

void bip_get_broadcast_address(
    BACNET_ADDRESS * dest)
{       /* destination address */
    int i = 0;  /* counter */
    if (dest) {
        dest->mac_len = 6;
        memcpy(&dest->mac[0], &BIP_Broadcast_Address.s_addr, 4);
        memcpy(&dest->mac[4], &BIP_Port, 2);
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0;  /* no SLEN */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            /* no SADR */
            dest->adr[i] = 0;
        }
				
    }

    return;
}


#endif
