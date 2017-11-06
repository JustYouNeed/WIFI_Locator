/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"
#include "tcp_cilent.h"
#include "wifi_station.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
# include "freertos/list.h"
# include "freertos/croutine.h"
# include "freertos/FreeRTOSConfig.h"
# include "freertos/portable.h"
# include "gpio.h"


# define MAC_LENGTH		6

xTaskHandle hWifiScanTask = 0;  /* 热点扫描任务句柄   */
xTaskHandle hPrintTask = 0;    /* 打印任务句柄  */
xTaskHandle hLedTask = 0;      /* LED状态指示任务句柄  */
xTaskHandle hWaitWifiConnectTask = 0;   /* 等待WIFI连接成功任务句柄  */


bool IsTCPConnected = false;   /* 该标志指示TCP连接状态 */
bool IsWIFIConnected = false;  /* 该标志指示WIFI连接状态  */
bool IsTCPSendOK = true;       /* 该标志指示TCP数据发送状态  */


struct espconn TCP_Conn;  /* 该变量指示TCP连接结构体  */
uint8 AP_ScanDone = false;    /* 该变量指示热点扫描是否完成  */
/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

uint8 * pre_Packet(uint8 * ssid, uint8 * mac, sint8 rssi)
{


	uint8 len = strlen(ssid) + strlen(mac) + 1;
	uint8 *packet = (uint8 *)zalloc(sizeof(uint8) * (len + 8));

	uint16 i = 0, j = 0;
	uint8 check = 0;


	uint8 temp = (uint8)rssi * 0xff;

	packet[j++] = 0xaf;  //帧头
	packet[j++] = 0xfa;  //帧头

	packet[j++] = strlen(ssid);  //ssid长度
	packet[j++] = strlen(mac);  //mac长度
	packet[j++] = 0x01;

	len = strlen(ssid);
	for(i = 0; i < len - 2; i++)
	{
		packet[j++] = ssid[i];
	}

	len = strlen(mac);

	for(i = 0; i < len - 2; i++)
	{
		packet[j++] = mac[i];
	}



	for(i = 0; i < j; i++)
		check += packet[i];

	packet[j++] = 0xff&check; //校验位

	packet[j++] = 0xfa;  //帧尾
	packet[j++] = 0xaf;  //帧尾

	free(packet);
	return packet;
}


void print_task(void *p_arg)
{
	uint8 packet[32];
    uint8 rssi = 0;
	sint8 temp=0,i=0;
	uint8 count = 0;
	uint8 connect_status;
	while(1)
	{
		printf("print task is running ...\r\n");
		connect_status = wifi_station_get_connect_status(); /* 获取当前连接状态 */

		//if(connect_status)

		if(AP_ScanDone && IsTCPConnected)  //
		{
			AP_ScanDone = FALSE;
			printf("AP count is %d\r\n",AP_Count);
			for(i = 0; i< AP_Count; i++)
			{
				rssi = (uint8)AP_Info[i].rssi & 0xff;
				memset(packet,0,32);

				packet[0] = 0xaf;
				packet[1] = 0xfa;
				packet[2] = MAC_LENGTH;
				packet[3] = AP_Info[i].mac[0];
				packet[4] = AP_Info[i].mac[1];
				packet[5] = AP_Info[i].mac[2];
				packet[6] = AP_Info[i].mac[3];
				packet[7] = AP_Info[i].mac[4];
				packet[8] = AP_Info[i].mac[5];
				packet[9] = 0xa5;

				packet[10] = rssi;
				packet[11] = 0xfa;
				packet[12] = 0xaf;

			//	packet = (uint8*)pre_Packet(AP_Info[i].ssid, AP_Info[i].mac, AP_Info[i].rssi);

			    /* 只有上一次发送成功后才进行下一次发送 */
				if(IsTCPSendOK == true)
				{
					temp = espconn_sent(&TCP_Conn, packet, 13);
					IsTCPSendOK = false;
				}

				espconn_set_opt(&TCP_Conn, 0x07);
				if(temp == -12)
				{
					printf("TCP Disconnect ,reconnecting\r\n");
					espconn_disconnect(&TCP_Conn);
					wifi_tcp_cilent_config();
				}
				printf("temp:%d\r\n",temp);
				printf("packet:%s\r\n",packet);
			}
		}

		vTaskDelay(200);
	}
}



static bool led_flag = false;
void led_task(void *p_arg)
{
	uint8 connect_status = 0;
	while(1)
	{
		connect_status = wifi_station_get_connect_status(); /* 获取当前连接状态 */

		if(led_flag)
		{
			if(connect_status != STATION_GOT_IP)
				gpio_output_set(0,	BIT12,	BIT12,	0);
			else
				gpio_output_set(0,	BIT13,	BIT13,	0);
			led_flag = false;
		}
		else
		{
			if(connect_status != STATION_GOT_IP)
				gpio_output_set(BIT12,	0,	BIT12,	0);
			else
				gpio_output_set(BIT13,	0,	BIT13,	0);
			led_flag = true;
		}
		if(connect_status != STATION_GOT_IP)
			vTaskDelay(5);
		else if(connect_status == STATION_CONNECTING)
			vTaskDelay(30);
		else vTaskDelay(50);
	}
}


void system_config(void)
{
	uart_init_new();
	printf("usart boundrate is 115200\r\n");
	printf("SDK version:%s\r\n", system_get_sdk_version());
	printf("Chip Id is :%d\r\n",system_get_chip_id());

	wifi_station_config();
	wifi_scan_ap_config();
}

void led_Config(void)
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U,FUNC_GPIO14);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U,FUNC_GPIO13);
//	gpio_output_set(0,	BIT14,	BIT14,	0);
//	gpio_output_set(0,	BIT12,	BIT12,	0);
//	gpio_output_set(0,	BIT13,	BIT13,	0);
	gpio_output_set(BIT12,	0,	BIT12,	0);
	gpio_output_set(BIT13,	0,	BIT13,	0);
//	gpio_output_set(BIT14,	0,	BIT14,	0);
}
/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	system_config();
	espconn_init();
	led_Config();
//	xEventGroupCreate();
	 xTaskCreate(print_task, "print task",256, NULL, 9, &hPrintTask);
	 xTaskCreate(wifi_waitfor_connected, "wifi_waitfor_connected",256, NULL, 6, &hWaitWifiConnectTask);
	 xTaskCreate(led_task, "led task", 128, NULL, 3, &hLedTask);
}

