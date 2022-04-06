#ifndef _H_COMM_SCHEDULE_
#define _H_COMM_SCHEDULE_

#define IRremote_WIFI_SSID "the_ssid"       //要连接到的WIFI名称
#define IRremote_WIFI_PASS "the_pwd"        //对应的密码
#define WIFI_HOSTNAME "ESP32-IRremote"      //在局域网中的主机名，可以在路由器后台看到

#define IRremote_MQTT_URI "URI"             //MQTT服务器地址，示例：mqtt://xxx.xxx.xxx.xxx
#define IRremote_MQTT_USR "the username"    //MQTT 用户名
#define IRremote_MQTT_PWD "the password"    //对应密码

#define WIFI_MAX_RETRY 3                    //意外断开、连接失败时连接重试的上限次数
#define MQTT_MAX_RETRY 3                    //意外断开、连接失败时连接重试的上限次数
#define BLE_CHECK_WIFI_IN_SECONDS 300       //在BLE模式下，检查WiFi可用的时间间隔
#define BLE_CHECK_WIFI_IN_MS (BLE_CHECK_WIFI_IN_SECONDS*1000)

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

void vTask_IRremote_control(void *pvParameters);
void mode_schedule_task(void *pvParameters);
#endif