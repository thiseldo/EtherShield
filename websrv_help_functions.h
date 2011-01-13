/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher 
 * Copyright: GPL V2
 *********************************************/
//@{
#ifndef WEBSRV_HELP_FUNCTIONS_H
#define WEBSRV_HELP_FUNCTIONS_H

// These functions are documented in websrv_help_functions.c
extern uint8_t find_key_val(char *str,char *strbuf, uint8_t maxlen,char *key);
extern void urldecode(char *urlbuf);
extern void urlencode(char *str,char *urlbuf);
extern uint8_t parse_ip(uint8_t *bytestr,char *str);
extern void mk_net_str(char *resultstr,uint8_t *bytestr,uint8_t len,char separator,uint8_t base);


#endif /* WEBSRV_HELP_FUNCTIONS_H */
//@}
