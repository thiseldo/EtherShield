/**
 * ENC28J60 Ethershield DHCP and Pachube demo - Pachube RGB
 * This reads a number of Pachube datastreams and
 * converts the output to colours on a RGB LED.
 * The original demo uses a Pachube app with 3 rotary controls
 * that select the value for each of the Red, Green and Blue colours.
 * The example uses DHCP to determin the local IP address, gateway,
 * and DNS server.
 * 
 * As a debugging aid, the RGB LED initialy glows red.
 * Once the ENC28J60 has been initialised it changes to Orange
 * When the IP address has been allocated it changes to Green.
 * It then changes to the set colour once the values are being
 * read from the Pachube application.
 *
 * Written By and (c) Andrew D Lindsay, May 2011
 * http://blog.thiseldo.co.uk
 * Feel free to use this code, modify and redistribute.
 * 
 */

#include <EtherShield.h>

#define DEBUG

// If using a Common Anode RGB LED (i.e. has connection to +5V
// Then leave this uncommented this
#define COMMON_ANODE
// If using Common Cathode RGB LED (i.e. has common connection to GND)
// then comment out the above line or change to:
//#undef COMMON_ANODE

#ifdef COMMON_ANODE
#define LOW_LIMIT 255
#define HIGH_LIMIT 0
#else
#define LOW_LIMIT 0
#define HIGH_LIMIT 255
#endif

// Define where the RGB LED is connected - this is a common cathode LED.
#define BLUEPIN  3  // Blue LED,  connected to digital pin 3
#define REDPIN   5  // Red LED,   connected to digital pin 5
#define GREENPIN 6  // Green LED, connected to digital pin 6

// Currently the only value on the network side that needs to be set
static uint8_t mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x25};

// IP and netmask allocated by DHCP
static uint8_t myip[4] = { 0,0,0,0 };
static uint8_t mynetmask[4] = { 0,0,0,0 };

// IP address of the host being queried to contact (IP of the first portion of the URL):
static uint8_t websrvip[4] = { 0, 0, 0, 0 };

// Default gateway, dns server and dhcp server. 
// These are found using DHCP
static uint8_t gwip[4] = { 0,0,0,0 };
static uint8_t dnsip[4] = { 0,0,0,0 };
static uint8_t dhcpsvrip[4] = { 0,0,0,0 };

int currentRed = 0;
int currentGreen = 0;
int currentBlue = 0;

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
#define HOSTNAME "www.pachube.com\r\nX-PachubeApiKey: XXXXXXXXXX"          // my API key
#define WEBSERVER_VHOST "www.pachube.com"
#define HTTPPATH "/api/NNNNN.csv"      // The feed - use API V1 csv

static uint8_t resend=0;
static int8_t dns_state=DNS_STATE_INIT;

EtherShield es=EtherShield();

#define BUFFER_SIZE 750
static uint8_t buf[BUFFER_SIZE+1];

void browserresult_callback(uint8_t statuscode,uint16_t datapos){
#ifdef DEBUG
  Serial.print("Received data, status:"); 
  Serial.println(statuscode,DEC);
#endif
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
#ifdef DEBUG
      Serial.println((char*)&buf[pos]);
#endif
      int red = 0;
      int green = 0;
      int blue = 0;
      int index = pos;
      char ch = buf[index++];
      while(ch != ',' && ch != 0 ) {
        red *= 10;
        red += (ch - '0');
        ch = buf[index++];
      }
      ch = buf[index++];
      while(ch != ',' && ch != 0 ) {
        green *= 10;
        green += (ch - '0');
        ch = buf[index++];
      }
      ch = buf[index++];
      while(ch != ',' && ch != 0 ) {
        blue *= 10;
        blue += (ch - '0');
        ch = buf[index++];
      }

#ifdef DEBUG
      Serial.print( "Red: " );
      Serial.println( red,DEC );
      Serial.print( "Green: " );
      Serial.println( green,DEC );
      Serial.print( "Blue: " );
      Serial.println( blue,DEC );
#endif

      // Set the RGB LEDS
      fadeTo( red, green, blue );
    }
  }
}

//function fades existing values to new RGB values
void fadeTo(int r, int g, int b)
{
  //map values
  r = map(r, 0, 255, LOW_LIMIT, HIGH_LIMIT);
  g = map(g, 0, 255, LOW_LIMIT, HIGH_LIMIT);
  b = map(b, 0, 255, LOW_LIMIT, HIGH_LIMIT);

  //output
  fadeToColour( REDPIN, currentRed, r );
  fadeToColour( GREENPIN, currentGreen, g );
  fadeToColour( BLUEPIN, currentBlue, b );
  
  currentRed = r;
  currentGreen = g;
  currentBlue = b;
}

// Fade a single colour
void fadeToColour( int pin, int fromValue, int toValue ) {
  int increment = (fromValue > toValue ? -1 : 1 );
  int startValue = (fromValue > toValue ?  : 1 );
  
  if( fromValue == toValue ) 
    return;  // Nothing to do!

  if( fromValue > toValue ) {
    // Fade down
    for( int i = fromValue; i >= toValue; i += increment ) {
      analogWrite( pin, i );
      delay(10);
    }
  } else {
    // Fade up
    for( int i = fromValue; i <= toValue; i += increment ) {
      analogWrite( pin, i );
      delay(10);
    }
  }
}


//function holds RGB values for time t milliseconds, mainly for demo
void solid(int r, int g, int b, int t)
{
  //map values
  r = map(r, 0, 255, LOW_LIMIT, HIGH_LIMIT);
  g = map(g, 0, 255, LOW_LIMIT, HIGH_LIMIT);
  b = map(b, 0, 255, LOW_LIMIT, HIGH_LIMIT);

  //output
  analogWrite(REDPIN,r);
  analogWrite(GREENPIN,g);
  analogWrite(BLUEPIN,b);

  currentRed = r;
  currentGreen = g;
  currentBlue = b;

  //hold at this colour set for t ms
  if( delay > 0 )
    delay(t);
}

void setup(){
#ifdef DEBUG
  Serial.begin(19200);
  Serial.println("Ethershield Pachube RGB");
#endif

  pinMode(REDPIN,   OUTPUT);   // sets the pins as output
  pinMode(GREENPIN, OUTPUT);   
  pinMode(BLUEPIN,  OUTPUT);
  // Set the RGB LEDs off
  solid(255, 0, 0, 0 );

  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize enc28j60
  es.ES_enc28j60Init(mymac);

  solid(255, 153, 51, 0 );

#ifdef DEBUG
  Serial.print( "ENC28J60 version " );
  Serial.println( es.ES_enc28j60Revision(), HEX);
  if( es.ES_enc28j60Revision() <= 0 ) 
    Serial.println( "Failed to access ENC28J60");
#endif

  es.ES_client_set_wwwip(websrvip);  // target web server

#ifdef DEBUG
  Serial.println("Ready");
#endif
}

#ifdef DEBUG
// Output a ip address from buffer from startByte
void printIP( uint8_t *buf ) {
  for( int i = 0; i < 4; i++ ) {
    Serial.print( buf[i], DEC );
    if( i<3 )
      Serial.print( "." );
  }
}
#endif

void loop()
{
  static uint32_t timetosend;
  uint16_t dat_p;
  int sec = 0;
  long lastDnsRequest = 0L;
  int plen = 0;
  long lastDhcpRequest = millis();
  uint8_t dhcpState = 0;
  boolean gotIp = false;

  es.ES_dhcp_start( buf, mymac, myip, mynetmask,gwip, dnsip, dhcpsvrip );

  while( !gotIp ) {
    dns_state=DNS_STATE_INIT;
    // handle ping and wait for a tcp packet
    plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
    dat_p=es.ES_packetloop_icmp_tcp(buf,plen);
    if(dat_p==0) {
      int retstat = es.ES_check_for_dhcp_answer( buf, plen);
      dhcpState = es.ES_dhcp_state();
      // we are idle here
      if( dhcpState != DHCP_STATE_OK ) {
        if (millis() > (lastDhcpRequest + 10000L) ){
          lastDhcpRequest = millis();
          // send dhcp
#ifdef DEBUG
          Serial.println("Sending DHCP Request");
#endif
          es.ES_dhcp_start( buf, mymac, myip, mynetmask,gwip, dnsip, dhcpsvrip );
        }
      } 
      else {
        if( !gotIp ) {
#ifdef DEBUG
          // Display the results:
          Serial.print( "My IP: " );
          printIP( myip );
          Serial.println();

          Serial.print( "Netmask: " );
          printIP( mynetmask );
          Serial.println();

          Serial.print( "DNS IP: " );
          printIP( dnsip );
          Serial.println();

          Serial.print( "GW IP: " );
          printIP( gwip );
          Serial.println();
#endif
          gotIp = true;
          solid(0, 255, 0, 0 );

          //init the ethernet/ip layer:
          es.ES_init_ip_arp_udp_tcp(mymac, myip, PORT);

          // Set the Router IP
          es.ES_client_set_gwip(gwip);  // e.g internal IP of dsl router

          // Set the DNS server IP address if required, or use default
          es.ES_dnslkup_set_dnsip( dnsip );
        }
      }
    }
  }


  // Main processing loop now we have our addresses
  while( es.ES_dhcp_state() == DHCP_STATE_OK ) {
    // Stays within this loop as long as DHCP state is ok
    // If it changes then it drops out and forces a renewal of details
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
        if (dns_state==DNS_STATE_INIT){
#ifdef DEBUG
          Serial.println("Request DNS" );
#endif
          sec=0;
          dns_state=DNS_STATE_REQUESTED;
          lastDnsRequest = millis();
          es.ES_dnslkup_request(buf,(uint8_t*)WEBSERVER_VHOST);
          continue;
        }
        if (dns_state==DNS_STATE_REQUESTED && es.ES_udp_client_check_for_dns_answer( buf, plen ) ){
#ifdef DEBUG
          Serial.println( "DNS Answer");
#endif
          dns_state=DNS_STATE_ANSWER;
          es.ES_client_set_wwwip(es.ES_dnslkup_getip());
        }
        if (dns_state!=DNS_STATE_ANSWER){
          // retry every minute if dns-lookup failed:
          if (millis() > (lastDnsRequest + 60000L) ){
            dns_state=DNS_STATE_INIT;
            lastDnsRequest = millis();
          }
          // don't try to use web client before
          // we have a result of dns-lookup
          continue;
        }
      } 
      else {
        if (dns_state==DNS_STATE_REQUESTED && es.ES_udp_client_check_for_dns_answer( buf, plen ) ){
          dns_state=DNS_STATE_ANSWER;
#ifdef DEBUG
          Serial.println( "DNS Answer 2");
#endif
          es.ES_client_set_wwwip(es.ES_dnslkup_getip());
        }
      }
    }
    // If we have IP address for server and its time then request data

    if( dns_state == DNS_STATE_ANSWER && millis() - timetosend > 3000)  // every 10 seconds
    {
      timetosend = millis();
#ifdef DEBUG
      Serial.println("Sending request");
#endif
      // note the use of PSTR - this puts the string into code space and is compulsory in this call
      // second parameter is a variable string to append to HTTPPATH, this string is NOT a PSTR
      es.ES_client_browse_url(PSTR(HTTPPATH), NULL, PSTR(HOSTNAME), &browserresult_callback);
    }
  }
}


