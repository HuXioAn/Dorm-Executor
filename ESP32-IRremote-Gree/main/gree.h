#ifndef _H_GREE_
#define _H_GREE_

#include "driver/rmt.h"

#define RMT_START_LEVEL_1 9000
#define RMT_START_LEVEL_2 4500

#define RMT_LOW_LEVEL_1 600
#define RMT_LOW_LEVEL_2 600
#define RMT_HIGH_LEVEL_1 600
#define RMT_HIGH_LEVEL_2 1600

#define RMT_CONNECT_LEVEL_1 600
#define RMT_CONNECT_LEVEL_2 20000




int generate_gree_item(uint8_t cmd[],rmt_item32_t** items,uint8_t invert);

void level_implement(uint8_t level,rmt_item32_t* item,uint8_t invert);




#endif