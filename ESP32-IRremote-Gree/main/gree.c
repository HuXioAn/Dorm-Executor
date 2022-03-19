#include "gree.h"
#include "stdlib.h"
#include "esp_log.h"

static const char *TAG = "GREE";

int generate_gree_item(uint8_t cmd[], rmt_item32_t **items, uint8_t invert)
{   
    rmt_item32_t *item=NULL;
    if ((item = calloc(70, sizeof(rmt_item32_t))))
    {
        //起始码
        item[0].duration0=9000;
        item[0].level0=!!invert;
        item[0].duration1=4500;
        item[0].level1=!invert;

        for(int i=0;i<32;i++){
            level_implement(cmd[i],&item[i+1],invert);
        }
        //连接码
        item[33].duration0=600;
        item[33].level0=!!invert;
        item[33].duration1=20000;
        item[33].level1=!invert;

        for(int i=32;i<67;i++){
            level_implement(cmd[i],&item[i+2],invert);
        }
        *items=item;
        return 69;
    }
    else
    {
        ESP_LOGE(TAG, "ERROR allocat memory for cmd items.\n");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    return -1;
}

void level_implement(uint8_t level, rmt_item32_t *item, uint8_t invert)
{
    if (0 == level)
    {
        item->duration0=RMT_LOW_LEVEL_1;
        item->level0=!!invert;
        item->duration1=RMT_LOW_LEVEL_2;
        item->level1=!invert;
    }else if(1== level){
        item->duration0=RMT_HIGH_LEVEL_1;
        item->level0=!!invert;
        item->duration1=RMT_HIGH_LEVEL_2;
        item->level1=!invert;
    }
}




