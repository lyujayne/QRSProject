/**
 * @file lv_port_indev.c  (LVGL v9 输入设备移植 - CST816 触摸)
 * 用法：user_TasksInit.c 里 lv_port_disp_init() 之后调用 lv_port_indev_init()
 */
#if 1

#include "lv_port_indev.h"   /* 原模板是 lv_port_indev_template.h，工程里不存在，已改 */
#include "lvgl.h"
#include "CST816.h"

static void touchpad_init(void);
static void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);
static bool touchpad_is_pressed(void);
static void touchpad_get_xy(int32_t * x, int32_t * y);

lv_indev_t * indev_touchpad;

/* ================= 初始化 + 注册 ================= */
void lv_port_indev_init(void)
{
    touchpad_init();

    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);
}

/* ================= Touchpad ================= */

static void touchpad_init(void)
{
    CST816_GPIO_Init();   /* 复位脚 + I2C 脚 */
    CST816_RESET();       /* 上电复位时序：低 10ms → 高 100ms */
    CST816_Init();        /* 再写寄存器配置（如 5 秒自动睡眠） */
}

/* LVGL 每帧回调：读按压力和坐标 */
static void touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    static int32_t last_x = 0;
    static int32_t last_y = 0;

    if (touchpad_is_pressed()) {
        touchpad_get_xy(&last_x, &last_y);
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->point.x = last_x;   /* 松开时保持上次坐标，LVGL 自己处理 */
    data->point.y = last_y;
}

/* 是否按下：FingerNum 寄存器 0x02，0=无触摸，0xFF=睡眠态 */
static bool touchpad_is_pressed(void)
{
    uint8_t fn = CST816_Get_FingerNum();
    return (fn != 0x00) && (fn != 0xFF);
}

/* 读取坐标到 CST816_Instance，再转给 LVGL */
static void touchpad_get_xy(int32_t * x, int32_t * y)
{
    CST816_Get_XY_AXIS();
    *x = (int32_t)CST816_Instance.X_Pos;
    *y = (int32_t)CST816_Instance.Y_Pos;
}

#endif /*Enable this file at the top*/
