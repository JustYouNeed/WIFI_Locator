# ifndef __WIFI_AP_H
# define __WIFI_AP_H

# include "esp_common.h"


# define CONNECT_AP_SSID		"Vector"
# define CONNECT_AP_PASSWORD	"19960114"

typedef struct _ap_info
{
	uint8 ssid[32];
	sint8 rssi;
	uint8 mac[6];
}ap_info;

extern uint8 AP_Count;   /* 该变量指示扫描到的热点数量  */
extern ap_info AP_Info[32];  /* 该数组存储扫描到的热点信息  */
void wifi_waitfor_connected(void *p_arg);
void wifi_station_config(void);
void wifi_scan_ap_config(void);
void wifi_scan_ap_done(void *p_arg, STATUS status);
void wifi_scan_ap(void *p_arg);



# endif

