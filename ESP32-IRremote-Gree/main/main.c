#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "gree.h"
#include "ble.h"



static const char *TAG = "ESP32-IR-GREE";
void vTask_gree_control(void * pvParameters);
QueueHandle_t xQueue_Gree;


void app_main(void)
{
ble_init();

xQueue_Gree = xQueueCreate(3,20);
if(xQueue_Gree == NULL)abort();


TaskHandle_t Gree_task = NULL;
BaseType_t xReturned;

xReturned = xTaskCreate(
    vTask_gree_control,
    "gree_control",
    1024,
    NULL,
    5,
    &Gree_task
);
if(pdFALSE == xReturned)abort();


while(1){
    vTaskDelay(100);
}



}


void vTask_gree_control(void * pvParameters){
    
    char gree_cmd[25];
    while(1){
        if(pdTRUE==xQueueReceive(xQueue_Gree,gree_cmd,portMAX_DELAY)){
        remote_control(gree_cmd);
        }
    }
}