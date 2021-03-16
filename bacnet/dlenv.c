/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
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

/* environment variables used for the command line tools */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacnet.h"
#include "bacdef.h"
#include "apdu.h"
#include "datalink.h"
#include "handlers.h"
#include "dlenv.h"
#include "tsm.h"
#include "bip.h"

//#include "func_linux.h"

/** @file dlenv.c  Initialize the DataLink configuration. */
#define htons(n) ((((uint16_t)((n) & 0xff)) << 8) | (((n) & 0xff00) >> 8))
#define ntohs(n) (((((uint16_t)(n) & 0xFF)) << 8) | (((uint16_t)(n) & 0xFF00) >> 8))
#define htonl(n) (((((uint32_t)(n) & 0xFF)) << 24) | \
                  ((((uint32_t)(n) & 0xFF00)) << 8) | \
                  ((((uint32_t)(n) & 0xFF0000)) >> 8) | \
                  ((((uint32_t)(n) & 0xFF000000)) >> 24))

#if defined(BACDL_BIP)
/* timer used to renew Foreign Device Registration */
static uint16_t BBMD_Timer_Seconds;
/* BBMD variables */
static long bbmd_timetolive_seconds = 60000;
static long bbmd_port = 0xBAC0;
static long bbmd_address = 0;
static int bbmd_result = 0;

void register_ftd(long address,int port,int time)
{
	dlenv_bbmd_address_set(address);
	dlenv_bbmd_port_set(port);
	dlenv_bbmd_ttl_set(time);
	dlenv_init();	
}

/* Simple setters for BBMD registration variables. */

/** Sets the IPv4 address for BBMD registration.
 * If not set here or provided by Environment variables,
 * no BBMD registration will occur.
 * @param address - IPv4 address (long) of BBMD to register with,
 *                       in network byte order.
 */
void dlenv_bbmd_address_set(
    long address)
{
    bbmd_address = address;
}

/** Set the port for BBMD registration.
 * Default if not set is 0xBAC0.
 * @param port - The port number (provided in network byte order).
 */
void dlenv_bbmd_port_set(
    int port)
{
    bbmd_port = port;
}

/** Set the Lease Time (Time-to-Live) for BBMD registration.
 * Default if not set is 60000 (1000 minutes).
 * @param ttl_secs - The Lease Time, in seconds.
 */
void dlenv_bbmd_ttl_set(
    int ttl_secs)
{
    bbmd_timetolive_seconds = ttl_secs;
}

/** Get the result of the last attempt to register with the indicated BBMD.
 * If we sent a foreign registration request, then see if we've received
 * a NAK in our BVLC handler.
 *
 * @return Positive number (of bytes sent) if registration was successful,
 *         0 if no registration request was made, or
 *         -1 if registration attempt failed.
 */
int dlenv_bbmd_result(
    void)
{
    if ((bbmd_result > 0) &&
        (bvlc_get_last_result() == BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK))
        return -1;
    /* Else, show our send: */
    return bbmd_result;
}
#endif

/** Register as a Foreign Device with the designated BBMD.
 * @ingroup DataLink
 * The BBMD's address, port, and lease time must be provided by
 * internal variables or Environment variables.
 * If no address for the BBMD is provided, no BBMD registration will occur.
 *
 * The Environment Variables depend on define of BACDL_BIP:
 *     - BACNET_BBMD_PORT - 0..65534, defaults to 47808
 *     - BACNET_BBMD_TIMETOLIVE - 0..65535 seconds, defaults to 60000
 *     - BACNET_BBMD_ADDRESS - dotted IPv4 address
 * @return Positive number (of bytes sent) on success,
 *         0 if no registration request is sent, or
 *         -1 if registration fails.
 */
int dlenv_register_as_foreign_device(
    void)
{
    int retval = 0;
#if defined(BACDL_BIP)
//    char far *pEnv = NULL;
///*    pEnv = getenv("BACNET_BBMD_PORT");   chelsea
//    if (pEnv)*/ {
//        bbmd_port = strtol(pEnv, NULL, 0);
//        if (bbmd_port > 0xFFFF) {
//            bbmd_port = 0xBAC0;
//        }
//    }
//   /* pEnv = getenv("BACNET_BBMD_TIMETOLIVE");	  chelsea
//    if (pEnv)*/ {
//        bbmd_timetolive_seconds = strtol(pEnv, NULL, 0);
//        if (bbmd_timetolive_seconds > 0xFFFF) {
//            bbmd_timetolive_seconds = 0xFFFF;
//        }
//    }
//   /* pEnv = getenv("BACNET_BBMD_ADDRESS");		 chelsea
//    if (pEnv)*/ {	
//    //    bbmd_address = bip_getaddrbyname(pEnv);
//    }
    if (bbmd_address) {
        uint32_t s_addr;//struct in_addr addr;	  chelsea
        s_addr = bbmd_address;
     /*   fprintf(stderr, "Registering with BBMD at %s:%ld for %ld seconds\n",	 chelsea
            inet_ntoa(addr), bbmd_port, bbmd_timetolive_seconds); */
        retval =
            bvlc_register_with_bbmd(bbmd_address, htons((uint16_t) bbmd_port),
            (uint16_t) bbmd_timetolive_seconds);
     /*   if (retval < 0)
            fprintf(stderr, "FAILED to Register with BBMD at %s \n",	  chelsea
                inet_ntoa(addr));  */
        BBMD_Timer_Seconds = (uint16_t) bbmd_timetolive_seconds;
    }

    bbmd_result = retval;
#endif
    return retval;
}




/** Datalink maintenance timer
 * @ingroup DataLink
 *
 * Call this function to renew our Foreign Device Registration
 * @param elapsed_seconds Number of seconds that have elapsed since last called.
 */
void dlenv_maintenance_timer(
    uint16_t elapsed_seconds)
{
#if defined(BACDL_BIP)
    if (BBMD_Timer_Seconds) {
        if (BBMD_Timer_Seconds <= elapsed_seconds) {
            BBMD_Timer_Seconds = 0;
        } else {
            BBMD_Timer_Seconds -= elapsed_seconds;
        }
        if (BBMD_Timer_Seconds == 0) {
            (void) dlenv_register_as_foreign_device();
            /* If that failed (negative), maybe just a network issue.
             * If nothing happened (0), may be un/misconfigured.
             * Set up to try again later in all cases. */
            BBMD_Timer_Seconds = (uint16_t) bbmd_timetolive_seconds;
        }
    }
#endif
}

/** Initialize the DataLink configuration from Environment variables,
 * or else to defaults.
 * @ingroup DataLink
 * The items configured depend on which BACDL_ the code is built for,
 * eg, BACDL_BIP.
 *
 * For most items, checks first for an environment variable, and, if
 * found, uses that to set the item's value.  Otherwise, will set
 * to a default value.
 *
 * The Environment Variables, by BACDL_ type, are:
 * - BACDL_ALL: (the general-purpose solution)
 *   - BACNET_DATALINK to set which BACDL_ type we are using.
 * - (Any):
 *   - BACNET_APDU_TIMEOUT - set this value in milliseconds to change
 *     the APDU timeout.  APDU Timeout is how much time a client
 *     waits for a response from a BACnet device.
 *   - BACNET_IFACE - set this value to dotted IP address (Windows) of
 *     the interface (see ipconfig command on Windows) for which you
 *     want to bind.  On Linux, set this to the /dev interface
 *     (i.e. eth0, arc0).  Default is eth0 on Linux, and the default
 *     interface on Windows.  Hence, if there is only a single network
 *     interface on Windows, the applications will choose it, and this
 *     setting will not be needed.
 * - BACDL_BIP: (BACnet/IP)
 *   - BACNET_IP_PORT - UDP/IP port number (0..65534) used for BACnet/IP
 *     communications.  Default is 47808 (0xBAC0).
 *   - BACNET_BBMD_PORT - UDP/IP port number (0..65534) used for Foreign
 *       Device Registration.  Defaults to 47808 (0xBAC0).
 *   - BACNET_BBMD_TIMETOLIVE - number of seconds used in Foreign Device
 *       Registration (0..65535). Defaults to 60000 seconds.
 *   - BACNET_BBMD_ADDRESS - dotted IPv4 address of the BBMD or Foreign
 *       Device Registrar.
 * - BACDL_MSTP: (BACnet MS/TP)
 *   - BACNET_MAX_INFO_FRAMES
 *   - BACNET_MAX_MASTER
 *   - BACNET_MSTP_BAUD
 *   - BACNET_MSTP_MAC
 */

void dlenv_init(
    void)
{
    char *pEnv = NULL;
//		Send_WhoIs_Init();

//		bip_set_socket(TCPIP_UdpNew(whois_InterAppId, 0, bbmd_address,
//		47808, 47808));

    dlenv_register_as_foreign_device();
}

