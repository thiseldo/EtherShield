/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher 
 * Copyright: GPL V2
 *
 * This file can be used to decide which functionallity of the
 * TCP/IP stack shall be available.
 *
 *********************************************/
//@{
#ifndef IP_CONFIG_H
#define IP_CONFIG_H

// IF using other SPI devices, for example RF12B modules then define this
// This alters the way the SS/CS pin is accessed. If set then it can be changed
// If not set then it is fixed at 10 and uses direct I/O
// Also for RF12 use the speed is reduced
//#define USE_RF12
#undef USE_RF12

// To provide flexibility, there is the option to have URLs etc in either
// Flash or program memory. In flash they cant be changed, in program memory
// they can be defined at runtime but take up program memory space.
// Define FLASH_VARS to use flash memory, undef it for the more flexible option
//
// This applies only to the ES_client_browse_url and ES_client_http_post functions.
// With FLASH_VARS the definitions are:
//
//  void ES_client_browse_url(prog_char *urlbuf, char *urlbuf_varpart,
//  prog_char *hoststr, void (*callback)(uint8_t,uint16_t));
//
//  void ES_client_http_post(prog_char *urlbuf, prog_char *hoststr,
//      prog_char *additionalheaderline, prog_char *method, char *postval,
//      void (*callback)(uint8_t,uint16_t));
//
// Without FLASH_VARS the definitions are:
//
//  void ES_client_browse_url(char *urlbuf, char *urlbuf_varpart,
//      char *hoststr, void (*callback)(uint8_t,uint16_t));
//
//  void ES_client_http_post(char *urlbuf, char *hoststr,
//      char *additionalheaderline, char *method, char *postval,
//      void (*callback)(uint8_t,uint16_t));
// 
#define FLASH_VARS 1
//#undef FLASH_VARS

//------------- functions in ip_arp_udp_tcp.c --------------
// an NTP client (ntp clock):
//#define NTP_client 1
// a spontaneous sending UDP client
#define UDP_client 1

// to send out a ping:
#undef PING_client
#define PINGPATTERN 0x42

// a UDP wake on lan sender:
//#define WOL_client

// a "web browser". This can be use to upload data
// to a web server on the internet by encoding the data 
// into the url (like a Form action of type GET):
#define WWW_client 1
#define TCP_client 1
// if you do not need a browser and just a server:
//#undef WWW_client
//
//------------- functions in websrv_help_functions.c --------------
//
// functions to decode cgi-form data:
#define FROMDECODE_websrv_help 1

// function to encode a URL (mostly needed for a web client)
#define URLENCODE_websrv_help 1

// DNS lookup support
#define DNS_client 1

// DHCP support
#define DHCP_client 1

#endif /* IP_CONFIG_H */
//@}
