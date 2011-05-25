/*
 * Arduino ENC28J60 Ethernet shield DNS client test
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

// IP address of the host being queried to contact (IP of the first portion of the URL):
static uint8_t websrvip[4] = { 
  0, 0, 0, 0 };

// Default gateway. The ip address of your DSL router. It can be set to the same as
// websrvip the case where there is no default GW to access the 
// web server (=web server is on the same lan as this host) 
static uint8_t gwip[4] = {
  192,168,1,1};
  
// DNS server IP.
// If ommited, or no call is made to ES_dnslkup_set_dnsip(dnsip), library will
// default to Google DNS server at 8.8.8.8
static uint8_t dnsip[4] = {
  192,168,1,1};    

// global string buffer for hostname message:
static char hoststr[150];

// listen port for tcp/www:
#define MYWWWPORT 80

static volatile uint8_t start_web_client=0;  // 0=off but enabled, 1=send tweet, 2=sending initiated, 3=twitter was sent OK, 4=diable twitter notify
static uint8_t contact_onoff_cnt=0;
static uint8_t web_client_attempts=0;
static uint8_t web_client_sendok=0;
static uint8_t resend=0;
static int8_t dns_state=-1;

#define BUFFER_SIZE 550
static uint8_t buf[BUFFER_SIZE+1];

EtherShield es=EtherShield();

// analyse the url given
// return values: -1 invalid password
//                -2 no command given 
//                0 switch off
//                1 switch on
//
//                The string passed to this function will look like this:
//                /?host=www.hostname.com HTTP/1.....
//                / HTTP/1.....
int8_t analyse_get_url(char *str)
{
  uint8_t mn=0;
  char kvalstrbuf[10];
  // the first slash:
  if (str[0] == '/' && str[1] == ' '){
    // end of url, display just the web page
    return(2);
  }
  // str is now something like ?host=www.xyz.com or just end of url
  if (es.ES_find_key_val(str,hoststr,150,"host")){
    dns_state=0;  // Request DNS
    return( 0 );
  }
  // browsers looking for /favion.ico, non existing pages etc...
  return(-1);
}
uint16_t http200ok(void)
{
  return(es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}


// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf)
{
  uint16_t plen;
  char vstr[5];
  plen=http200ok();
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<h2>DNS Client Test</h2>\n<pre>\n"));

  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("\n<form action=/ method=get>\nHost: <input type=text size=50 name=host>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<input type=submit value=\"Submit Address\"></form>\n"));

  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<a href=\"http://192.168.1.25/\">Refresh to see Results</a>\n"));

  // Display last results
  if( dns_state >= 0 ) {
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("\nHostname: "));
    plen=es.ES_fill_tcp_data(buf,plen,hoststr);

    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("\nIP address: "));
    if( dns_state == 2 ) {
      // convert number to string:
      itoa(websrvip[0],vstr,10);
      plen=es.ES_fill_tcp_data(buf,plen,vstr);
      plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("."));
      itoa(websrvip[1],vstr,10);
      plen=es.ES_fill_tcp_data(buf,plen,vstr);
      plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("."));
      itoa(websrvip[2],vstr,10);
      plen=es.ES_fill_tcp_data(buf,plen,vstr);
      plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("."));
      itoa(websrvip[3],vstr,10);
      plen=es.ES_fill_tcp_data(buf,plen,vstr);
    } 
    else {
      plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("Wating for response") );
    }
  }
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("\n</pre><br><hr><a href='http://blog.thiseldo.co.uk'>blog.thiseldo.co.uk</a>"));
  return(plen);
}


void browserresult_callback(uint8_t statuscode,uint16_t datapos){
  if (statuscode==0){
    web_client_sendok++;
  }
  // clear pending state at sucessful contact with the
  // web server even if account is expired:
  if (start_web_client==2) start_web_client=3;
}


void setup(){
  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize ENC28J60
  es.ES_enc28j60Init(mymac);

  //init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, MYWWWPORT);

  // init the web client:
  es.ES_client_set_gwip(gwip);  // e.g internal IP of dsl router
  es.ES_dnslkup_set_dnsip(dnsip); // generally same IP as router

}

void loop(){
  uint16_t dat_p;
  int8_t cmd;
  start_web_client=1;
  int sec = 0;
  long lastDnsRequest = 0L;

  while(1){
    // handle ping and wait for a tcp packet
    int plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
    dat_p=es.ES_packetloop_icmp_tcp(buf,plen);
    if(dat_p==0) {
      // we are idle here
      if (es.ES_client_waiting_gw() ){
        continue;
      }
      if (dns_state==0){
        sec=0;
        dns_state=1;
        lastDnsRequest = millis();
        es.ES_dnslkup_request(buf, (uint8_t*)hoststr );
        continue;
      }
      if (dns_state==1 && es.ES_udp_client_check_for_dns_answer( buf, plen ) ){
        dns_state=2;
        //        es.ES_client_set_wwwip(es.ES_dnslkup_getip());
        uint8_t *websrvipptr = &websrvip[0];
        websrvipptr = es.ES_dnslkup_getip();
        
        for(int on=0; on <4; on++ ) {
          websrvip[on] = *websrvipptr++;
        }
        continue;
      }
    } else {
      if (dns_state==1 && es.ES_udp_client_check_for_dns_answer( buf, plen ) ){
        dns_state=2;
        //        es.ES_client_set_wwwip(es.ES_dnslkup_getip());
        uint8_t *websrvipptr = &websrvip[0];
        websrvipptr = es.ES_dnslkup_getip();
        
        for(int on=0; on <4; on++ ) {
          websrvip[on] = *websrvipptr++;
        }
        continue;
      }
    }

    if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
      // head, post and other methods:
      //
      // for possible status codes see:
      // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
      dat_p=print_webpage(buf);
      goto SENDTCP;
    }

    cmd=analyse_get_url((char *)&(buf[dat_p+4]));
    if( cmd == 0 ) {
       // wait for DNS response
      continue;
    }

    dat_p=print_webpage(buf);

SENDTCP:
    es.ES_www_server_reply(buf,dat_p); // send data
  }
}

