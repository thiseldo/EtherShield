/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 *
 * Author: Andrew Lindsay
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
#include "ip_config.h"
#include "net.h"
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

dhcpData *dhcpPtr;

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

static char hostname[16];
static uint8_t haveDhcpAnswer=0;
static uint8_t dhcp_ansError=0;
uint32_t currentXid = 0;
uint16_t currentSecs = 0;
static uint32_t leaseStart = 0;
static uint32_t leaseTime = 0;

// Functions prototypes
uint8_t have_dhcpoffer(uint8_t *buf,uint16_t plen);
uint8_t have_dhcpack(uint8_t *buf,uint16_t plen);

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

/*
uint8_t dhcp_get_error_info(void)
{       
        return(dhcp_ansError);
}

uint8_t *dhcp_getip(void)
{       
        return(dhcpip);
}
*/

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
        sprintf( hostname, "Arduino-%02x", macaddr[6] );
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
        send_udp_prepare(buf,(DHCPCLIENT_SRC_PORT_H<<8)|(dhcptid_l&0xff),dhcpip,DHCP_DEST_PORT);

        while(i<6){
                buf[ETH_DST_MAC +i]=0xff;
                i++;
        }
        buf[IP_TOTLEN_L_P]=0x82;
        buf[IP_PROTO_P]=IP_PROTO_UDP_V;
        i=0;
        while(i<4){
                buf[IP_SRC_P+i]=0;
                buf[IP_DST_P+i]=0xff;
                i++;
        }
        buf[UDP_DST_PORT_H_P]=0;
        buf[UDP_DST_PORT_L_P]=DHCP_SRC_PORT; 
        buf[UDP_SRC_PORT_H_P]=0;
        buf[UDP_SRC_PORT_L_P]=DHCP_DEST_PORT;
        // Set length later when known
        buf[UDP_LEN_H_P]=0;
        buf[UDP_LEN_L_P]=0;
        // zero the checksum
        buf[UDP_CHECKSUM_H_P]=0;
        buf[UDP_CHECKSUM_L_P]=0;

        // Build DHCP Packet from buf[UDP_DATA_P]
        // Make dhcpPtr start of UDP data buffer
        dhcpPtr = &buf[UDP_DATA_P];
        // 0-3 op, htype, hlen, hops
        dhcpPtr->op = DHCP_BOOTREQUEST;
        dhcpPtr->htype = 1;
        dhcpPtr->hlen = 6;
        dhcpPtr->hops = 0;
        // 4-7 xid
        dhcpPtr->xid = currentXid;

        // 8-9 secs
        dhcpPtr->secs = currentSecs;

        // 10-11 flags
        dhcpPtr->flags = 0x0000;
        
        // 12-15 ciaddr
        for( i=0; i<4; i++)
                dhcpPtr->ciaddr[i] = 0;

        // 16-19 yiaddr
        for( i=0; i<4; i++)
                dhcpPtr->yiaddr[i] = dhcpip[i];

        // 20-23 siaddr
        for( i=0; i<4; i++)
                dhcpPtr->siaddr[i] = 0;

        // 24-27 giaddr
        for( i=0; i<4; i++)
                dhcpPtr->giaddr[i] = 0;

        // 28-43 chaddr(16)
        for( i=0; i<6; i++)
                dhcpPtr->chaddr[i] = macaddr[i];
        for( i=6; i<16; i++)
                dhcpPtr->chaddr[i] = 0;

        // 44-107 sname (64)
        for( i=0; i<64; i++)
                dhcpPtr->sname[i] = 0;
        // 108-235 file(128)
        for( i=0; i<128; i++)
                dhcpPtr->file[i] = 0;
        // options defined as option, length, value
        int optionIndex = sizeof( dhcpData );
        // Magic cookie 99, 130, 83 and 99
        buf[UDP_DATA_P + optionIndex] = 99;
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 130;
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 83;
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 99;
        optionIndex++;
        
        // Set correct options
        // Option 1 - DHCP message type
        buf[UDP_DATA_P + optionIndex] =  53;   // DHCPDISCOVER, DHCPREQUEST
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 1;      // Length 
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = requestType;      // Value
        optionIndex++;

        // Define options based on if we are reply or request

        if( requestType == DHCPDISCOVER ) {  
                // Option 2
                buf[UDP_DATA_P + optionIndex] = 116;     // Auto configuration
                optionIndex++;
                buf[UDP_DATA_P + optionIndex] = 1;      // Length 
                optionIndex++;
                buf[UDP_DATA_P + optionIndex] = 0x01;      // Value
                optionIndex++;
        }

        // Option 3
        buf[UDP_DATA_P + optionIndex] = 61;     // Client identifier
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 7;      // Length 
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 0x01;      // Value
        optionIndex++;
        for( i=0; i<6; i++)
                buf[UDP_DATA_P + optionIndex +i] = macaddr[+i];
        optionIndex += 6;

        // Option 4
        buf[UDP_DATA_P + optionIndex] = 12;     // Host name
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 10;      // Length 
        optionIndex++;
        // setup hostname
        i = 0;
        while( hostname[i] != 0 ) {
                buf[UDP_DATA_P + optionIndex +i] = hostname[i];
                i++;
        }
        buf[UDP_DATA_P + optionIndex + i] = 0;
        i++;
        optionIndex += i;

        if( requestType == DHCPREQUEST ) {
                buf[UDP_DATA_P + optionIndex] = 50;     // Requested IP address
                optionIndex++;
                buf[UDP_DATA_P + optionIndex] = 4;      // Length 
                optionIndex++;
                for( i=0; i<4; i++)
                        buf[UDP_DATA_P + optionIndex +i] = dhcpip[i];
                optionIndex += 4;

                buf[UDP_DATA_P + optionIndex] = 54;     // Server IP address
                optionIndex++;
                buf[UDP_DATA_P + optionIndex] = 4;      // Length 
                optionIndex++;
                for( i=0; i<4; i++)
                        buf[UDP_DATA_P + optionIndex +i] = dhcpserver[i];
                optionIndex += 4;
        }

        // Option 5 - parameter list - minimal list for what we need
        buf[UDP_DATA_P + optionIndex] = 55;     // Parameter request list
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 4;      // Length 
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 1;      // Subnet mask
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 15;     // Domain name
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 3;      // Route/Gateway
        optionIndex++;
        buf[UDP_DATA_P + optionIndex] = 6;      // DNS Server
        optionIndex++;

        // payload len should be around 300
        send_udp_transmit(buf, optionIndex);
}

// Examine packet, if dhcp then process, if just exit.
// Perform lease expiry checks and initiate renewal
// process the answer from the dhcp server:
// return 1 on sucessful processing of answer.
// We set also the variable haveDhcpAnswer
// Either DHCPOFFER, DHCPACK or DHCPNACK
// Return 0 for nothing processed, 1 for done soemthing
uint8_t check_for_dhcp_answer(uint8_t *buf, uint16_t plen){
        uint8_t j,i;

        // Check for lease expiry
        uint32_t currentSecs = millis();
        if (plen<70){
                return(0);
        }
        if (buf[UDP_SRC_PORT_L_P]!=DHCP_SRC_PORT){
                // not from a DHCP
                return(0);
        }

        // Map struct onto payload
        dhcpPtr = &buf[UDP_DATA_P];

        // Must be a reply
        if( dhcpPtr->op != DHCP_BOOTREPLY ) {
                // Not a reply
                return(0);
        }

        // Must be for our transaction ID
        if( dhcpPtr->xid != currentXid ) {
                // Not a response to us
                return(0);
        }

        // Get Type, either Offer, Ack or Nack
        // Find Options - at end of struct + 4 to skip magic cookie
        //
        int optionIndex = UDP_DATA_P + sizeof( dhcpData ) + 4;
        uint8_t dhcpType = 0;
        if( buf[optionIndex] == 53 ) {
                // Type option
                //optionIndex++;
                dhcpType = buf[optionIndex+2];
                switch( dhcpType ) {
                        case DHCPOFFER:
                                return(have_dhcpoffer( buf, plen ));
                        case DHCPACK:
                                return(have_dhcpack( buf, plen ));
                }
        }

        return( 0 );
}


uint8_t have_dhcpoffer(uint8_t *buf,uint16_t plen) {
  // Map struct onto payload
  dhcpPtr = &buf[UDP_DATA_P];
  int optionIndex = UDP_DATA_P + sizeof( dhcpData ) + 4;

  // Offered IP address is in yiaddr
  uint8_t n;
  for( n=0; n<4; n++ )
          dhcpip[n] = dhcpPtr->yiaddr[n];

  // Scan through variable length option list identifying 
  // options we want
  uint8_t moreOptions = 1;
  int optionLen = 0;
  int i = 0;
  while( moreOptions > 0 ) {
          optionLen = buf[optionIndex+1];
          switch( buf[optionIndex] ) {
                // Option: (t=1,l=4) Subnet Mask = 255.255.255.0
                case 1:
                        for( i = 0; i<4; i++ )
                                dhcpmask[i] = buf[optionIndex+2+i];
                        break;
                // Option: (t=3,l=4) Router = 192.168.1.1
                case 3:
                        for( i = 0; i<4; i++ )
                                gwaddr[i] = buf[optionIndex+2+i];
                        break;
                // Option: (t=6,l=8) Domain Name Server
                case 6:
                        for( i = 0; i<4; i++ )
                                dnsserver[i] = buf[optionIndex+2+i];
                        break;
                // Option: (t=15,l=4) Domain Name = "home"
                case 15:
                        break;
                // Option: (t=51,l=4) IP Address Lease Time = 1 day
                case 51:
                        leaseTime = 0;
                        for( i = 0; i<4; i++ ) {
                                leaseTime += buf[optionIndex+2+i];
                                leaseTime = leaseTime << 8;
                        }
                        leaseTime *= 1000;      // milliseconds
                        break;
                // Option: (t=54,l=4) Server Identifier = 192.168.1.1
                case 54:
                        for( i = 0; i<4; i++ )
                                dhcpserver[i] = buf[optionIndex+2+i];
                        break;
                default:
                        break;
          }
          // Just move index
          optionIndex += buf[optionIndex+1] +2;
          if( optionIndex >= plen ) 
                  moreOptions = 0;
  }
  dhcp_request_ip( buf );
  return(1);
}

uint8_t have_dhcpack(uint8_t *buf,uint16_t plen) {
  dhcpState = DHCP_STATE_OK;    // Complete for now
  leaseStart = millis();

  return(2);
}

#endif

/* end of dhcp.c */
