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
static int bip_udp_sock = -1;
static struct sockaddr_in bip_last_source = { 0 };
/* port to use - stored in network byte order */
static uint16_t BIP_Port = 0;   /* this will force initialization in demos */
/* IP Address - stored in network byte order */
static struct in_addr BIP_Address;
/* Broadcast Address - stored in network byte order */
static struct in_addr BIP_Broadcast_Address;

uint8_t bip_mpdu_dest_valid = 0;
struct sockaddr_in bip_mpdu_dest = { 0 };

void bip_set_udp_sock(
    int sock_fd)
{
    bip_udp_sock = sock_fd;
}

int bip_socket(
    void)
{
    return bip_udp_sock;
}

void bip_set_addr(
    uint32_t net_address)
{
    BIP_Address.s_addr = net_address;
}

uint32_t bip_get_addr(
    void)
{
    return BIP_Address.s_addr;
}

uint16_t bip_get_port(
    void)
{
    return BIP_Port;
}

void bip_set_source_addr(
    struct sockaddr_in *sin)
{
    if (sin) {
        bip_last_source = *sin;
    }
}

void bip_get_source_addr(
    struct sockaddr_in *sin)
{
    if (sin) {
        *sin = bip_last_source;
    }
}

int bip_get_udp_sock(
    void)
{
    return bip_udp_sock;
}

int bip_send_mpdu_addr(
    uint32_t dest_addr,
    uint16_t dest_port,
    uint8_t * mtu,
    uint16_t mtu_len)
{
    struct sockaddr_in dest;

    if ((bip_udp_sock < 0) || !mtu || (mtu_len == 0)) {
        return 0;
    }

    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dest_addr;
    dest.sin_port = dest_port;

    return sendto(bip_udp_sock, mtu, mtu_len, 0,
        (struct sockaddr *) &dest, sizeof(dest));
}

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

uint32_t bip_get_broadcast_addr(
    void)
{
    return BIP_Broadcast_Address.s_addr;
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

static bool bip_is_broadcast_dest(
    BACNET_ADDRESS * dest)
{
    return (dest->net == BACNET_BROADCAST_NETWORK) || ((dest->net > 0) &&
        (dest->len == 0)) || (dest->mac_len == 0);
}

static void bip_update_client_send_address(
    struct in_addr *address,
    uint16_t port)
{
    uint32_t addr = address->s_addr;

    Send_bip_address[0] = (uint8_t) (addr >> 24);
    Send_bip_address[1] = (uint8_t) (addr >> 16);
    Send_bip_address[2] = (uint8_t) (addr >> 8);
    Send_bip_address[3] = (uint8_t) (addr);
    Send_bip_address[4] = (uint8_t) (port >> 8);
    Send_bip_address[5] = (uint8_t) (port);
}

static int bip_build_bvlc_packet(
    uint8_t *buf,
    BACNET_ADDRESS * dest,
    uint8_t * pdu,
    unsigned pdu_len,
    struct in_addr *address,
    uint16_t *port,
    bool set_client_dest)
{
    int len = 0;

    buf[0] = BVLL_TYPE_BACNET_IP;
    if (bip_is_broadcast_dest(dest)) {
        if (bvlc_is_foreign_device()) {
            buf[1] = BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK;
            address->s_addr = bvlc_get_bbmd_addr();
            *port = bvlc_get_bbmd_port();
        } else {
            buf[1] = BVLC_ORIGINAL_BROADCAST_NPDU;
            address->s_addr = BIP_Broadcast_Address.s_addr;
            *port = BIP_Port;
        }
    } else if (dest->mac_len == 6) {
        bip_decode_bip_address(dest, address, port);
        buf[1] = BVLC_ORIGINAL_UNICAST_NPDU;
    } else {
        return -1;
    }

    if (set_client_dest && bip_is_broadcast_dest(dest)) {
        bip_update_client_send_address(address, *port);
    }

    len = 2;
    len += encode_unsigned16(&buf[len], (uint16_t) (pdu_len + 4));
    memcpy(&buf[len], pdu, pdu_len);

    return len + (int) pdu_len;
}

int bip_send_pdu(
    BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,      /* any data to be sent - may be null */
    unsigned pdu_len,uint8_t protocal
	)
{       /* number of bytes of data */  
    struct in_addr address;
    uint16_t port = 0;
    int bytes_sent = 0;

    (void) npdu_data;
    (void) protocal;

    bytes_sent = bip_build_bvlc_packet(bip_send_buf, dest, pdu, pdu_len,
        &address, &port, false);
    if (bytes_sent < 0) {
        return -1;
    }

    bip_send_len = bytes_sent;
    bip_mpdu_dest_valid = 0;
    if (bip_is_broadcast_dest(dest) && bvlc_is_foreign_device()) {
        bip_mpdu_dest.sin_family = AF_INET;
        bip_mpdu_dest.sin_addr = address;
        bip_mpdu_dest.sin_port = port;
        memset(bip_mpdu_dest.sin_zero, 0, sizeof(bip_mpdu_dest.sin_zero));
        bip_mpdu_dest_valid = 1;
    }

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
    struct in_addr address;
    uint16_t port = 0;
    int bytes_sent = 0;

    (void) npdu_data;
    (void) protocal;

    bytes_sent = bip_build_bvlc_packet(bip_client_send_buf, dest, pdu, pdu_len,
        &address, &port, true);
    if (bytes_sent < 0) {
        return -1;
    }

    bip_client_send_len = bytes_sent;

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
    uint16_t pdu_len = 0;
    struct sockaddr_in sin = { 0 };
    int function = 0;

    received_bytes = Get_bip_len();
    bip_Data = pdu;

    if ((protocal == BAC_IP) && (received_bytes <= 0)) {
        return 0;
    }

    if (pdu[0] != BVLL_TYPE_BACNET_IP) {
        return 0;
    }

    function = pdu[1];
    (void) decode_unsigned16(&pdu[2], &pdu_len);
    pdu_len -= 4;

    sin.sin_family = AF_INET;
    if (protocal == BAC_IP) {
        bip_get_source_addr(&sin);
    } else if (protocal == BAC_IP_CLIENT) {
        memcpy(&sin.sin_addr.s_addr, BIP_src_addr, 4);
        memcpy(&sin.sin_port, &BIP_src_addr[4], 2);
    }

    return bvlc_handle_npdu(src, pdu, function, pdu_len, max_pdu, &sin);
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
