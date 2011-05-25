/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: 
 * Copyright: GPL V2
 *
 * DHCP look-up functions based on the udp client
 *
 * Chip type : ATMEGA88/ATMEGA168/ATMEGA328p with ENC28J60
 *********************************************/
//@{
#ifndef DHCP_H
#define DHCP_H 1

// to use this you need to enable UDP_client in the file ip_config.h
#if defined (UDP_client)

// Some defines

#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTRESPONSE 2

extern void dhcp_start(uint8_t *buf, uint8_t *macaddrin, uint8_t *ipaddrin,
                uint8_t *maskin, uint8_t *gwipin, uint8_t *dhcpsvrin,
                uint8_t *dnssvrin );

extern uint8_t dhcp_state( void );

uint8_t check_for_dhcp_answer(uint8_t *buf,uint16_t plen);

uint8_t have_dhcpoffer(uint8_t *buf,uint16_t plen);
uint8_t have_dhcpack(uint8_t *buf,uint16_t plen);

#endif /* UDP_client */
#endif /* DHCP_H */
//@}
