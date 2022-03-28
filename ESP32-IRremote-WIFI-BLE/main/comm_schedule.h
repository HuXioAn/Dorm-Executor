#ifndef _H_COMM_SCHEDULE_
#define _H_COMM_SCHEDULE_

#define IRremote_WIFI_SSID "the_ssid"
#define IRremote_WIFI_PASS "the_pwd"

#define IRremote_MQTT_URI "URI"
#define IRremote_MQTT_USR "the username"
#define IRremote_MQTT_PWD "the password"

#define WIFI_MAX_RETRY 3
#define MQTT_MAX_RETRY 3
#define BLE_CHECK_WIFI_IN_SECONDS 60 //在BLE模式下，检查WiFi可用的时间间隔

typedef enum
{
    COMM_MODE_WIFI = 0,
    COMM_MODE_BLE,
    COMM_MODE_MQTT
} COMM_MODE;

void wifi_init_sta(void);
void wifi_deinit_sta(void);
void mqtt_app_start(void);
void mqtt_app_stop(void);

#endif