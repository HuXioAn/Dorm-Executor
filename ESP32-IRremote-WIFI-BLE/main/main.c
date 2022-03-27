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
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
QueueHandle_t xQueue_Mode;
static esp_mqtt_client_handle_t mqtt_client;
int fail_cause;

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
                wifi_deinit_sta();
                mqtt_app_stop();
                fail_cause=0;
                //WiFi、mqtt清理完毕，下面开启蓝牙
                


            }
            else if (mode == COMM_MODE_WIFI)
            {
                //同理，关闭BLE，开启WiFi
            }else if(mode == COMM_MODE_MQTT){
                //WIFI连接成功，获得IP，开启mqtt
                mqtt_app_start();
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
        fail_cause=1;
        if (s_retry_num < WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {//重连次数到达上限，准备切换蓝牙,但要注意，在已连接过程中的断开同样会触发MQTT的断开事件，要区分两者
            
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


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{   
    static int s_retry_num=0;
    //ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        msg_id = esp_mqtt_client_subscribe(client,"ac",0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        s_retry_num=0;//重试次数清零
        break;
    case MQTT_EVENT_DISCONNECTED:
        //当WiFi断开，MQTT断开时都会触发此事件，所以加入原因标志来防止重复向队列发送
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

        if (s_retry_num < MQTT_MAX_RETRY)
        {
            esp_mqtt_client_reconnect(mqtt_client);
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the MQTT Broker");
        }
        else
        {
            if(!fail_cause){//如果错误的原因不是来自WIFI
            COMM_MODE to_ble=COMM_MODE_BLE;
            xQueueSend(xQueue_Mode,&to_ble,100/portTICK_PERIOD_MS);
            }
        }
        
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        //printf("DATA=%.*s\r\n", event->data_len, event->data);
        if((event->topic)[0]=='a'&&(event->topic)[1]=='c'){
            //修改为任务
            ESP_LOGI(TAG,"GREE CONTROL!");
            remote_control(event->data);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = IRremote_MQTT_URI,
        .username = IRremote_MQTT_USR,
        .password = IRremote_MQTT_PWD,

    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}


static void mqtt_app_stop(void){
    ESP_ERROR_CHECK(esp_mqtt_client_disconnect(mqtt_client));
    ESP_ERROR_CHECK(esp_mqtt_client_stop(mqtt_client));
    //？？？没有unregister event handler
    ESP_ERROR_CHECK(esp_mqtt_client_destroy(mqtt_client));
    
}

















