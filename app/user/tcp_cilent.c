#include "tcp_cilent.h"
# include "wifi_station.h"

# include "mem.h"

extern xTaskHandle hWifiScanTask;  /* 热点扫描任务句柄   */
//extern xTaskHandle hPrintTask;    /* 打印任务句柄  */
//extern xTaskHandle hLedTask;      /* LED状态指示任务句柄  */
//extern xTaskHandle hWaitWifiConnectTask;   /* 等待WIFI连接成功任务句柄  */

extern bool IsTCPConnected;   /* 该标志指示TCP连接状态 */
//extern bool IsWIFIConnected;  /* 该标志指示WIFI连接状态  */
extern bool IsTCPSendOK;       /* 该标志指示TCP数据发送状态  */

//extern uint8 AP_Count;   /* 该变量指示扫描到的热点数量  */
//extern ap_info AP_Info[32];  /* 该数组存储扫描到的热点信息  */
extern struct espconn TCP_Conn;  /* 该变量指示TCP连接结构体  */
extern uint8 AP_ScanDone;    /* 该变量指示热点扫描是否完成  */

sint8 tcp_cilent_status = 0;

LOCAL struct _esp_tcp user_tcp;
ip_addr_t tcp_server_ip;

void wifi_tcp_cilent_config(void)
{

	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);

	if(wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0)
	{

		// Connect to tcp server as NET_DOMAIN
		TCP_Conn.proto.tcp = &user_tcp;
		TCP_Conn.type = ESPCONN_TCP;
		TCP_Conn.state = ESPCONN_NONE;

		const char esp_tcp_server_ip[4] = {CONNECT_TCP_SERVER_IP0, CONNECT_TCP_SERVER_IP1, CONNECT_TCP_SERVER_IP2, CONNECT_TCP_SERVER_IP3}; // remote IP of TCP server
		sprintf(TCP_Conn.proto.tcp->remote_ip, esp_tcp_server_ip);

		TCP_Conn.proto.tcp->remote_port = 8080;  // remote port

		TCP_Conn.proto.tcp->local_port = espconn_port(); //local port of ESP8266

		espconn_regist_connectcb(&TCP_Conn, _cb_wifi_tcp_connect); // register connect callback
		espconn_regist_reconcb(&TCP_Conn, _cb_wifi_tcp_reconnect); // register reconnect callback as error handler
		tcp_cilent_status = espconn_connect(&TCP_Conn);

	}
}


bool wifi_tcp_cilent_senddata(struct espconn *per_espconn, uint8 *pbuff)
{
	sint16 status = 0;

	//status = espconn_send(per_espconn, (uint8 *)pbuff, strlen(pbuff));
	return TRUE;
}




void ICACHE_FLASH_ATTR _cb_wifi_tcp_sentdata(void *p_arg)
{
	printf("send data ok!\r\n");
	IsTCPSendOK = true;
}
void ICACHE_FLASH_ATTR _cb_wifi_tcp_recvdata(void *p_arg, char *pdata, unsigned short len)
{
	printf("recv data\r\n");
}



void ICACHE_FLASH_ATTR _cb_wifi_tcp_connect(void *p_arg)
{
	struct espconn *espsconn_temp = (struct espconn *)p_arg;
	printf("wifi tcp connect\r\n");

	IsTCPConnected = true;  //TCP Connected


    espconn_regist_recvcb(espsconn_temp, _cb_wifi_tcp_recvdata);
    espconn_regist_sentcb(espsconn_temp, _cb_wifi_tcp_sentdata);

    espconn_regist_disconcb(espsconn_temp, _cb_wifi_tcp_disconnect);

	if(tcp_cilent_status == 0  && hWifiScanTask == 0)
	{
		xTaskCreate(wifi_scan_ap, "AP Scan Task", 512, NULL, 4, &hWifiScanTask);
	}
}

void _cb_wifi_tcp_disconnect(void *p_arg)
{
	printf("wifi tcp disconnect!\r\n");

	IsTCPConnected = false;  //TCP Disconnect

	wifi_tcp_cilent_config();
}

void ICACHE_FLASH_ATTR _cb_wifi_tcp_reconnect(void *p_arg, sint8 err)
{
	printf("wifi tcp reconnect\r\n");
	IsTCPConnected = false;  //TCP Disconnect
	wifi_tcp_cilent_config();
}
