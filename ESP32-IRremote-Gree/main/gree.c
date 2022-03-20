#include "gree.h"
#include "stdlib.h"
#include "esp_log.h"

static const char *TAG = "GREE";

int generate_gree_item(uint8_t cmd[], rmt_item32_t **items, uint8_t invert)
{   
    rmt_item32_t *item=NULL;
    if ((item = calloc(200, sizeof(rmt_item32_t))))
    {
        //起始码
        item[0].duration0=9000;
        item[0].level0=!!invert;
        item[0].duration1=4500;
        item[0].level1=!invert;

        for(int i=0;i<35;i++){
            level_implement(cmd[i],&item[i+1],invert);
        }
        //连接码
        item[36].duration0=600;
        item[36].level0=!!invert;
        item[36].duration1=20000;
        item[36].level1=!invert;

        for(int i=35;i<67;i++){
            level_implement(cmd[i],&item[i+2],invert);
        }
        item[69].duration0=600;
        item[69].level0=!!invert;
        item[69].duration1=20000;
        item[69].level1=!invert;
        //40ms
        item[70].duration0=10000;
        item[70].level0=!invert;
        item[70].duration1=10000;
        item[70].level1=!invert;




        //起始码
        item[71].duration0=9000;
        item[71].level0=!!invert;
        item[71].duration1=4500;
        item[71].level1=!invert;

        for(int i=67;i<102;i++){
            level_implement(cmd[i],&item[i+5],invert);
        }
        //连接码
        item[107].duration0=600;
        item[107].level0=!!invert;
        item[107].duration1=20000;
        item[107].level1=!invert;

        for(int i=102;i<134;i++){
            level_implement(cmd[i],&item[i+6],invert);
        }
        item[140].duration0=600;
        item[140].level0=!!invert;
        item[140].duration1=0;
        item[140].level1=!invert;
        // //77ms
        // item[141].duration0=30000;
        // item[141].level0=!invert;
        // item[141].duration1=30000;
        // item[141].level1=!invert;       
        
        // for(int i=134;i<177;i++){
        //     level_implement(cmd[i],&item[i+8],invert);
        // }

        // item[185].duration0=600;
        // item[185].level0=!!invert;
        // item[185].duration1=0;
        // item[185].level1=!invert; 


        *items=item;
        return 141;
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




