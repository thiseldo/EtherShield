/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 *
 * Author: Andrew Lindsay
 * Rewritten and optimized by Jean-Claude Wippler, http://jeelabs.org/
 * Please visit Jeelabs for tons of cool stuff.
 *
 * Copyright: GPL V2
 * See http://www.gnu.org/licenses/gpl.html
 *
 * DHCP look-up functions based on the udp client
 * http://www.ietf.org/rfc/rfc2131.txt
 *********************************************/
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <stdlib.h>
#include "enc28j60.h"
#include "ip_config.h"
#include "net.h"
#include "dhcp.h"
#include "ip_arp_udp_tcp.h"

#if defined (UDP_client) 

#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTREPLY 2
#define DHCPDISCOVER 0x01
#define DHCPOFFER  0x02
#define DHCPREQUEST 0x03
#define DHCPACK 0x05
//#define DHCPNACK

// size 236
typedef struct dhcpData {
        // 0-3 op, htype, hlen, hops
        uint8_t op;
        uint8_t htype;
        uint8_t hlen;
        uint8_t hops;
        // 4-7 xid
        uint32_t xid;
        // 8-9 secs
        uint16_t secs;
        // 10-11 flags
        uint16_t flags;
        // 12-15 ciaddr
        uint8_t ciaddr[4];
        // 16-19 yiaddr
        uint8_t yiaddr[4];
        // 20-23 siaddr
        uint8_t siaddr[4];
        // 24-27 giaddr
        uint8_t giaddr[4];
        // 28-43 chaddr(16)
        uint8_t chaddr[16];
        // 44-107 sname (64)
        uint8_t sname[64];
        // 108-235 file(128)
        uint8_t file[128];
        // options
        // dont include as its variable size,
        // just tag onto end of structure
} dhcpData;

static uint8_t dhcptid_l=0; // a counter for transaction ID
// src port high byte must be a a0 or higher:
#define DHCPCLIENT_SRC_PORT_H 0xe0 
#define DHCP_SRC_PORT 67
#define DHCP_DEST_PORT 68

static uint8_t dhcpState = DHCP_STATE_INIT;

// Pointers to values we have set or need to set
static uint8_t *macaddr;
uint8_t *dhcpip;
uint8_t *dhcpmask;
uint8_t *dhcpserver;
uint8_t *dnsserver;
uint8_t *gwaddr;

static char hostname[] = "Arduino-00";
static uint8_t haveDhcpAnswer=0;
static uint8_t dhcp_ansError=0;
uint32_t currentXid = 0;
uint16_t currentSecs = 0;
static uint32_t leaseStart = 0;
static uint32_t leaseTime = 0;
static uint8_t* bufPtr;

static void addToBuf(uint8_t b) {
    *bufPtr++ = b;
}

uint8_t dhcp_state(void)
{
        // Check lease and request renew if currently OK and time
        // leaseStart - start time in millis
        // leaseTime - length of lease in millis
        //
        if( dhcpState == DHCP_STATE_OK && (leaseStart + leaseTime) <= millis() ) {
                // Calling app needs to detect this and init renewal
                dhcpState = DHCP_STATE_RENEW;
        }
        return(dhcpState);
}

// Start request sequence, send DHCPDISCOVER
// Wait for DHCPOFFER
// Send DHCPREQUEST
// Wait for DHCPACK
// All configured
void dhcp_start(uint8_t *buf, uint8_t *macaddrin, uint8_t *ipaddrin,
                uint8_t *maskin, uint8_t *gwipin, uint8_t *dhcpsvrin,
                uint8_t *dnssvrin )
{
        macaddr = macaddrin;
        dhcpip = ipaddrin;
        dhcpmask = maskin;
        gwaddr = gwipin;
        dhcpserver = dhcpsvrin;
        dnsserver = dnssvrin;
        srand(analogRead(0));
        currentXid = 0x00654321 + rand();
        currentSecs = 0;
        int n;
        for( n=0; n<4; n++ ) {
          dhcpip[n] = 0;
          dhcpmask[n] = 0;
          gwaddr[n] = 0;
          dhcpserver[n] = 0;
          dnsserver[n] = 0;
        }
        // Set a unique hostname, use Arduino- plus last octet of mac address
        hostname[8] = 'A' + (macaddr[5] >> 4);
        hostname[9] = 'A' + (macaddr[5] & 0x0F);

        // Reception of broadcast packets turned off by default, but
        // it has been shown that some routers send responses as
        // broadcasts. Enable here and disable later
        enc28j60EnableBroadcast();
        dhcp_send( buf, DHCPDISCOVER );
        dhcpState = DHCP_STATE_DISCOVER;
}

void dhcp_request_ip(uint8_t *buf )
{
        dhcp_send( buf, DHCPREQUEST );
        dhcpState = DHCP_STATE_REQUEST;
}


// Main DHCP message sending function, either DHCPDISCOVER or DHCPREQUEST
void dhcp_send(uint8_t *buf, uint8_t requestType ) {
        uint8_t lenpos,lencnt;
        int i=0;
        char c;
        haveDhcpAnswer=0;
        dhcp_ansError=0;
        dhcptid_l++; // increment for next request, finally wrap
        // destination IP gets replaced after this call
       
        memset(buf, 0, 400); //XXX OUCH!   
        send_udp_prepare(buf,(DHCPCLIENT_SRC_PORT_H<<8)|(dhcptid_l&0xff),dhcpip,DHCP_DEST_PORT);

        memcpy(buf + ETH_SRC_MAC, macaddr, 6);
        memset(buf + ETH_DST_MAC, 0xFF, 6);
        buf[IP_TOTLEN_L_P]=0x82;
        buf[IP_PROTO_P]=IP_PROTO_UDP_V;
        memset(buf + IP_DST_P, 0xFF, 4);
        buf[UDP_DST_PORT_L_P]=DHCP_SRC_PORT; 
        buf[UDP_SRC_PORT_H_P]=0;
        buf[UDP_SRC_PORT_L_P]=DHCP_DEST_PORT;

        // Build DHCP Packet from buf[UDP_DATA_P]
        // Make dhcpPtr start of UDP data buffer
        dhcpData *dhcpPtr = &buf[UDP_DATA_P];
        // 0-3 op, htype, hlen, hops
        dhcpPtr->op = DHCP_BOOTREQUEST;
        dhcpPtr->htype = 1;
        dhcpPtr->hlen = 6;
        dhcpPtr->hops = 0;
        // 4-7 xid
        dhcpPtr->xid = currentXid;
        // 8-9 secs
        dhcpPtr->secs = currentSecs;
        // 16-19 yiaddr
        memset(dhcpPtr->yiaddr, 0, 4);
        // 28-43 chaddr(16)
        memcpy(dhcpPtr->chaddr, macaddr, 6);

        // options defined as option, length, value
        bufPtr = buf + UDP_DATA_P + sizeof( dhcpData );
        // Magic cookie 99, 130, 83 and 99
        addToBuf(99);
        addToBuf(130);
        addToBuf(83);
        addToBuf(99);
        
        // Set correct options
        // Option 1 - DHCP message type
        addToBuf(53);   // DHCPDISCOVER, DHCPREQUEST
        addToBuf(1);      // Length 
        addToBuf(requestType);      // Value

        // Client Identifier Option, this is the client mac address
        addToBuf(61);     // Client identifier
        addToBuf(7);      // Length 
        addToBuf(0x01);      // Value
        for( i=0; i<6; i++)
                addToBuf(macaddr[i]);

        // Host name Option
        addToBuf(12);     // Host name
        addToBuf(10);      // Length 
        for( i=0; i<10; i++)
                addToBuf(hostname[i]);

        if( requestType == DHCPREQUEST ) {
                // Request IP address
                addToBuf(50);     // Requested IP address
                addToBuf(4);      // Length 
                for( i=0; i<4; i++)
                        addToBuf(dhcpip[i]);

                // Request using server ip address
                addToBuf(54);     // Server IP address
                addToBuf(4);      // Length 
                for( i=0; i<4; i++)
                        addToBuf(dhcpserver[i]);
        }

        // Additional information in parameter list - minimal list for what we need
        addToBuf(55);     // Parameter request list
        addToBuf(3);      // Length 
        addToBuf(1);      // Subnet mask
        addToBuf(3);      // Route/Gateway
        addToBuf(6);      // DNS Server

        // payload len should be around 300
        addToBuf(255);      // end option
        send_udp_transmit(buf, bufPtr - buf - UDP_DATA_P);
}

// Examine packet, if dhcp then process, if just exit.
// Perform lease expiry checks and initiate renewal
// process the answer from the dhcp server:
// return 1 on sucessful processing of answer.
// We set also the variable haveDhcpAnswer
// Either DHCPOFFER, DHCPACK or DHCPNACK
// Return 0 for nothing processed, 1 for done soemthing
uint8_t check_for_dhcp_answer(uint8_t *buf, uint16_t plen){
    // Map struct onto payload
    dhcpData *dhcpPtr = &buf[UDP_DATA_P];
    if (plen >= 70 && buf[UDP_SRC_PORT_L_P] == DHCP_SRC_PORT &&
            dhcpPtr->op == DHCP_BOOTREPLY && dhcpPtr->xid == currentXid ) {
        // Check for lease expiry
        // uint32_t currentSecs = millis();
        int optionIndex = UDP_DATA_P + sizeof( dhcpData ) + 4;
        if( buf[optionIndex] == 53 )
            switch( buf[optionIndex+2] ) {
                case DHCPOFFER: return have_dhcpoffer( buf, plen );
                case DHCPACK:   return have_dhcpack( buf, plen );
            }
    }
    return 0;
}


uint8_t have_dhcpoffer (uint8_t *buf,uint16_t plen) {
    // Map struct onto payload
    dhcpData *dhcpPtr = buf + UDP_DATA_P;
    // Offered IP address is in yiaddr
    memcpy(dhcpip, dhcpPtr->yiaddr, 4);
    // Scan through variable length option list identifying options we want
    uint8_t *ptr = (uint8_t*) (dhcpPtr + 1) + 4;
    do {
        uint8_t option = *ptr++;
        uint8_t optionLen = *ptr++;
        uint8_t i;
        switch (option) {
            case 1:  memcpy(dhcpmask, ptr, 4);
                     break;
            case 3:  memcpy(gwaddr, ptr, 4);
                     break;
            case 6:  memcpy(dnsserver, ptr, 4);
                     break;
            case 51: leaseTime = 0;
                     for (i = 0; i<4; i++)
                         leaseTime = (leaseTime + ptr[i]) << 8;
                     leaseTime *= 1000;      // milliseconds
                     break;
            case 54: memcpy(dhcpserver, ptr, 4);
                     break;
        }
        ptr += optionLen;
    } while (ptr < buf + plen);
    dhcp_request_ip( buf );
    return 1;
}

uint8_t have_dhcpack (uint8_t *buf,uint16_t plen) {
    dhcpState = DHCP_STATE_OK;
    leaseStart = millis();
    // Turn off broadcast. Application if it needs it can re-enable it
    enc28j60DisableBroadcast();
    return 2;
}

#endif

/* end of dhcp.c */
