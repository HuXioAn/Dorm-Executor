#include "gree.h"
#include "stdlib.h"
#include "esp_log.h"


static const char *TAG = "GREE";



int generate_gree_item(uint8_t cmd[],rmt_item32_t* item,uint8_t invert){
    if(item=calloc(35,sizeof(rmt_item32_t))){



    }else{
        ESP_LOGE(TAG,"ERROR allocat memory for cmd items.");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
}

void level_implement(uint8_t level,rmt_item32_t* item,uint8_t invert){
    if(0==level){
        item->
    }
}
