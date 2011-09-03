/*
 * Arduino ENC28J60 Ethernet shield twitter client
 */

// If using a Nanode (www.nanode.eu) instead of Arduino and ENC28J60 EtherShield then
// use this define:
#define NANODE

#include "EtherShield.h"
#ifdef NANODE
#include <NanodeMAC.h>
#endif

// A pin to use as input to trigger tweets
#define INPUT_PIN 2

// Note: This software implements a web server and a web browser.
// The web server is at "myip" 
// 
// Please modify the following lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
#ifdef NANODE
static uint8_t mymac[6] = { 0,0,0,0,0,0 };
#else
static uint8_t mymac[6] = { 0x54,0x55,0x58,0x12,0x34,0x56 };
#endif

static uint8_t myip[4] = { 192,168,1,25};

// IP address of the twitter server to contact (IP of the first portion of the URL):
// DNS look-up is a feature which we still need to add.
static uint8_t websrvip[4] = { 0, 0, 0, 0 };

// The name of the virtual host which you want to contact at websrvip (hostname of the first portion of the URL):
// This example has been changed to use supertweet.net instead of twitter.com
// du to the fact that twitter are dropping the us eof basic authentication in
// favour of OAuth. supertweet.net is a basic authentication proxy.

#define WEBSERVER_VHOST "api.supertweet.net"

// API URL to send request to
#define API_URL "/1/statuses/update.xml"

// The BLOGGACCOUNT Authorization code can be generated from 
// username and password of your supertweet account
// by using this encoder: http://tuxgraphics.org/~guido/javascript/base64-javascript.html

// The password is NOT your twitter.com password, but the one setup in
// supertweet.net for your username.
// It is up to you if you wish to keep them the same, but for security reasons
// this is not recommended.

#define BLOGGACCOUNT "Authorization: Basic xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// Default gateway. The ip address of your DSL router. It can be set to the same as
// websrvip the case where there is no default GW to access the 
// web server (=web server is on the same lan as this host) 
static uint8_t gwip[4] = { 192,168,1,1};
//
// global string buffer for twitter message:
static char statusstr[150];

// listen port for tcp/www:
#define MYWWWPORT 80
//
// the password string, (only a-z,0-9,_ characters):
static char password[]="secret"; 

static volatile uint8_t start_web_client=0;  // 0=off but enabled, 1=send tweet, 2=sending initiated, 3=twitter was sent OK, 4=diable twitter notify
static uint8_t contact_onoff_cnt=0;
static uint8_t web_client_attempts=0;
static uint8_t web_client_sendok=0;
static uint8_t resend=0;
static int8_t dns_state=0;

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the follow variables are longs because the time, measured in milliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 200;   // the debounce time, increase if the output flickers

#define BUFFER_SIZE 650
#define DATE_BUFFER_SIZE 30
static char datebuf[DATE_BUFFER_SIZE]="none";
static uint8_t buf[BUFFER_SIZE+1];

EtherShield es=EtherShield();
#ifdef NANODE
NanodeMAC mac( mymac );
#endif

uint8_t verify_password(char *str)
{
  // the first characters of the received string are
  // a simple password/cookie:
  if (strncmp(password,str,strlen(password))==0){
    return(1);
  }
  return(0);
}

// analyse the url given
// return values: -1 invalid password
//                -2 no command given 
//                0 switch off
//                1 switch on
//
//                The string passed to this function will look like this:
//                /?mn=1&pw=secret HTTP/1.....
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
  // str is now something like ?pw=secret&mn=0 or just end of url
  if (es.ES_find_key_val(str,kvalstrbuf,10,"mn")){
    if (kvalstrbuf[0]=='1'){
      mn=1;
    }
    // to change the mail notification one needs also a valid passw:
    if (es.ES_find_key_val(str,kvalstrbuf,10,"pw")){
      if (verify_password(kvalstrbuf)){
        return(mn);
      }
      else{
        return(-1);
      }
    }
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
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<h2>Twitter notification status</h2>\n<pre>\n"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("Number of INPUT_PIN to GND connections: "));
  // convert number to string:
  itoa(contact_onoff_cnt,vstr,10);
  plen=es.ES_fill_tcp_data(buf,plen,vstr);
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("\nNumber of twitter attempts: "));
  // convert number to string:
  itoa(web_client_attempts,vstr,10);
  plen=es.ES_fill_tcp_data(buf,plen,vstr);
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("\nNumber successful twitter: "));
  // convert number to string:
  itoa(web_client_sendok,vstr,10);
  plen=es.ES_fill_tcp_data(buf,plen,vstr);
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("\nDate of last twitter: "));
  plen=es.ES_fill_tcp_data(buf,plen,datebuf);
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("\nTwitter notify is: "));
  if (start_web_client==4){
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("OFF"));
  }
  else{
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("ON"));
  }
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("\n<form action=/ method=get>\npassw: <input type=password size=8 name=pw><input type=hidden name=mn "));
  if (start_web_client==4){
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("value=1><input type=submit value=\"enable twitter msg\">"));
  }
  else{
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("value=0><input type=submit value=\"disable twitter msg\">"));
  }
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</form>\n</pre><br><hr>tuxgraphics.org/blog.thiseldo.co.uk"));
  return(plen);
}

void store_date_if_found(char *str)
{
  uint8_t i=100; // search the first 100 char
  uint8_t j=0;
  char datekeyword[]="Date: "; // not any repeating characters, search does not need to resume at partial match
  while(i && *str){
    if (j && datekeyword[j]=='\0'){
      // found date
      i=0;
      while(*str && *str!='\r' && *str!='\n' && i< DATE_BUFFER_SIZE-1){
        datebuf[i]=*str;
        str++;
        i++;
      }
      datebuf[i]='\0';
      return;
    }
    if (*str==datekeyword[j]){
      j++;
    }
    else{
      j=0;
    }
    str++;
    i--;
  }
}

void browserresult_callback(uint8_t statuscode,uint16_t datapos){
  if (statuscode==0){
    web_client_sendok++;
    //    LEDOFF;
    // copy the "Date: ...." as returned by the server
    store_date_if_found((char *)&(buf[datapos]));
  }
  // clear pending state at sucessful contact with the
  // web server even if account is expired:
  if (start_web_client==2) start_web_client=3;
}


void setup(){

  pinMode( INPUT_PIN, INPUT );
  digitalWrite( INPUT_PIN, HIGH );    // Set internal pullup

  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize enc28j60
#ifdef NANODE
  es.ES_enc28j60Init(mymac,8);
#else
  es.ES_enc28j60Init(mymac);
#endif

  //init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, MYWWWPORT);

  // init the web client:
  es.ES_client_set_gwip(gwip);  // e.g internal IP of dsl router

}

void loop(){
  uint16_t dat_p;
  int8_t cmd;
  start_web_client=1;
  int sec = 0;
  long lastDnsRequest = 0L;
  int plen = 0;
  
  while(1){
    // handle ping and wait for a tcp packet
    plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
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
        es.ES_dnslkup_request(buf,(uint8_t*)WEBSERVER_VHOST);
        continue;
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

      if (start_web_client==1){
        start_web_client=2;
        web_client_attempts++;
        // the twitter line is status=Url_encoded_string
        strcat(statusstr,"status=");
        // the text to send to twitter (append after status=):
        //urlencode("Meals are ready",&(statusstr[7]));
        es.ES_urlencode("Test of Arduino and ENC28J60 Ethernet Shield twitter client 1234567890AA",&(statusstr[7]));
        // The BLOGGACCOUNT Authorization code can be generated from 
        // username and password of your twitter account 
        // by using this encoder: http://tuxgraphics.org/~guido/javascript/base64-javascript.html
        es.ES_client_http_post(PSTR( API_URL ),PSTR(WEBSERVER_VHOST),PSTR(BLOGGACCOUNT),NULL,statusstr,&browserresult_callback);
      }
      buttonState = digitalRead(INPUT_PIN);

      // check to see if you just pressed the button 
      // (i.e. the input went from HIGH to LOW),  and you've waited 
      // long enough since the last press to ignore any noise:  
      if ((buttonState == LOW) && 
        (lastButtonState == HIGH) && 
        (millis() - lastDebounceTime) > debounceDelay) {
        contact_onoff_cnt++;
        // ... and store the time of the last button press
        // in a variable:
        lastDebounceTime = millis();

        // Trigger a tweet        
        resend=1; // resend once if it failed
        start_web_client=1;

      }

      // save the buttonState.  Next time through the loop,
      // it'll be the lastButtonState:
      lastButtonState = buttonState;

      continue;
    } else {
      if (dns_state==1 && es.ES_udp_client_check_for_dns_answer( buf, plen ) ){
        dns_state=2;
        es.ES_client_tcp_set_serverip(es.ES_dnslkup_getip());
      }
    }

    if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
      // head, post and other methods:
      //
      // for possible status codes see:
      // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
      dat_p=http200ok();
      dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("<h1>200 OK</h1>"));
      goto SENDTCP;
    }
    cmd=analyse_get_url((char *)&(buf[dat_p+4]));
    // for possible status codes see:
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
    if (cmd==-1){
      dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
      goto SENDTCP;
    }
    if (cmd==1 && start_web_client==4){
      // twitter was off, switch on
      start_web_client=0;
    }
    if (cmd==0 ){
      start_web_client=4; // twitter off
    }
    dat_p=http200ok();
    dat_p=print_webpage(buf);
    //
SENDTCP:
    es.ES_www_server_reply(buf,dat_p); // send data

  }

}



