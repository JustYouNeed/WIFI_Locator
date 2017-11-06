# ifndef __TCP_CILENT_H
# define __TCP_CILENT_H

# include "esp_common.h"

# include "sockets.h"
# include "espconn.h"


#include "c_types.h"
#include "mem.h"

#include "gpio.h"


# define CONNECT_TCP_SERVER_IP0		192
# define CONNECT_TCP_SERVER_IP1		168
# define CONNECT_TCP_SERVER_IP2		137
# define CONNECT_TCP_SERVER_IP3		1
# define CONNECT_TCP_SERVER_PORT	8080

extern bool tcp_IsTxOK;

void wifi_tcp_cilent_config(void);
bool wifi_tcp_cilent_senddata(struct espconn *per_espconn, uint8 *pbuff);

void _cb_wifi_tcp_connect(void *p_arg);
void _cb_wifi_tcp_disconnect(void *p_arg);
void _cb_wifi_tcp_reconnect(void *p_arg, sint8 err);
void _cb_wifi_tcp_sentdata(void *p_arg);
void _cb_wifi_tcp_recvdata(void *p_arg, char *pdata, unsigned short len);

# endif


