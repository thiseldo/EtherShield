/*
 * Arduino ENC28J60 Ethernet shield Wake-On-Lan client
 * This is just an example client that waits 5s then sends
 * the magic packet out every 60 seconds.
 * In practice, this is only really needs to be sent once.
 *
 */
#include "EtherShield.h"

// Please modify the following lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
static uint8_t mymac[6] = {
  0x54,0x55,0x58,0x10,0x00,0x25};

// The arduinos IP address
static uint8_t myip[4] = {
  192,168,1,25};

// The mac address of the PC you want to wake u
static uint8_t wolmac[6] = {
  0x01,0x23,0x45,0x67,0x89,0xab};

// Packet buffer, must be big enough to packet and payload
#define BUFFER_SIZE 150
static uint8_t buf[BUFFER_SIZE+1];

EtherShield es=EtherShield();
uint32_t lastUpdate = 0;

void setup(){

  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize enc28j60
  es.ES_enc28j60Init(mymac);

  //init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, 80);

  lastUpdate = millis();

  delay(5000);
}

void loop(){
  uint16_t dat_p;

  while(1){
    // handle ping and wait for a tcp packet
    dat_p=es.ES_packetloop_icmp_tcp(buf,es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));

    if( lastUpdate < (millis() - 60000) ) {
      es.ES_send_wol( buf, wolmac );
      lastUpdate = millis();
    }
  }
}

// End

