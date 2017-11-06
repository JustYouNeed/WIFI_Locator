#include "wifi_station.h"
#include "tcp_cilent.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"


uint8 AP_Count = 0;   /* �ñ���ָʾɨ�赽���ȵ�����  */
ap_info AP_Info[32];  /* ������洢ɨ�赽���ȵ���Ϣ  */

//extern xTaskHandle hWifiScanTask;  /* �ȵ�ɨ��������   */
//extern xTaskHandle hPrintTask;    /* ��ӡ������  */
//extern xTaskHandle hLedTask;      /* LED״ָ̬ʾ������  */
//extern xTaskHandle hWaitWifiConnectTask;   /* �ȴ�WIFI���ӳɹ�������  */

extern bool IsTCPConnected;   /* �ñ�־ָʾTCP����״̬ */
extern bool IsWIFIConnected;  /* �ñ�־ָʾWIFI����״̬  */
//extern bool IsTCPSendOK;       /* �ñ�־ָʾTCP���ݷ���״̬  */


extern struct espconn TCP_Conn;  /* �ñ���ָʾTCP���ӽṹ��  */
extern uint8 AP_ScanDone;    /* �ñ���ָʾ�ȵ�ɨ���Ƿ����  */

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
	/* ��ȡ��ǰ wifi ģʽ */
	wifi_mode = wifi_get_opmode();
	if(wifi_mode != STATIONAP_MODE) /* �����ǰģʽ��ΪSTATION ,APģʽ �� ��Ҫ����Ϊ��ģʽ*/
	{
		wifi_set_opmode(STATIONAP_MODE);
		struct station_config *config = (struct station_config *)zalloc(sizeof(struct station_config));

		wifi_station_get_config_default(config); /* ��ȡ�ڲ�ԭ��������ȵ���Ϣ */
		if(strcmp(config->ssid, CONNECT_AP_SSID))  /* ����ڲ����ȵ���Ϣ��Ҫ���ӵ��ȵ���Ϣ����ͬ����Ҫ���ӵ����õ��ȵ� */
		{
			sprintf(config->ssid, CONNECT_AP_SSID);
			sprintf(config->password, CONNECT_AP_PASSWORD);
		}

		wifi_station_set_config(config);  /* ���ӵ��ȵ� */
		free(config);
		wifi_station_connect();
	}
}




/******************************************************************************
 * FunctionName : wifi_waitfor_connected
 * Description  :�ȴ�ģ���������ȵ�
 *
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void wifi_waitfor_connected(void *p_arg)
{
 static	uint8 connect_status = 0;

	while(1)
	{
		connect_status = wifi_station_get_connect_status(); /* ��ȡ��ǰ����״̬ */

		if(connect_status == STATION_GOT_IP) /* ����Ѿ������IP��ַ��˵���Ѿ��������ȵ� */
		{
			IsWIFIConnected = true;
			printf("wifi connect successful!\r\n");
			wifi_tcp_cilent_config();  /* ����TCP Server */
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

			if(strcmp(bss_link->ssid, "Vector") == 0)  /* ֻ�����ú��ȵ���Ϣ�ű�������  */
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
