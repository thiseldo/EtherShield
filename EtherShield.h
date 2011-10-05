/*
  EHTERSHIELD_H library for Arduino etherShield
  Copyright (c) 2008 Xing Yu.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ETHERSHIELD_H
#define ETHERSHIELD_H

#include <inttypes.h>
#include "ip_config.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"
#include "net.h"

class EtherShield
{
  public:
  
    EtherShield();

	void ES_enc28j60SpiInit( void );
	void ES_enc28j60Init( uint8_t* macaddr);
	void ES_enc28j60Init( uint8_t* macaddr, uint8_t csPin );
	void ES_enc28j60clkout(uint8_t clk);
	uint8_t ES_enc28j60linkup(void);
	void ES_enc28j60PhyWrite(uint8_t address, uint16_t data);
	uint16_t ES_enc28j60PacketReceive(uint16_t len, uint8_t* packet);
	void ES_enc28j60PacketSend(uint16_t len, uint8_t* packet);
	uint8_t ES_enc28j60Revision(void);
	uint8_t ES_enc28j60Read( uint8_t address );
	void ES_enc28j60EnableBroadcast( void );
	void ES_enc28j60DisableBroadcast( void );
	void ES_enc28j60EnableMulticast( void );
	void ES_enc28j60DisableMulticast( void );
	void ES_enc28j60PowerUp();
	void ES_enc28j60PowerDown();   

	void ES_init_ip_arp_udp_tcp(uint8_t *mymac,uint8_t *myip,uint16_t port);
	// for a UDP server:
	uint8_t ES_eth_type_is_arp_and_my_ip(uint8_t *buf,uint16_t len);
	uint8_t ES_eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len);

	void ES_make_echo_reply_from_request(uint8_t *buf,uint16_t len);
	void ES_make_tcp_synack_from_syn(uint8_t *buf);
	void ES_make_tcp_ack_from_any(uint8_t *buf,int16_t datlentoack,uint8_t addflags);
	void ES_make_tcp_ack_with_data(uint8_t *buf,uint16_t dlen);
	void ES_make_tcp_ack_with_data_noflags(uint8_t *buf,uint16_t dlen);
	void ES_init_len_info(uint8_t *buf);
	uint16_t ES_get_tcp_data_pointer(void);
	uint16_t ES_build_tcp_data(uint8_t *buf, uint16_t srcPort );
	void ES_send_tcp_data(uint8_t *buf,uint16_t dlen );

	// UDP - dirkx
	void ES_send_udp_data(uint8_t *buf,uint16_t dlen,uint16_t source_port, uint8_t *dest_ip, uint16_t dest_port);
	void ES_send_udp_data(uint8_t *buf, uint8_t *destmac,uint16_t dlen,uint16_t source_port, uint8_t *dest_ip, uint16_t dest_port);
	
	void ES_fill_buf_p(uint8_t *buf,uint16_t len, const prog_char *progmem_s);
	uint16_t ES_checksum(uint8_t *buf, uint16_t len,uint8_t type);
	void ES_fill_ip_hdr_checksum(uint8_t *buf);

	// return 0 to just continue in the packet loop and return the position 
	// of the tcp data if there is tcp data part
	uint16_t ES_packetloop_icmp_tcp(uint8_t *buf,uint16_t plen);
	// functions to fill the web pages with data:
	uint16_t ES_fill_tcp_data_p(uint8_t *buf,uint16_t pos, const prog_char *progmem_s);
	uint16_t ES_fill_tcp_data(uint8_t *buf,uint16_t pos, const char *s);
	uint16_t ES_fill_tcp_data_len(uint8_t *buf,uint16_t pos, const char *s, uint16_t len );
	// send data from the web server to the client:
	void ES_www_server_reply(uint8_t *buf,uint16_t dlen);
	
	// -- client functions --
	uint8_t ES_client_store_gw_mac(uint8_t *buf);	//, uint8_t *gwipaddr);
	void ES_client_set_gwip(uint8_t *gwipaddr);
	void ES_client_set_wwwip(uint8_t *wwwipaddr);
	void ES_client_tcp_set_serverip(uint8_t *ipaddr);
	void ES_client_arp_whohas(uint8_t *buf,uint8_t *ip_we_search);
	uint8_t ES_client_waiting_gw( void );

#if defined (TCP_client) || defined (WWW_client) || defined (NTP_client)
	uint8_t ES_client_tcp_req(uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),uint16_t (*datafill_callback)(uint8_t fd),uint16_t port );

	void ES_tcp_client_send_packet(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t max_segment_size, 
		uint8_t clear_seqck, uint16_t next_ack_num, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip);
	uint16_t ES_tcp_get_dlength( uint8_t *buf );

#endif
#ifdef DNS_client
	uint8_t ES_dnslkup_haveanswer(void);
	uint8_t ES_dnslkup_get_error_info(void);
	uint8_t *ES_dnslkup_getip( void );
	void ES_dnslkup_set_dnsip(uint8_t *dnsipaddr);
	void ES_dnslkup_request(uint8_t *buf, uint8_t *hoststr );
	uint8_t ES_udp_client_check_for_dns_answer(uint8_t *buf,uint16_t plen);
	uint8_t resolveHostname(uint8_t *buf, uint16_t buffer_size, uint8_t *hostname );
#endif

#ifdef DHCP_client
	uint8_t ES_dhcp_state(void);
	void ES_dhcp_start(uint8_t *buf, uint8_t *macaddrin, uint8_t *ipaddrin,
			uint8_t *maskin, uint8_t *gwipin, uint8_t *dhcpsvrin,
			uint8_t *dnssvrin );

	uint8_t ES_check_for_dhcp_answer(uint8_t *buf,uint16_t plen);
	uint8_t allocateIPAddress(uint8_t *buf, uint16_t buffer_size, uint8_t *mymac, uint16_t myport, uint8_t *myip, uint8_t *mynetmask, uint8_t *gwip, uint8_t *dnsip, uint8_t *dhcpsvrip );
#endif

#define HTTP_HEADER_START ((uint16_t)TCP_SRC_PORT_H_P+(buf[TCP_HEADER_LEN_P]>>4)*4)
#ifdef WWW_client
	// ----- http get
#ifdef FLASH_VARS
	void ES_client_browse_url(prog_char *urlbuf, char *urlbuf_varpart, prog_char *hoststr,
			void (*callback)(uint8_t,uint16_t,uint16_t));
#else
	void ES_client_browse_url(char *urlbuf, char *urlbuf_varpart, char *hoststr,
			void (*callback)(uint8_t,uint16_t,uint16_t));
#endif
	// The callback is a reference to a function which must look like this:
	// void browserresult_callback(uint8_t statuscode,uint16_t datapos)
	// statuscode=0 means a good webpage was received, with http code 200 OK
	// statuscode=1 an http error was received
	// statuscode=2 means the other side in not a web server and in this case datapos is also zero
	// ----- http post 
	// client web browser using http POST operation:
	// additionalheaderline must be set to NULL if not used.
	// method should be used if default POST to be overridden set NULL for dfault
	// postval is a string buffer which can only be de-allocated by the caller 
	// when the post operation was really done (e.g when callback was executed).
	// postval must be urlencoded.
#ifdef FLASH_VARS
	void ES_client_http_post(prog_char *urlbuf, prog_char *hoststr, prog_char *additionalheaderline,
			prog_char *method, char *postval,void (*callback)(uint8_t,uint16_t));
#else
	void ES_client_http_post(char *urlbuf, char *hoststr, char *additionalheaderline,
			char *method, char *postval,void (*callback)(uint8_t,uint16_t));
#endif
	// The callback is a reference to a function which must look like this:
	// void browserresult_callback(uint8_t statuscode,uint16_t datapos)
	// statuscode=0 means a good webpage was received, with http code 200 OK
	// statuscode=1 an http error was received
	// statuscode=2 means the other side in not a web server and in this case datapos is also zero
#endif		// WWW_client

#ifdef NTP_client
	void ES_client_ntp_request(uint8_t *buf,uint8_t *ntpip,uint8_t srcport);
	uint8_t ES_client_ntp_process_answer(uint8_t *buf,uint32_t *time,uint8_t dstport_l);
#endif		// NTP_client

	// you can find out who ping-ed you if you want:
	void ES_register_ping_rec_callback(void (*callback)(uint8_t *srcip));

#ifdef PING_client
	void ES_client_icmp_request(uint8_t *buf,uint8_t *destip);
	// you must loop over this function to check if there was a ping reply:
	uint8_t ES_packetloop_icmp_checkreply(uint8_t *buf,uint8_t *ip_monitoredhost);
#endif // PING_client

#ifdef WOL_client
	void ES_send_wol(uint8_t *buf,uint8_t *wolmac);
#endif // WOL_client

#ifdef FROMDECODE_websrv_help
	uint8_t ES_find_key_val(char *str,char *strbuf, uint16_t maxlen,char *key);
	void ES_urldecode(char *urlbuf);
#endif	// FROMDECODE_websrv_help

#ifdef URLENCODE_websrv_help
	void ES_urlencode(char *str,char *urlbuf);
#endif	// URLENCODE_websrv_help

	uint8_t ES_parse_ip(uint8_t *bytestr,char *str);
	void ES_mk_net_str(char *resultstr,uint8_t *bytestr,uint16_t len,char separator,uint8_t base);


	uint8_t ES_nextTcpState( uint8_t *buf,uint16_t plen );
	uint8_t ES_currentTcpState( );
	uint8_t ES_tcpActiveOpen( uint8_t *buf,uint16_t plen, uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),uint16_t (*datafill_callback)(uint8_t fd),uint16_t port );
	void ES_tcpPassiveOpen( uint8_t *buf,uint16_t plen );
	void ES_tcpClose( uint8_t *buf,uint16_t plen );

};

#endif

