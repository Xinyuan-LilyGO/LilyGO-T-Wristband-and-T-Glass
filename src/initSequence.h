/**
 * @file      initSequence.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2023-05-29
 *
 */
#pragma once

#include <stdint.h>

typedef struct {
    uint32_t addr;
    uint8_t param[20];
    uint32_t len;
} lcd_cmd_t;

#define AMOLED_DEFAULT_BRIGHTNESS               175

#define JD9613_INIT_SEQUENCE_LENGTH             88
extern const lcd_cmd_t jd9613_cmd[JD9613_INIT_SEQUENCE_LENGTH];
#define JD9613_WIDTH                            126
#define JD9613_HEIGHT                           294










