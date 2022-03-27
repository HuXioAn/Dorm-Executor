#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "gree.h"
#include "comm_schedule.h"

/*
整合WIFI+MQTT与BLE两种通信方式，并可在运行时禁用、启用其中一个或者两个。
WIFI优先级高于BLE，因为BLE的创建总是可行的，所以在BLE模式下会定期检查WIFI是否可用，如可用，立即切换。
*/

static void WIFI_STA_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
QueueHandle_t xQueue_Mode;

void app_main(void)
{
    xQueue_Mode = xQueueCreate(5, sizeof(COMM_MODE));
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_sta();
}

void mode_schedule_task(void *pvParameters)
{
    COMM_MODE mode;
    while (1)
    {
        if (pdTRUE == xQueueReceive(xQueue_Mode, &mode, portMAX_DELAY))
        {
            if (mode == COMM_MODE_BLE)
            {
                //切换到BLE模式，首先为WIFI，MQTT收拾残局，再开启BLE初始化
            }
            else if (mode == COMM_MODE_WIFI)
            {
                //同理，关闭BLE，开启WiFi
            }else if(mode == COMM_MODE_MQTT){
                //WIFI连接成功，获得IP，开启mqtt
            }
            else
            {
                abort();
            }
        }
    }
}

void wifi_init_sta(void)
{

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &WIFI_STA_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &WIFI_STA_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = IRremote_WIFI_SSID,
            .password = IRremote_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

static void WIFI_STA_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{   
    static int s_retry_num=0;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    { //协议栈开启
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    { //断开事件,会在连接失败和因意外断开时发生
      //在本应用中不会主动断开，所以发生事件就要重连
        if (s_retry_num < WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {//重连次数到达上限，准备切换蓝牙
            COMM_MODE to_ble=COMM_MODE_BLE;
            xQueueSend(xQueue_Mode,&to_ble,100/portTICK_PERIOD_MS);
        }
        
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        //连接后得到IP，在此事件以后才能建立TCP连接,通知MQTT初始化。
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        COMM_MODE to_ble=COMM_MODE_MQTT;//开启MQTT
        xQueueSend(xQueue_Mode,&to_ble,100/portTICK_PERIOD_MS);
        
    }
}

void wifi_deinit_sta(void)
{
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}


static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
        .username = IRremote_MQTT_USR,
        .password = IRremote_MQTT_PWD,

    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}