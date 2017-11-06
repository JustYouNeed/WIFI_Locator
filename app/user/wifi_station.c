#include "wifi_station.h"
#include "tcp_cilent.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"


uint8 AP_Count = 0;   /* 该变量指示扫描到的热点数量  */
ap_info AP_Info[32];  /* 该数组存储扫描到的热点信息  */

//extern xTaskHandle hWifiScanTask;  /* 热点扫描任务句柄   */
//extern xTaskHandle hPrintTask;    /* 打印任务句柄  */
//extern xTaskHandle hLedTask;      /* LED状态指示任务句柄  */
//extern xTaskHandle hWaitWifiConnectTask;   /* 等待WIFI连接成功任务句柄  */

extern bool IsTCPConnected;   /* 该标志指示TCP连接状态 */
extern bool IsWIFIConnected;  /* 该标志指示WIFI连接状态  */
//extern bool IsTCPSendOK;       /* 该标志指示TCP数据发送状态  */


extern struct espconn TCP_Conn;  /* 该变量指示TCP连接结构体  */
extern uint8 AP_ScanDone;    /* 该变量指示热点扫描是否完成  */

const char *AP_SSID[] = {"Vector","CMCC"};



/******************************************************************************
 * FunctionName : wifi_station_config
 * Description  :Configuring esp8266 to station mode
 *
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void wifi_station_config(void)
{
	uint8 wifi_mode = 0x00;
	/* 获取当前 wifi 模式 */
	wifi_mode = wifi_get_opmode();
	if(wifi_mode != STATIONAP_MODE) /* 如果当前模式不为STATION ,AP模式 ， 则要设置为该模式*/
	{
		wifi_set_opmode(STATIONAP_MODE);
		struct station_config *config = (struct station_config *)zalloc(sizeof(struct station_config));

		wifi_station_get_config_default(config); /* 获取内部原来保存的热点信息 */
		if(strcmp(config->ssid, CONNECT_AP_SSID))  /* 如果内部的热点信息和要连接的热点信息不相同，则要连接到设置的热点 */
		{
			sprintf(config->ssid, CONNECT_AP_SSID);
			sprintf(config->password, CONNECT_AP_PASSWORD);
		}

		wifi_station_set_config(config);  /* 连接到热点 */
		free(config);
		wifi_station_connect();
	}
}




/******************************************************************************
 * FunctionName : wifi_waitfor_connected
 * Description  :等待模块连接上热点
 *
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void wifi_waitfor_connected(void *p_arg)
{
 static	uint8 connect_status = 0;

	while(1)
	{
		connect_status = wifi_station_get_connect_status(); /* 获取当前连接状态 */

		if(connect_status == STATION_GOT_IP) /* 如果已经获得了IP地址，说明已经接连上热点 */
		{
			IsWIFIConnected = true;
			printf("wifi connect successful!\r\n");
			wifi_tcp_cilent_config();  /* 连接TCP Server */
			vTaskSuspend(NULL);
			//vTask
		}else
		{
			printf("wifi connecting...\r\n");
		}
		vTaskDelay(500);
	}
}

/******************************************************************************
 * FunctionName : wifi_scan_ap_config
 * Description  :configuring esp8266 to station mode or AP mode for scan other ap host
 *
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void wifi_scan_ap_config(void)
{
	uint8 wifi_mode = 0x00;
	wifi_mode = wifi_get_opmode();   //

	wifi_station_scan(NULL,wifi_scan_ap_done);
}

/******************************************************************************
 * FunctionName : wifi_scan_ap_config
 * Description  :configuring esp8266 to station mode or AP mode for scan other ap host
 *
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void wifi_scan_ap_done(void *p_arg, STATUS status)
{
	uint8 cnt = 0;
	AP_Count = 0;
	if(status == OK)
	{
		AP_ScanDone = TRUE;
		struct bss_info *bss_link = (struct bss_info *)p_arg;
		while(bss_link != NULL)
		{
			memset(AP_Info[AP_Count].ssid, 0, 32);
			memset(AP_Info[AP_Count].mac, 0, 6);
			AP_Info[AP_Count].rssi = 0;

			if(strcmp(bss_link->ssid, "Vector") == 0)  /* 只有设置好热点信息才保存下来  */
			{
				if(strlen(bss_link->ssid) <= 32)
					memcpy(AP_Info[AP_Count].ssid, bss_link->ssid, strlen(bss_link->ssid));
				else
					memcpy(AP_Info[AP_Count].ssid, bss_link->ssid, 32);

				memcpy(AP_Info[AP_Count].mac, bss_link->bssid, 6);
				AP_Info[AP_Count].rssi = bss_link->rssi;

				AP_Count ++;
			}
			bss_link = bss_link->next.stqe_next;
		}
	}else
		printf("faild!!!\r\n");
}

/******************************************************************************
 * FunctionName : wifi_scan_ap_config
 * Description  :configuring esp8266 to station mode or AP mode for scan other ap host
 *
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void wifi_scan_ap(void *p_arg)
{
	bool scan_ok = false;
	wifi_set_opmode(STATIONAP_MODE);

	while(1)
	{
		if(AP_ScanDone == FALSE && IsTCPConnected)
		{
			scan_ok = wifi_station_scan(NULL,wifi_scan_ap_done);
			if(scan_ok)
				printf("wifi scan done\r\n");
			else
				printf("scan failed!!!!!!!!!\r\n");
		}
		vTaskDelay(500);
	}
}
