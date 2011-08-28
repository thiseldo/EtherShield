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

// If using a Nanode (www.nanode.eu) instead of Arduino and ENC28J60 EtherShield then
// use this define:
#define NANODE

#include <EtherShield.h>
#ifdef NANODE
#include <NanodeMAC.h>
#endif

#define DEBUG

// If using a Common Anode RGB LED (i.e. has connection to +5V
// Then leave this uncommented this
//#define COMMON_ANODE
// If using Common Cathode RGB LED (i.e. has common connection to GND)
// then comment out the above line or change to:
#undef COMMON_ANODE

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

// Please modify the following lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
#ifdef NANODE
static uint8_t mymac[6] = { 0,0,0,0,0,0 };
#else
static uint8_t mymac[6] = { 0x54,0x55,0x58,0x12,0x34,0x56 };
#endif

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
// Replace xxxxxxxxxxxxx with your Pachube API key
#define HOSTNAME "www.pachube.com\r\nX-PachubeApiKey: xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"          // API key
#define WEBSERVER_VHOST "www.pachube.com"
// Replace nnnnn with your Feed number
#define HTTPPATH "/api/nnnnn.csv"      // The feed - use API V2 csv

static uint8_t resend=0;
static int8_t dns_state=DNS_STATE_INIT;

EtherShield es=EtherShield();
#ifdef NANODE
NanodeMAC mac( mymac );
#endif

#define BUFFER_SIZE 750
static uint8_t buf[BUFFER_SIZE+1];

void browserresult_callback(uint8_t statuscode,uint16_t datapos){
char headerEnd[2] = {'\r','\n' };
int contentLen = 0;

#ifdef DEBUG
  Serial.print("Received data, status:"); 
  Serial.println(statuscode,DEC);
//  Serial.println((char*)&buf[datapos]);
#endif

  if (datapos != 0)
  {
    // Scan headers looking for Content-Length: 5
    // Start of a line, look for "Content-Length: "
    // now search for the csv data - it follows the first blank line
    uint16_t pos = datapos;
    while (buf[pos])    // loop until end of buffer (or we break out having found what we wanted)
    {
      // Look for line with \r\n on its own
      if( strncmp ((char*)&buf[pos],headerEnd, 2) == 0 ) {
        Serial.println("End of headers");
        pos += 2;
        break;
      }
      
      if( strncmp ((char*)&buf[pos], "Content-Length:", 15) == 0 ) {
        // Found Content-Length 
        pos += 16;          // Skip to value
        char ch = buf[pos++];
        contentLen = 0;
        while(ch >= '0' && ch <= '9' ) {  // Only digits
          contentLen *= 10;
          contentLen += (ch - '0');
          ch = buf[pos++];
        }
#ifdef DEBUG
        Serial.print("Content Length: " );
        Serial.println( contentLen, DEC );
#endif
      }
      // Scan to end of line
      while( buf[pos++] != '\r' ) { }
      while( buf[pos++] != '\n' ) { }
      
      if (buf[pos] == 0) break; // run out of buffer??
    }
    if (buf[pos])  // we didn't run out of buffer
    {

      int red = 0;
      int green = 0;
      int blue = 0;
      int index = pos;
      char ch = buf[index++];
      while(ch >= '0' && ch <= '9' ) {
        red *= 10;
        red += (ch - '0');
        ch = buf[index++];
      }
      ch = buf[index++];
      while(ch >= '0' && ch <= '9') {
        green *= 10;
        green += (ch - '0');
        ch = buf[index++];
      }
      ch = buf[index++];
      while(ch >= '0' && ch <= '9' && index < (pos+contentLen+1)) {
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
//      solid( red, green, blue, 0 );
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
#ifdef NANODE
  es.ES_enc28j60Init(mymac,8);
#else
  es.ES_enc28j60Init(mymac);
#endif

  solid(255, 153, 51, 0 );

#ifdef DEBUG
  Serial.print( "ENC28J60 version " );
  Serial.println( es.ES_enc28j60Revision(), HEX);
  if( es.ES_enc28j60Revision() <= 0 ) {
    Serial.println( "Failed to access ENC28J60");

    while(1);    // Just loop here
  }
#endif

  es.ES_client_set_wwwip(websrvip);  // target web server
/*  
  for( int i=0; i<4; i++ ) {
   solid(255,0,0, 200 );
   solid(0,255,0, 200 );
   solid(0,0,255, 200 );
   }
  
  // All off
  solid( 0, 0, 0, 0 );
*/
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

  // Get IP Address details
  if( es.allocateIPAddress(buf, BUFFER_SIZE, mymac, 80, myip, mynetmask, gwip, dnsip, dhcpsvrip ) > 0 ) {
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
    // Perform DNS Lookup for host name
    if( es.resolveHostname(buf, BUFFER_SIZE,(uint8_t*)WEBSERVER_VHOST ) > 0 ) {
      Serial.println("Hostname resolved");
    } else {
      Serial.println("Failed to resolve hostname");
    }
  } 
  else {
    // Failed, do something else....
    Serial.println("Failed to get IP Address");
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
      } 
    }
    // If we have IP address for server and its time then request data

    if( millis() - timetosend > 3000)  // every 10 seconds
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


