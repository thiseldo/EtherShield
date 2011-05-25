// EtherShield webserver demo
#include "EtherShield.h"

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {
  0x54,0x55,0x58,0x10,0x00,0x25}; 
  
static uint8_t myip[4] = {
  192,168,1,25};

#define MYWWWPORT 80
#define BUFFER_SIZE 550
static uint8_t buf[BUFFER_SIZE+1];

// The ethernet shield
EtherShield es=EtherShield();

uint16_t http200ok(void)
{
  return(es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf)
{
  uint16_t plen;
  plen=http200ok();
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<html><head><title>Arduino ENC28J60 Ethernet Shield V1.0</title></head><body>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<center><h1>Welcome to Arduino ENC28J60 Ethernet Shield V1.0</h1>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<hr><br><h2><font color=\"blue\">-- Put your ARDUINO online -- "));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<br> Control digital outputs"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<br> Read digital analog inputs HERE"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<br></font></h2>") );
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</center><hr>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("V1.0 <a href=\"http://blog.thiseldo.co.uk\">blog.thiseldo.co.uk</a>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</body></html>"));

  return(plen);
}


void setup(){

  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize enc28j60
  es.ES_enc28j60Init(mymac);

  // init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, MYWWWPORT);
}

void loop(){
  uint16_t plen, dat_p;

  while(1) {
    // read packet, handle ping and wait for a tcp packet:
    dat_p=es.ES_packetloop_icmp_tcp(buf,es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));

    /* dat_p will be unequal to zero if there is a valid 
     * http get */
    if(dat_p==0){
      // no http request
      continue;
    }
    // tcp port 80 begin
    if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
      // head, post and other methods:
      dat_p=http200ok();
      dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("<h1>200 OK</h1>"));
      goto SENDTCP;
    }
    // just one web page in the "root directory" of the web server
    if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0){
      dat_p=print_webpage(buf);
      goto SENDTCP;
    }
    else{
      dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
      goto SENDTCP;
    }
SENDTCP:
    es.ES_www_server_reply(buf,dat_p); // send web page data
    // tcp port 80 end

  }

}


