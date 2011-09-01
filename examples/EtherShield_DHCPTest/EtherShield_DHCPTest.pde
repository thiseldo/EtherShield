/*
 * Arduino ENC28J60 Ethernet shield/Nanode DHCP client test
 */

// If using a Nanode (www.nanode.eu) instead of Arduino and ENC28J60 EtherShield then
// use this define:
#define NANODE

#include <EtherShield.h>
#ifdef NANODE
#include <NanodeMAC.h>
#endif

// Please modify the following lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
#ifdef NANODE
static uint8_t mymac[6] = { 0,0,0,0,0,0 };
#else
static uint8_t mymac[6] = { 0x54,0x55,0x58,0x12,0x34,0x56 };
#endif

static uint8_t myip[4] = { 0,0,0,0 };
static uint8_t mynetmask[4] = { 0,0,0,0 };

// IP address of the host being queried to contact (IP of the first portion of the URL):
static uint8_t websrvip[4] = { 0, 0, 0, 0 };

// Default gateway. The ip address of your DSL router. It can be set to the same as
// websrvip the case where there is no default GW to access the 
// web server (=web server is on the same lan as this host) 
static uint8_t gwip[4] = { 0,0,0,0};

static uint8_t dnsip[4] = { 0,0,0,0 };
static uint8_t dhcpsvrip[4] = { 0,0,0,0 };

#define DHCPLED 6

// listen port for tcp/www:
#define MYWWWPORT 80

#define BUFFER_SIZE 750
static uint8_t buf[BUFFER_SIZE+1];

EtherShield es=EtherShield();
#ifdef NANODE
NanodeMAC mac( mymac );
#endif

// Programmable delay for flashing LED
uint16_t delayRate = 0;

void setup() {

  Serial.begin(19200);
  Serial.println("DHCP Client test");
  pinMode( DHCPLED, OUTPUT);
  digitalWrite( DHCPLED, HIGH);    // Turn it off

  for( int i=0; i<6; i++ ) {
    Serial.print( mymac[i], HEX );
    Serial.print( i < 5 ? ":" : "" );
  }
  Serial.println();
  
  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize enc28j60
  Serial.println("Init ENC28J60");
#ifdef NANODE
  es.ES_enc28j60Init(mymac,8);
#else
  es.ES_enc28j60Init(mymac);
#endif

  Serial.println("Init done");
  
  Serial.print( "ENC28J60 version " );
  Serial.println( es.ES_enc28j60Revision(), HEX);
  if( es.ES_enc28j60Revision() <= 0 ) {
    Serial.println( "Failed to access ENC28J60");

    while(1);    // Just loop here
  }
  
  Serial.println("Requesting IP Addresse");
  // Get IP Address details
  if( es.allocateIPAddress(buf, BUFFER_SIZE, mymac, 80, myip, mynetmask, gwip, dhcpsvrip, dnsip ) > 0 ) {
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
   
    digitalWrite( DHCPLED, HIGH);
    delayRate = 1000;
  } else {
    Serial.println("Failed to get IP address");
    delayRate = 200;
  }

}

// Output a ip address from buffer
void printIP( uint8_t *buf ) {
  for( int i = 0; i < 4; i++ ) {
    Serial.print( buf[i], DEC );
    if( i<3 )
      Serial.print( "." );
  }
}

void loop(){
  digitalWrite( DHCPLED, HIGH);
  delay(delayRate);
  digitalWrite(DHCPLED, LOW);
  delay(delayRate);
}







