/*
 * Arduino ENC28J60 Ethernet shield UDP broadcast client
 */
#include "EtherShield.h"

// Note: This software implements a web server and a web browser.
// The web server is at "myip" 
// 
// Please modify the following lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
static uint8_t mymac[6] = {
  0x54,0x55,0x58,0x10,0x00,0x25};

static uint8_t myip[4] = {
  192,168,1,25};

static uint8_t broadcastip[4] = {
  192,168,1,255};

// Port 52240
#define DEST_PORT_L  0x10
#define DEST_PORT_H  0xCC

const char iphdr[] PROGMEM ={
  0x45,0,0,0x82,0,0,0x40,0,0x20}; // 0x82 is the total

struct UDPPayload {
  uint32_t time;            // Time
  uint16_t temperature;     // Temp in 1/10 degree
  uint16_t data[10];        //watts data
  uint16_t errorCount;      // count of errors in the XML.
  uint16_t timeout_count;   // count of all protocol lockups
};

UDPPayload udpPayload;
uint8_t  srcport = 11023;

// Packet buffer, must be big enough to packet and payload
#define BUFFER_SIZE 150
static uint8_t buf[BUFFER_SIZE+1];

EtherShield es=EtherShield();
uint32_t lastUpdate = 0;

void setup(){

  // Initialise data in the payload structure
  for( int i=0; i<10; i++ ) {
    udpPayload.data[i] = i;
  }
  udpPayload.time = millis();
  udpPayload.temperature = 0;

  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize ENC28J60
  es.ES_enc28j60Init(mymac);

  //init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, 80);

  lastUpdate = millis();

  delay(10000);
}

void loop(){
  uint16_t dat_p;

  while(1){
    // handle ping and wait for a tcp packet
    dat_p=es.ES_packetloop_icmp_tcp(buf,es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));

    if( lastUpdate < (millis() - 10000) ) {
      // time to send broadcast
      // Update the data to make it different each time
      udpPayload.time = millis();
      udpPayload.temperature++;
      for( int i=0; i<10; i++ ) {
        udpPayload.data[i]++;
      }
      broadcastData() ;
      lastUpdate = millis();
    }
  }
}

// Broadcast the data in the udpPayload structure
void broadcastData( void ) {
  uint8_t i=0;
  uint16_t ck;
  // Setup the MAC addresses for ethernet header
  while(i<6){
    buf[ETH_DST_MAC +i]= 0xff; // Broadcsat address
    buf[ETH_SRC_MAC +i]=mymac[i];
    i++;
  }
  buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
  buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
  es.ES_fill_buf_p(&buf[IP_P],9,iphdr);

  // IP Header
  buf[IP_TOTLEN_L_P]=28+sizeof(UDPPayload);
  buf[IP_PROTO_P]=IP_PROTO_UDP_V;
  i=0;
  while(i<4){
    buf[IP_DST_P+i]=broadcastip[i];
    buf[IP_SRC_P+i]=myip[i];
    i++;
  }
  es.ES_fill_ip_hdr_checksum(buf);
  buf[UDP_DST_PORT_H_P]=DEST_PORT_H;
  buf[UDP_DST_PORT_L_P]=DEST_PORT_L;
  buf[UDP_SRC_PORT_H_P]=10;
  buf[UDP_SRC_PORT_L_P]=srcport; // lower 8 bit of src port
  buf[UDP_LEN_H_P]=0;
  buf[UDP_LEN_L_P]=8+sizeof(UDPPayload); // fixed len
  // zero the checksum
  buf[UDP_CHECKSUM_H_P]=0;
  buf[UDP_CHECKSUM_L_P]=0;
  // copy the data:
  i=0;
  // most fields are zero, here we zero everything and fill later
  uint8_t* b = (uint8_t*)&udpPayload;
  while(i< sizeof( UDPPayload ) ){ 
    buf[UDP_DATA_P+i]=*b++;
    i++;
  }
  // Create correct checksum
  ck=es.ES_checksum(&buf[IP_SRC_P], 16 + sizeof( UDPPayload ),1);
  buf[UDP_CHECKSUM_H_P]=ck>>8;
  buf[UDP_CHECKSUM_L_P]=ck& 0xff;
  es.ES_enc28j60PacketSend(42 + sizeof( UDPPayload ), buf);
}

// End

