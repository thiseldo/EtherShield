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

//------------- functions in ip_arp_udp_tcp.c --------------
// an NTP client (ntp clock):
#define NTP_client
// a spontaneous sending UDP client
#define UDP_client

// to send out a ping:
#undef PING_client
#define PINGPATTERN 0x42

// a UDP wake on lan sender:
#undef WOL_client

// a "web browser". This can be use to upload data
// to a web server on the internet by encoding the data 
// into the url (like a Form action of type GET):
#define WWW_client
#define TCP_client
// if you do not need a browser and just a server:
//#undef WWW_client
//
//------------- functions in websrv_help_functions.c --------------
//
// functions to decode cgi-form data:
#define FROMDECODE_websrv_help

// function to encode a URL (mostly needed for a web client)
#define URLENCODE_websrv_help

// DNS lookup support
#define DNS_client

#endif /* IP_CONFIG_H */
//@}
