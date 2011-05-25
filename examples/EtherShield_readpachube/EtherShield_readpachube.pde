/*
 * Read a pachube data stream and report results as csv
 */
#include <EtherShield.h>

static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x25};
static uint8_t myip[4] = {192,168,1,25};
// Default gateway. The ip address of your DSL router. It can be set to the same as
// websrvip the case where there is no default GW to access the
// web server (=web server is on the same lan as this host)
static uint8_t gwip[4] = {192,168,1,1};

//============================================================================================================
// Pachube declarations
//============================================================================================================
#define PORT 80                   // HTTP

// the etherShield library does not really support sending additional info in a get request
// here we fudge it in the host field to add the API key
// Http header is
// Host: <HOSTNAME>
// X-PachubeApiKey: xxxxxxxx
// User-Agent: Arduino/1.0
// Accept: text/html
#define HOSTNAME "www.pachube.com\r\nX-PachubeApiKey: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"      // API key
static uint8_t websrvip[4] = { 0,0,0,0 };	// Get pachube ip by DNS call
#define WEBSERVER_VHOST "www.pachube.com"
#define HTTPPATH "/api/0000.csv"      // Set your own feed ID here

static uint8_t resend=0;
static int8_t dns_state=0;

EtherShield es=EtherShield();

#define BUFFER_SIZE 550
static uint8_t buf[BUFFER_SIZE+1];

void browserresult_callback(uint8_t statuscode,uint16_t datapos){
  if (datapos != 0)
  {
    // now search for the csv data - it follows the first blank line
    // I'm sure that there is an easier way to search for a blank line - but I threw this together quickly
    // and it works for me.
    uint16_t pos = datapos;
    while (buf[pos])    // loop until end of buffer (or we break out having found what we wanted)
    {
      while (buf[pos]) if (buf[pos++] == '\n') break;   // find the first line feed
      if (buf[pos] == 0) break; // run out of buffer
      if (buf[pos++] == '\r') break; // if it is followed by a carriage return then it is a blank line (\r\n\r\n)
    }
    if (buf[pos])  // we didn't run out of buffer
    {
      pos++;  //skip over the '\n' remaining
      Serial.println((char*)&buf[pos]);
    }
  }
}

void setup(){
  Serial.begin(9600);
#ifdef FLASH_VARS
  Serial.println("Ethershield Pachube Read example, using Flash");
#else
  Serial.println("Ethershield Pachube Read example using ram");
#endif

  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize ENC28J60
  es.ES_enc28j60Init(mymac);

  //init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac, myip, PORT);

  // init the web client:
  es.ES_client_set_gwip(gwip);  // e.g internal IP of dsl router

}

void loop()
{
  static uint32_t timetosend;
  uint16_t dat_p;
  int sec = 0;
  long lastDnsRequest = 0L;
  int plen = 0;

  dns_state=0;

  while(1) {
    // handle ping and wait for a tcp packet - calling this routine powers the sending and receiving of data
    plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
    dat_p=es.ES_packetloop_icmp_tcp(buf,plen);
    if( plen > 0 ) {
      // We have a packet
      // Check if IP data
      if (dat_p == 0) {
        if (es.ES_client_waiting_gw() ){
          // No ARP received for gateway
          continue;
        }
        // It has IP data
        if (dns_state==0){
          sec=0;
          dns_state=1;
          lastDnsRequest = millis();
          es.ES_dnslkup_request(buf,(uint8_t*)WEBSERVER_VHOST);
          continue;
        }
        if (dns_state==1 && es.ES_udp_client_check_for_dns_answer( buf, plen ) ){
          dns_state=2;
          es.ES_client_set_wwwip(es.ES_dnslkup_getip());
        }
        if (dns_state!=2){
          // retry every minute if dns-lookup failed:
          if (millis() > (lastDnsRequest + 60000L) ){
            dns_state=0;
            lastDnsRequest = millis();
          }
          // don't try to use web client before
          // we have a result of dns-lookup
          continue;
        }
      } else {
        if (dns_state==1 && es.ES_udp_client_check_for_dns_answer( buf, plen ) ){
          dns_state=2;
          es.ES_client_set_wwwip(es.ES_dnslkup_getip());
        }
      }
    }
    // If we have IP address for server and its time then request data

    if( dns_state == 2 && millis() - timetosend > 10000)  // every 10 seconds
    {
      timetosend = millis();
#ifdef FLASH_VARS
      // note the use of PSTR - this puts the string into code space and is compulsory in this call
      // second parameter is a variable string to append to HTTPPATH, this string is NOT a PSTR
      es.ES_client_browse_url(PSTR(HTTPPATH), NULL, PSTR(HOSTNAME), &browserresult_callback);
#else
      es.ES_client_browse_url(HTTPPATH, NULL, HOSTNAME, &browserresult_callback);
#endif
    }
  }
}


