/*
作者：胡小安
GitHub：https://github.com/HuXioAn
仓库地址：https://github.com/HuXioAn/Dorm-Executor

*/

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

#define CMD_LENGTH 67
#define CMD_MODE_BIT 0
#define CMD_SWITCH_BIT 3
#define CMD_TEMP_BIT 8
#define CMD_ELEC_HEAT_BIT 23
#define CMD_VERI_BIT 63

#define RMT_TX_GPIO 4

typedef enum AC_MODE_S
{
    GREE_AC_MODE_COOL = 0,
    GREE_AC_MODE_HEAT
} AC_MODE_T;



void generate_gree_cmd(uint8_t *cmd, uint8_t temp, int power, AC_MODE_T mode);

void level_implement(uint8_t level, rmt_item32_t *item, uint8_t invert);

int generate_gree_item(uint8_t cmd[], rmt_item32_t **items, uint8_t invert);

void send_gree(uint8_t temp, int power, AC_MODE_T mode);

void remote_control(const char *ctr_str);

#endif