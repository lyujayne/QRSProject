#ifndef __PAGE_HOME_H
#define __PAGE_HOME_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 表盘数据源：目前只有 RTC 是真数据，其余为桩值 */
typedef struct
{
    uint8_t  battery;   /* 电量 0~100% */
    uint16_t steps;     /* 今日步数 */
    uint8_t  heart;     /* 心率 bpm */
    int16_t  temp;      /* 温度，单位 0.1°C（265 = 26.5°C） */
} page_home_data_t;

void page_home_create(void);   /* 创建表盘 UI，必须在 LVGL 任务内调用 */
void page_home_refresh(void);  /* 周期性刷新（时间/日期/桩数据） */
void page_home_get_data(page_home_data_t *d);  /* 采集数据，传感器未接入时返回默认值 */

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_HOME_H */
