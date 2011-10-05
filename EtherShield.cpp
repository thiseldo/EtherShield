/*! \mainpage Arduino ENC28J60 EtherShield Library

\section Introduction

This library is derived from original code by Guido Socher and Pascal Stang, and hence licensed as GPL2. See http://www.gnu.org/licenses/gpl.html

It comprises a C++ class wrapper and a number of C files. It still follows pretty much the same structure as the original code that it was based on.

The Arduino EtherShield Library was initially created by Xing Yu of Nuelectronics, http://www.nuelectronics.com/estore/index.php?main_page=product_info&cPath=1&products_id=4

The library was heavily modified and improved by Andrew D. Lindsay (http://blog.thiseldo.co.uk) with extra code from the Tuxgraphics.org ethernet library (http://www.tuxgraphics.org/electronics/200905/embedded-tcp-ip-stack.shtml), which also originated from the Pascal Stang code.

Further additions include the DHCP implementation with some assistance from JCW at http://jeelabs.org which is now used in their own version of the library for their EtherCard at http://jeelabs.net/projects/hardware/wiki/Ether_Card.

The library is now being used successfully with the Nanode, as minimal Ethernet connected Arduino compatible board, details available from http://wiki.london.hackspace.org.uk/view/Project:Nanode

\section Download

Download the latest library and examples from https://github.com/thiseldo/EtherShield

To fully utilise the Nanode board, you will also require a library that can access the onboard MAC address chip.
One such library is available from https://github.com/thiseldo/NanodeMAC and is used in the examples provided with this library.

\section Instalation

The library .zip file downloaded from https://github.com/thiseldo/EtherShield should be renamed to EtherShield.zip or EtherShield.tar.gz depending on the archive file you're downloading.

The file should be extracted to the sketchbook/libraries/ folder so that there is a subdirectory called EtherSheild containing all the files from the archive.

\section License

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  A copy of the GNU Lesser General Public
  License is available from http://www.gnu.org/licenses/gpl.html; or write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 */
#if (ARDUINO >= 100)
#include <Arduino.h>
#endif

extern "C" {
	#include "ip_config.h"
	#include "enc28j60.h"
	#include "ip_arp_udp_tcp.h"
	#include "websrv_help_functions.h"
	#if (ARDUINO >= 100)
#else
	#include "wiring.h"
#endif
#ifdef DNS_client
	#include "dnslkup.h"
#endif
#ifdef DHCP_client
	#include "dhcp.h"
#endif
}
#include "EtherShield.h"

/**
 * Default constructor
 */
EtherShield::EtherShield(){
}

/**
 * Initialise SPI, separate from main initialisation so that
 * multiple SPI devices can be used together
 */
void EtherShield::ES_enc28j60SpiInit(){
  enc28j60SpiInit();
}

/**
 * Initialise the ENC28J60 using default chip select pin
 */
void EtherShield::ES_enc28j60Init(uint8_t* macaddr){
  ES_enc28j60Init(macaddr, DEFAULT_ENC28J60_CONTROL_CS );
}


/**
 * Initialise the ENC28J60 using selectable chip select pin
 * Flash the 2 MagJack LEDs
 */
void EtherShield::ES_enc28j60Init( uint8_t* macaddr, uint8_t csPin ) {
  /*initialize enc28j60*/
  enc28j60InitWithCs( macaddr, csPin );
  enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
  delay(10);

  for( int f=0; f<3; f++ ) {
  	// 0x880 is PHLCON LEDB=on, LEDA=on
  	// enc28j60PhyWrite(PHLCON,0b0011 1000 1000 00 00);
  	enc28j60PhyWrite(PHLCON,0x3880);
  	delay(500);

  	// 0x990 is PHLCON LEDB=off, LEDA=off
  	// enc28j60PhyWrite(PHLCON,0b0011 1001 1001 00 00);
  	enc28j60PhyWrite(PHLCON,0x3990);
  	delay(500);
  }

  // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
  // enc28j60PhyWrite(PHLCON,0b0011 0100 0111 01 10);
  enc28j60PhyWrite(PHLCON,0x3476);
  delay(100);
}

void EtherShield::ES_enc28j60clkout(uint8_t clk){
	enc28j60clkout(clk);
}

uint8_t EtherShield::ES_enc28j60linkup(void) {
	return enc28j60linkup();
}

void EtherShield::ES_enc28j60EnableBroadcast( void ) {
	enc28j60EnableBroadcast();
}

void EtherShield::ES_enc28j60DisableBroadcast( void ) {
	enc28j60DisableBroadcast();
}

void EtherShield::ES_enc28j60EnableMulticast( void ) {
	enc28j60EnableMulticast();
}

void EtherShield::ES_enc28j60DisableMulticast( void ) {
	enc28j60DisableMulticast();
}

uint8_t EtherShield::ES_enc28j60Read( uint8_t address ) {
	return enc28j60Read( address );
}

uint8_t EtherShield::ES_enc28j60Revision(void) {
	return enc28j60getrev();
}

void EtherShield::ES_enc28j60PhyWrite(uint8_t address, uint16_t data){
	enc28j60PhyWrite(address,  data);
}

uint16_t EtherShield::ES_enc28j60PacketReceive(uint16_t len, uint8_t* packet){
	return enc28j60PacketReceive(len, packet);
}

void EtherShield::ES_enc28j60PacketSend(uint16_t len, uint8_t* packet){
	enc28j60PacketSend(len, packet);
}

void EtherShield::ES_init_ip_arp_udp_tcp(uint8_t *mymac,uint8_t *myip,uint16_t wwwp){
	init_ip_arp_udp_tcp(mymac,myip,wwwp);
}

uint8_t EtherShield::ES_eth_type_is_arp_and_my_ip(uint8_t *buf,uint16_t len) {
	return eth_type_is_arp_and_my_ip(buf,len);
}

uint8_t EtherShield::ES_eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len) {
	eth_type_is_ip_and_my_ip(buf, len);
}

void EtherShield::ES_make_echo_reply_from_request(uint8_t *buf,uint16_t len) {
	make_echo_reply_from_request(buf,len);
}

void EtherShield::ES_make_tcp_synack_from_syn(uint8_t *buf) {
	make_tcp_synack_from_syn(buf);
}

void EtherShield::ES_make_tcp_ack_from_any(uint8_t *buf,int16_t datlentoack,uint8_t addflags ) {
	void make_tcp_ack_from_any(uint8_t *buf,int16_t datlentoack,uint8_t addflags );
}

void EtherShield::ES_make_tcp_ack_with_data(uint8_t *buf,uint16_t dlen ) {
	make_tcp_ack_with_data(buf, dlen);
}
		
void EtherShield::ES_make_tcp_ack_with_data_noflags(uint8_t *buf,uint16_t dlen ) {
	make_tcp_ack_with_data_noflags(buf, dlen);
}

uint16_t EtherShield::ES_build_tcp_data(uint8_t *buf, uint16_t srcPort ) {
	return build_tcp_data( buf, srcPort );
}

void EtherShield::ES_send_tcp_data(uint8_t *buf,uint16_t dlen ) {
	send_tcp_data(buf, dlen);
}

void EtherShield::ES_send_udp_data(uint8_t *buf, uint8_t *destmac,uint16_t dlen,uint16_t source_port, uint8_t *dest_ip, uint16_t dest_port) {
	send_udp_prepare(buf,source_port, dest_ip, dest_port);
	for(uint8_t i = 0; i< 6; i++ )
		buf[ETH_DST_MAC+i] = destmac[i];
	send_udp_transmit(buf,dlen);
}

void EtherShield::ES_send_udp_data(uint8_t *buf,uint16_t dlen,uint16_t source_port, uint8_t *dest_ip, uint16_t dest_port) {
	send_udp_prepare(buf,source_port, dest_ip, dest_port);
	send_udp_transmit(buf,dlen);
}

void EtherShield::ES_init_len_info(uint8_t *buf) {
	init_len_info(buf);
}

void EtherShield::ES_fill_buf_p(uint8_t *buf,uint16_t len, const prog_char *progmem_s) {
	fill_buf_p(buf, len, progmem_s);
}

uint16_t EtherShield::ES_checksum(uint8_t *buf, uint16_t len,uint8_t type) {
	return checksum(buf, len, type);

}

void EtherShield::ES_fill_ip_hdr_checksum(uint8_t *buf) {
	fill_ip_hdr_checksum(buf);
}

uint16_t EtherShield::ES_get_tcp_data_pointer(void) {
	return get_tcp_data_pointer();
}

uint16_t EtherShield::ES_packetloop_icmp_tcp(uint8_t *buf,uint16_t plen) {
	return packetloop_icmp_tcp(buf,plen);
}

uint16_t EtherShield::ES_fill_tcp_data_p(uint8_t *buf,uint16_t pos, const prog_char *progmem_s){
	return fill_tcp_data_p(buf, pos, progmem_s);
}

uint16_t EtherShield::ES_fill_tcp_data(uint8_t *buf,uint16_t pos, const char *s){
	return fill_tcp_data(buf,pos, s);
}

uint16_t EtherShield::ES_fill_tcp_data_len(uint8_t *buf,uint16_t pos, const char *s, uint16_t len ){
	return fill_tcp_data_len(buf,pos, s, len);
}

void EtherShield::ES_www_server_reply(uint8_t *buf,uint16_t dlen) {
	www_server_reply(buf,dlen);
}
	
uint8_t EtherShield::ES_client_store_gw_mac(uint8_t *buf) {
	return client_store_gw_mac(buf);
}

void EtherShield::ES_client_set_gwip(uint8_t *gwipaddr) {
	client_set_gwip(gwipaddr);
}

void EtherShield::ES_client_set_wwwip(uint8_t *wwwipaddr) {
	//client_set_wwwip(wwwipaddr);
	client_tcp_set_serverip(wwwipaddr);
}

void EtherShield::ES_client_tcp_set_serverip(uint8_t *ipaddr) {
	client_tcp_set_serverip(ipaddr);
}

void EtherShield::ES_client_arp_whohas(uint8_t *buf,uint8_t *ip_we_search) {
	client_arp_whohas(buf, ip_we_search);
}

#if defined (TCP_client) || defined (WWW_client) || defined (NTP_client)
uint8_t EtherShield::ES_client_tcp_req(uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),uint16_t (*datafill_callback)(uint8_t fd),uint16_t port ) {
	return client_tcp_req( result_callback, datafill_callback, port );
}

void EtherShield::ES_tcp_client_send_packet(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t max_segment_size, 
	uint8_t clear_seqck, uint16_t next_ack_num, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip){
	
	tcp_client_send_packet(buf, dest_port, src_port, flags, max_segment_size, clear_seqck, next_ack_num, dlength,dest_mac,dest_ip);
}

uint16_t EtherShield::ES_tcp_get_dlength( uint8_t *buf ){
	return tcp_get_dlength(buf);
}

#endif		// TCP_client WWW_Client etc

#ifdef WWW_client

// ----- http get

#ifdef FLASH_VARS
void EtherShield::ES_client_browse_url(prog_char *urlbuf, char *urlbuf_varpart, prog_char *hoststr,
		void (*callback)(uint8_t,uint16_t,uint16_t)) {
	client_browse_url(urlbuf, urlbuf_varpart, hoststr, callback);
}
#else
void EtherShield::ES_client_browse_url(char *urlbuf, char *urlbuf_varpart, char *hoststr,
		void (*callback)(uint8_t,uint16_t,uint16_t)) {
	client_browse_url(urlbuf, urlbuf_varpart, hoststr, callback);
}
#endif		// FLASH_VARS

#ifdef FLASH_VARS
void EtherShield::ES_client_http_post(prog_char *urlbuf, prog_char *hoststr, prog_char *additionalheaderline,
		prog_char *method, char *postval,void (*callback)(uint8_t,uint16_t)) {
	client_http_post(urlbuf, hoststr, additionalheaderline, method, postval,callback);
}
#else
void EtherShield::ES_client_http_post(char *urlbuf, char *hoststr, char *additionalheaderline,
		char *method, char *postval,void (*callback)(uint8_t,uint16_t)) {
	client_http_post(urlbuf, hoststr, additionalheaderline, method, postval,callback);
}
#endif		// FLASH_VARS

#endif		// WWW_client

#ifdef NTP_client
void EtherShield::ES_client_ntp_request(uint8_t *buf,uint8_t *ntpip,uint8_t srcport) {
	client_ntp_request(buf,ntpip,srcport);
}

uint8_t EtherShield::ES_client_ntp_process_answer(uint8_t *buf,uint32_t *time,uint8_t dstport_l) {
	return client_ntp_process_answer(buf,time,dstport_l);
}
#endif		// NTP_client

void EtherShield::ES_register_ping_rec_callback(void (*callback)(uint8_t *srcip)) {
	register_ping_rec_callback(callback);
}

#ifdef PING_client
void EtherShield::ES_client_icmp_request(uint8_t *buf,uint8_t *destip) {
	client_icmp_request(buf,destip);
}

uint8_t EtherShield::ES_packetloop_icmp_checkreply(uint8_t *buf,uint8_t *ip_monitoredhost) {
	return packetloop_icmp_checkreply(buf,ip_monitoredhost);
}
#endif // PING_client

#ifdef WOL_client
void EtherShield::ES_send_wol(uint8_t *buf,uint8_t *wolmac) {
	send_wol(buf,wolmac);
}
#endif // WOL_client


#ifdef FROMDECODE_websrv_help
uint8_t EtherShield::ES_find_key_val(char *str,char *strbuf, uint16_t maxlen,char *key) {
	return find_key_val(str,strbuf, maxlen,key);
}

void EtherShield::ES_urldecode(char *urlbuf) {
	urldecode(urlbuf);
}
#endif


#ifdef URLENCODE_websrv_help
void EtherShield::ES_urlencode(char *str,char *urlbuf) {
	urlencode(str,urlbuf);
}
#endif

uint8_t EtherShield::ES_parse_ip(uint8_t *bytestr,char *str) {
	return parse_ip(bytestr,str);
}

void EtherShield::ES_mk_net_str(char *resultstr,uint8_t *bytestr,uint16_t len,char separator,uint8_t base) {
	mk_net_str(resultstr,bytestr,len,separator,base);
}

uint8_t EtherShield::ES_client_waiting_gw() {
	return( client_waiting_gw() );
}

#ifdef DNS_client
uint8_t EtherShield::ES_dnslkup_haveanswer(void)
{       
        return( dnslkup_haveanswer() );
}

uint8_t EtherShield::ES_dnslkup_get_error_info(void)
{       
        return( dnslkup_get_error_info() );
}

uint8_t * EtherShield::ES_dnslkup_getip(void)
{       
        return(dnslkup_getip() );
}

void EtherShield::ES_dnslkup_set_dnsip(uint8_t *dnsipaddr) {
	dnslkup_set_dnsip(dnsipaddr);
}

void EtherShield::ES_dnslkup_request(uint8_t *buf,uint8_t *hostname) {
	return( dnslkup_request(buf, hostname) );
}

uint8_t EtherShield::ES_udp_client_check_for_dns_answer(uint8_t *buf,uint16_t plen){
	return( udp_client_check_for_dns_answer( buf, plen) );
}


// Perform all processing to resolve a hostname to IP address.
// Returns 1 for successful Name resolution, 0 otherwise
uint8_t EtherShield::resolveHostname(uint8_t *buf, uint16_t buffer_size, uint8_t *hostname ) {
  uint16_t dat_p;
  int plen = 0;
  long lastDnsRequest = millis();
  uint8_t dns_state = DNS_STATE_INIT;
  boolean gotAddress = false;
  uint8_t dnsTries = 3;	// After 10 attempts fail gracefully so other action can be carried out

  while( !gotAddress ) {
    // handle ping and wait for a tcp packet
    plen = enc28j60PacketReceive(buffer_size, buf);
    dat_p=packetloop_icmp_tcp(buf,plen);

    // We have a packet
    // Check if IP data
    if (dat_p == 0) {
      if (client_waiting_gw() ) {
        // No ARP received for gateway
        continue;
      }
      // It has IP data
      if (dns_state==DNS_STATE_INIT) {
        dns_state=DNS_STATE_REQUESTED;
        lastDnsRequest = millis();
        dnslkup_request(buf,hostname);
        continue;
      }
      if (dns_state!=DNS_STATE_ANSWER){
        // retry every minute if dns-lookup failed:
        if (millis() > (lastDnsRequest + 60000L) ){
	  if( --dnsTries <= 0 ) 
	    return 0;		// Failed to allocate address

          dns_state=DNS_STATE_INIT;
          lastDnsRequest = millis();
        }
        // don't try to use client before
        // we have a result of dns-lookup

        continue;
      }
    } 
    else {
      if (dns_state==DNS_STATE_REQUESTED && udp_client_check_for_dns_answer( buf, plen ) ){
        dns_state=DNS_STATE_ANSWER;
        //client_set_wwwip(dnslkup_getip());
        client_tcp_set_serverip(dnslkup_getip());
	gotAddress = true;
      }
    }
  }
  
  return 1;
}

#endif		// DNS_client

#ifdef DHCP_client
void EtherShield::ES_dhcp_start(uint8_t *buf, uint8_t *macaddrin, uint8_t *ipaddrin,
     uint8_t *maskin, uint8_t *gwipin, uint8_t *dhcpsvrin, uint8_t *dnssvrin ) {
	dhcp_start(buf, macaddrin, ipaddrin, maskin, gwipin, dhcpsvrin, dnssvrin );
}
uint8_t EtherShield::ES_dhcp_state(void)
{       
        return( dhcp_state() );
}

uint8_t EtherShield::ES_check_for_dhcp_answer(uint8_t *buf,uint16_t plen){
	return( check_for_dhcp_answer( buf, plen) );
}

// Utility functions 

// Perform all processing to get an IP address plus other addresses returned, e.g. gw, dns, dhcp server.
// Returns 1 for successful IP address allocation, 0 otherwise
uint8_t EtherShield::allocateIPAddress(uint8_t *buf, uint16_t buffer_size, uint8_t *mymac, uint16_t myport, uint8_t *myip, uint8_t *mynetmask, uint8_t *gwip, uint8_t *dnsip, uint8_t *dhcpsvrip ) {
  uint16_t dat_p;
  int plen = 0;
  long lastDhcpRequest = millis();
  uint8_t dhcpState = 0;
  boolean gotIp = false;
  uint8_t dhcpTries = 10;	// After 10 attempts fail gracefully so other action can be carried out

  dhcp_start( buf, mymac, myip, mynetmask,gwip, dnsip, dhcpsvrip );

  while( !gotIp ) {
    // handle ping and wait for a tcp packet
    plen = enc28j60PacketReceive(buffer_size, buf);
    dat_p=packetloop_icmp_tcp(buf,plen);
    if(dat_p==0) {
      int retstat = check_for_dhcp_answer( buf, plen);
      dhcpState = dhcp_state();
      // we are idle here
      if( dhcpState != DHCP_STATE_OK ) {
        if (millis() > (lastDhcpRequest + 10000L) ){
          lastDhcpRequest = millis();
	  if( --dhcpTries <= 0 ) 
		  return 0;		// Failed to allocate address
          // send dhcp
          dhcp_start( buf, mymac, myip, mynetmask,gwip, dnsip, dhcpsvrip );
        }
      } else {
        if( !gotIp ) {
          gotIp = true;

          //init the ethernet/ip layer:
          init_ip_arp_udp_tcp(mymac, myip, myport);

          // Set the Router IP
          client_set_gwip(gwip);  // e.g internal IP of dsl router

          // Set the DNS server IP address if required, or use default
          dnslkup_set_dnsip( dnsip );

        }
      }
    }
  }

  return 1;

}

#endif		// DHCP_client

void EtherShield::ES_enc28j60PowerUp(){
 enc28j60PowerUp();
}

void EtherShield::ES_enc28j60PowerDown(){
 enc28j60PowerDown();
}


// TCP functions broken out here for testing
uint8_t EtherShield::ES_nextTcpState( uint8_t *buf, uint16_t plen ) {
	return nextTcpState(buf, plen );
}

uint8_t EtherShield::ES_currentTcpState( ) {
	return currentTcpState( );
}

uint8_t EtherShield::ES_tcpActiveOpen( uint8_t *buf,uint16_t plen,
       uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),
       uint16_t (*datafill_callback)(uint8_t fd),
       uint16_t port ) {
	return tcpActiveOpen(buf, plen, result_callback, datafill_callback, port );
}

void EtherShield::ES_tcpPassiveOpen( uint8_t *buf,uint16_t plen ) {
	tcpPassiveOpen(buf, plen );
}

void EtherShield::ES_tcpClose( uint8_t *buf,uint16_t plen ) {
	tcpClose(buf, plen );
}

// End
