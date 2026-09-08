/**
 * @file    page_home.c
 * @brief   智能手表表盘（首页）
 *          分辨率 240x280（LVGL 逻辑坐标，OFFSET 已由 lcd.c 处理）
 *          时间/日期来自 RTC；步数/心率/电量/温度为桩数据，
 *          接入传感器后只需修改 page_home_read_sensors()
 */
#include "page_home.h"
#include "rtc.h"

/* ------------------------- 控件句柄 ------------------------- */
static lv_obj_t *lbl_time;      /* HH:MM 大数字（冒号每秒闪烁） */
static lv_obj_t *lbl_sec;       /* 秒 */
static lv_obj_t *lbl_date;      /* 2026/09/08 */
static lv_obj_t *lbl_week;      /* TUE */
static lv_obj_t *lbl_steps_val;
static lv_obj_t *lbl_heart_val;
static lv_obj_t *lbl_temp_val;
static lv_obj_t *lbl_battery;   /* 电量百分比 */
static lv_obj_t *lbl_bat_sym;   /* 电量图标 */

/* ------------------------- 缓存与常量 ------------------------- */
static uint8_t  s_last_day = 0xFF;   /* 日期变化才重绘日期 */
static uint8_t  s_last_sec = 0xFF;   /* 秒变化门控 */

/* HAL RTC 约定：WeekDay 1=周一 ... 7=周日 */
static const char *const s_week_tbl[7] =
{
    "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"
};

#define C_TIME_MAIN   lv_color_hex(0xFFFFFF)   /* 时间主色 */
#define C_TIME_ACCENT lv_color_hex(0x2FC6F6)   /* 秒/冒号强调色 */
#define C_CAPTION     lv_color_hex(0x8A8F98)   /* 状态栏/卡片标题灰 */
#define C_CARD_BORDER lv_color_hex(0x3A3F45)

/* ------------------------- 内部函数 ------------------------- */

/* 创建带默认样式的文本标签 */
static lv_obj_t *ui_label(lv_obj_t *parent, const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

/* 电池图标随电量切换 */
static const char *battery_symbol(uint8_t pct)
{
    if (pct >= 80) return LV_SYMBOL_BATTERY_FULL;
    if (pct >= 60) return LV_SYMBOL_BATTERY_3;
    if (pct >= 40) return LV_SYMBOL_BATTERY_2;
    if (pct >= 20) return LV_SYMBOL_BATTERY_1;
    return LV_SYMBOL_BATTERY_EMPTY;
}

/* 状态栏：左 WIFI/BT 图标，右电池 */
static void ui_build_status_bar(lv_obj_t *scr)
{
    lv_obj_t *lbl_wifi = ui_label(scr, &lv_font_montserrat_14, C_CAPTION);
    lv_label_set_text(lbl_wifi, LV_SYMBOL_WIFI " " LV_SYMBOL_BLUETOOTH);
    lv_obj_align(lbl_wifi, LV_ALIGN_TOP_LEFT, 10, 4);

    lbl_battery = ui_label(scr, &lv_font_montserrat_14, C_CAPTION);
    lv_obj_align(lbl_battery, LV_ALIGN_TOP_RIGHT, -10, 4);

    lbl_bat_sym = ui_label(scr, &lv_font_montserrat_14, C_CAPTION);
    lv_obj_align(lbl_bat_sym, LV_ALIGN_TOP_RIGHT, -62, 4);
}

/* 中央时钟：HH:MM + 闪烁冒号 + 秒 */
static void ui_build_clock(lv_obj_t *scr)
{
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, 240, 70);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, 4, 0);

    lbl_time = ui_label(cont, &lv_font_montserrat_32, C_TIME_MAIN);
    lv_label_set_text(lbl_time, "--:--");

    /* 秒：跟随 flex 行排在时间右侧，小字号+强调色 */
    lbl_sec = ui_label(cont, &lv_font_montserrat_14, C_TIME_ACCENT);
    lv_label_set_text(lbl_sec, "--");
}

/* 日期行：2026/09/08  TUE */
static void ui_build_date(lv_obj_t *scr)
{
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, 240, 24);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 140);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, 8, 0);

    lbl_date = ui_label(cont, &lv_font_montserrat_14, C_CAPTION);
    lbl_week = ui_label(cont, &lv_font_montserrat_14, C_TIME_ACCENT);
}

/* 底部数据卡片：STEPS / BPM / TEMP */
static lv_obj_t *ui_stat_card(lv_obj_t *parent, const char *caption)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_style_bg_opa(card, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, C_CARD_BORDER, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 2, 0);

    lv_obj_t *cap = ui_label(card, &lv_font_montserrat_14, C_CAPTION);
    lv_label_set_text(cap, caption);
    return card;
}

static void ui_build_stats(lv_obj_t *scr)
{
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, 220, 56);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, 8, 0);

    lv_obj_t *card;
    card = ui_stat_card(cont, "STEPS");
    lbl_steps_val = ui_label(card, &lv_font_montserrat_14, C_TIME_MAIN);

    card = ui_stat_card(cont, "BPM");
    lbl_heart_val = ui_label(card, &lv_font_montserrat_14, C_TIME_MAIN);

    card = ui_stat_card(cont, "TEMP");
    lbl_temp_val = ui_label(card, &lv_font_montserrat_14, C_TIME_MAIN);
}

/* ------------------------- 数据采集（桩） ------------------------- */

void page_home_get_data(page_home_data_t *d)
{
    /* 传感器/ADC 未接入：先返回默认值。
       接入后改为读 ADC(电量) / MPU6050(计步) / EM7028(心率) / AHT21(温度)。 */
    d->battery = 87;
    d->steps   = 6523;
    d->heart   = 72;
    d->temp    = 265;   /* 26.5°C */
}

/* ------------------------- 对外接口 ------------------------- */

void page_home_create(void)
{
    lv_obj_t *scr = lv_screen_active();

    /* 黑底 */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    ui_build_status_bar(scr);
    ui_build_clock(scr);
    ui_build_date(scr);
    ui_build_stats(scr);

    page_home_refresh();   /* 先画一帧 */
}

void page_home_refresh(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    page_home_data_t data;

    /* 注意：HAL 要求先 GetTime 再 GetDate（影子寄存器锁存顺序） */
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
        return;
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    /* 秒变化门控：每秒最多刷一次 */
    if (sTime.Seconds == s_last_sec)
        return;
    s_last_sec = sTime.Seconds;

    /* 时间：冒号每秒闪烁——偶数秒显示 ":"，奇数秒显示空格 */
    if (sTime.Seconds % 2 == 0)
        lv_label_set_text_fmt(lbl_time, "%02u:%02u", sTime.Hours, sTime.Minutes);
    else
        lv_label_set_text_fmt(lbl_time, "%02u %02u", sTime.Hours, sTime.Minutes);
    lv_label_set_text_fmt(lbl_sec, "%02u", sTime.Seconds);

    /* 日期：跨天（或首次）才重绘 */
    if (sDate.Date != s_last_day)
    {
        lv_label_set_text_fmt(lbl_date, "%04u/%02u/%02u",
                              sDate.Year + 2000, sDate.Month, sDate.Date);
        lv_label_set_text(lbl_week, s_week_tbl[sDate.WeekDay - 1]);
        s_last_day = sDate.Date;
    }

    /* 桩数据刷新（接传感器后此调用保留） */
    page_home_get_data(&data);
    lv_label_set_text_fmt(lbl_steps_val, "%u", data.steps);
    lv_label_set_text_fmt(lbl_heart_val, "%u", data.heart);
    lv_label_set_text_fmt(lbl_temp_val, "%d.%d°", data.temp / 10, data.temp % 10);
    lv_label_set_text_fmt(lbl_battery, "%u%%", data.battery);
    lv_label_set_text(lbl_bat_sym, battery_symbol(data.battery));
}

