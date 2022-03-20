#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "gree.h"


static const char *TAG = "ESP32-IR-GREE";

#define RMT_TX_GPIO 4

void app_main(void)
{
    uint8_t cmd[177] = {    1,0,0,1,0,0,0,0,1,
                            0,0,1,0,0,0,0,0,0,
                            0,0,0,1,0,0,0,0,0,0,
							1,0,1,0,0,1,0,
                            //20ms
                            0,0,0,0,0,0,0,0,0,
							0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,
                            0,0,0,1,1,
                            //40ms
                            1,0,0,1,0,0,0,0,1,
                            0,0,1,0,0,0,0,0,0,
                            0,0,0,1,0,0,0,0,0,0,
							1,1,1,0,0,1,0,
                            //20ms
                            0,0,0,0,0,0,0,0,0,
							0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,
                            0,0,0,1,1,
                            //77ms
                            1,0,0,1,0,0,0,0,1,
                            0,0,1,0,0,0,0,0,0,
                            0,0,0,1,0,0,0,0,0,0,
							0,0,0,0,0,0,0,0,
                            1,0,1,0,0,1,0
                            
                            
                            
                            
                            };

    uint8_t cmdo[67] = {1,0,0,0,0,0,1,
							0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,
							1,0,1,0,0,1,0,1,0,0,0,0,0,0,0,0,
							0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
							0,1,0}; 

    rmt_item32_t* items=NULL;
    int length = generate_gree_item(cmd,&items,1);
    ESP_LOGI(TAG,"items generated.\n");
    rmt_config_t rmt_tx_config = RMT_DEFAULT_CONFIG_TX(RMT_TX_GPIO,RMT_CHANNEL_0);
    rmt_tx_config.tx_config.carrier_en = true;
    //rmt_tx_config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
    //rmt_tx_config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    rmt_config(&rmt_tx_config);
    rmt_driver_install(RMT_CHANNEL_0, 0, 0);

    vTaskDelay(1000/portTICK_PERIOD_MS);
    rmt_write_items(RMT_CHANNEL_0,items,length,true);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    free(items);
    rmt_driver_uninstall(RMT_CHANNEL_0);
    ESP_LOGI(TAG,"RMT DRIVER UNINSTALLED.\n");

}
