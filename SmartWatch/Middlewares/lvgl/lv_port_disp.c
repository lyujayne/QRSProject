/**
 * @file lv_port_disp.c  (LVGL v9 显示移植 - 复用 BSP/LCD 的 LCD_Color_Fill + DMA 回调)
 */
#if 1

#include "lv_port_disp.h"
#include "lcd.h"
#include "lcd_init.h"
#include "spi.h"
#include "tim.h"

static lv_display_t * current_disp = NULL;

/* DMA 完成回调（由 lcd.c 的 lcd_ready_cb 转调） */
static void lcd_flush_ready_callback(void)
{
    if (current_disp != NULL)
    {
        lv_display_flush_ready(current_disp);
        current_disp = NULL;
    }
}

/* 屏初始化：LCD_Init + 背光 + 注册回调 */
static void disp_init(void)
{
    LCD_Init();                                  /* ST7789 初始化序列 + GPIO */
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 500);  /* 背光 50% (ARR=999) */
    LCD_Open_Light();
    LCD_Set_Flush_Complete_Callback(lcd_flush_ready_callback);
}

/* LVGL flush：直接调 LCD_Color_Fill（内部已加 OFFSET_Y=20 + 字节序转换） */
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    current_disp = disp;
    LCD_Color_Fill(area->x1, area->y1, area->x2, area->y2, (u16 *)px_map);
}

void lv_port_disp_init(void)
{
    disp_init();

    lv_display_t * disp = lv_display_create(LCD_W, LCD_H);   /* 240 x 280 */
    lv_display_set_flush_cb(disp, disp_flush);

    /* 双缓冲 partial 渲染（/10 行，约 13KB x 2） */
    static uint8_t buf1[LCD_W * LCD_H / 10 * 2];
    static uint8_t buf2[LCD_W * LCD_H / 10 * 2];
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
}

#else
typedef int keep_pedantic_happy;
#endif
