/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg

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
#include "datalink.h"
#include "bacnet.h"
#include "net.h"

#include <string.h>

/** @file datalink.c  Optional run-time assignment of datalink transport */
#if 0
//#if defined(BACDL_ALL) || defined FOR_DOXYGEN
/* Function pointers - point to your datalink */
static int BIP_Socket = -1;
/* port to use - stored in network byte order */
static uint16_t BIP_Port = 0;   /* this will force initialization in demos */
/* IP Address - stored in network byte order */
static struct in_addr BIP_Address;
/* Broadcast Address - stored in network byte order */
static struct in_addr BIP_Broadcast_Address;
/** Function template to Initialize the DataLink services at the given interface.
 * @ingroup DLTemplates
 * 
 * @note For Linux, ifname is eth0, ath0, arc0, ttyS0, and others.
         For Windows, ifname is the COM port or dotted ip address of the interface.
         
 * @param ifname [in] The named interface to use for the network layer.
 * @return True if the interface is successfully initialized,
 *         else False if the initialization fails.
 */
	
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

void bip_get_my_address(
    BACNET_ADDRESS * my_address)
{
    int i = 0;

    if (my_address) {
        my_address->mac_len = 6;
        memcpy(&my_address->mac[0], &BIP_Address.s_addr, 4);
        memcpy(&my_address->mac[4], &BIP_Port, 2);

        my_address->net = 0;    /* local only, no routing */
        my_address->len = 0;    /* no SLEN */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            /* no SADR */
            my_address->adr[i] = 0;
        }
    }

    return;
}
#endif



	
int datalink_send_pdu (
BACNET_ADDRESS * dest,
BACNET_NPDU_DATA * npdu_data,
uint8_t * pdu, 
unsigned pdu_len,
uint8_t protocal)
{
	uint16_t ret = 0;

	if(protocal == BAC_MSTP)
	{
		ret = dlmstp_send_pdu(dest,npdu_data,pdu,pdu_len,0);
	}
	
	else if(protocal == BAC_IP)
	{
		ret = bip_send_pdu(dest,npdu_data,pdu,pdu_len,protocal);  // serves
	}
//	else if(protocal == BAC_BVLC)
//	{		
//		ret = bvlc_send_pdu(dest,npdu_data,pdu,pdu_len);
//	}
#if 1//BAC_MASTER
	else if(protocal == BAC_IP_CLIENT)
	{
		ret = bip_send_pdu_client(dest,npdu_data,pdu,pdu_len,protocal);  // client
	}
	else if(protocal == BAC_IP_CLIENT2)
	{
		//ret = bip_send_pdu_client2(dest,npdu_data,pdu,pdu_len,protocal);  // client
	}
#endif

	
	return ret;
}

uint16_t datalink_receive (BACNET_ADDRESS * src, uint8_t * pdu,
    uint16_t max_pdu, unsigned timeout,uint8_t protocal)
{
	uint16_t ret = 0;
	
	if(protocal == BAC_MSTP)
	{
		ret = dlmstp_receive(src,pdu,max_pdu,0);
	}
	else if(protocal == BAC_IP || protocal == BAC_IP_CLIENT)
	{
		ret = bip_receive(src,pdu,max_pdu,protocal);
	}
	return ret;
}

void datalink_get_broadcast_address ( BACNET_ADDRESS * dest,uint8_t protocal)
{
  if(protocal == BAC_MSTP)
	{
		dlmstp_get_broadcast_address(dest);
	}

	else if(protocal == BAC_IP || protocal == BAC_IP_CLIENT)
	{
		bip_get_broadcast_address(dest);
	}

	
}

void datalink_get_my_address ( BACNET_ADDRESS * my_address,uint8_t protocal)
{

	if(protocal == BAC_MSTP /*|| protocal == BAC_PTP*/)
	{
		dlmstp_get_my_address(my_address);
	}


	else if((protocal == BAC_IP) ||(protocal == BAC_IP_CLIENT))
	{
		bip_get_my_address(my_address);
	}
	
}






//bool(*datalink_init) (char *ifname);
//
///** Function template to send a packet via the DataLink.
// * @ingroup DLTemplates
// *
// * @param dest [in] Destination address.
// * @param npdu_data [in] The NPDU header (Network) information.
// * @param pdu [in] Buffer of data to be sent - may be null.
// * @param pdu_len [in] Number of bytes in the pdu buffer.
// * @return Number of bytes sent on success, negative number on failure.
// */
//int (
//    *datalink_send_pdu) (
//    BACNET_ADDRESS * dest,
//    BACNET_NPDU_DATA * npdu_data,
//    uint8_t * pdu,
//    unsigned pdu_len);
//
//uint16_t(*datalink_receive) (BACNET_ADDRESS * src, uint8_t * pdu,
//    uint16_t max_pdu, unsigned timeout);
//
///** Function template to close the DataLink services and perform any cleanup.
// * @ingroup DLTemplates
// */
//void (
//    *datalink_cleanup) (
//    void);
//
//void (
//    *datalink_get_broadcast_address) (
//    BACNET_ADDRESS * dest);
//
//void (
//    *datalink_get_my_address) (
//    BACNET_ADDRESS * my_address);
//




//void datalink_set(
//    char *datalink_string)
//{
//    if (strcasecmp("bip", datalink_string) == 0) {
//        datalink_init = bip_init;
//        datalink_send_pdu = bip_send_pdu;
//        datalink_receive = bip_receive;
//        datalink_cleanup = bip_cleanup;
//        datalink_get_broadcast_address = bip_get_broadcast_address;
//        datalink_get_my_address = bip_get_my_address;
//    } else if (strcasecmp("ethernet", datalink_string) == 0) {
//        datalink_init = ethernet_init;
//        datalink_send_pdu = ethernet_send_pdu;
//        datalink_receive = ethernet_receive;
//        datalink_cleanup = ethernet_cleanup;
//        datalink_get_broadcast_address = ethernet_get_broadcast_address;
//        datalink_get_my_address = ethernet_get_my_address;
//    } else if (strcasecmp("arcnet", datalink_string) == 0) {
//        datalink_init = arcnet_init;
//        datalink_send_pdu = arcnet_send_pdu;
//        datalink_receive = arcnet_receive;
//        datalink_cleanup = arcnet_cleanup;
//        datalink_get_broadcast_address = arcnet_get_broadcast_address;
//        datalink_get_my_address = arcnet_get_my_address;
//    } else if (strcasecmp("mstp", datalink_string) == 0) {
//        datalink_init = dlmstp_init;
//        datalink_send_pdu = dlmstp_send_pdu;
//        datalink_receive = dlmstp_receive;
//        datalink_cleanup = dlmstp_cleanup;
//        datalink_get_broadcast_address = dlmstp_get_broadcast_address;
//        datalink_get_my_address = dlmstp_get_my_address;
//    }
//}
//#endif
