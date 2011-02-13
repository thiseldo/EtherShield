/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher 
 * Copyright: GPL V2
 *
 * DNS look-up functions based on the udp client
 *
 * Chip type           : ATMEGA88/ATMEGA168/ATMEGA328p with ENC28J60
 *********************************************/
//@{
#ifndef DNSLKUP_H
#define DNSLKUP_H 1

// to use this you need to enable UDP_client in the file ip_config.h
//
#if defined (UDP_client)

// look-up a hostname (you should check client_waiting_gw() before calling this function):
extern void dnslkup_request(uint8_t *buf, uint8_t *progmem_hostname);
//extern void dnslkup_request(uint8_t *buf,const prog_char *progmem_hostname);
// returns 1 if we have an answer from an DNS server and an IP
extern uint8_t dnslkup_haveanswer(void);
// get information about any error (zero means no error, otherwise see dnslkup.c)
extern uint8_t dnslkup_get_error_info(void);
// loop over this function to search for the answer of the
// DNS server.
// You call this function when enc28j60PacketReceive returned non
// zero and packetloop_icmp_tcp did return zero.
uint8_t udp_client_check_for_dns_answer(uint8_t *buf,uint16_t plen);
// returns the host IP of the name that we looked up if dnslkup_haveanswer did return 1
extern uint8_t *dnslkup_getip(void);

// set DNS server to be used for lookups.
extern void dnslkup_set_dnsip(uint8_t *dnsipaddr);

#endif /* UDP_client */
#endif /* DNSLKUP_H */
//@}
