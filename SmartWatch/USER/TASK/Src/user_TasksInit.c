/**
 * @file    user_TasksInit.c
 * @brief   任务集中创建 + LVGL 专属任务（参考 OV_Watch 结构，适配 LVGL v9）
 *          核心原则：全工程只有 LvHandlerTask 调用 lv_timer_handler()，
 *          其他任务想动界面，只能通过消息队列发消息给它。
 */

#include "user_TasksInit.h"      /* 本模块头文件（含 cmsis_os.h） */
// #include "user_KeyTask.h"        /* 声明 KeyTask 函数，等下 osThreadNew 要用 */
#include "main.h"                /* HAL 库入口：HAL_GetTick() 在这里声明 */
#include "lcd_init.h"            /* 背光开关：LCD_Open_Light / LCD_Close_Light */
#include "lvgl.h"                /* LVGL 库 */
#include "lv_port_disp.h"        /* 屏幕驱动入口：lv_port_disp_init() */
#include "page_home.h"           /* 表盘：page_home_create() / page_home_refresh() */

osThreadId_t LvHandlerTaskHandle;
const osThreadAttr_t LvHandlerTask_attributes = {
    .name = "LvHandlerTask",
    .stack_size = 1024*4,
    .priority = (osPriority_t) osPriorityLow , /* 最低优先级：渲染最耗时，不能抢按键/传感器的执行权 */
};

osThreadId_t KeyTaskHandle;
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 128 * 4,                     /* 512B = 128 字，正好等于最小栈要求 */
  .priority = (osPriority_t) osPriorityNormal, /* 普通优先级：按键要响应及时，高于 LVGL */
};


osMessageQueueId_t Key_MessageQueue;         /* 按键事件：KeyTask 投，LvHandlerTask 收 */
osMessageQueueId_t HomeUpdata_MessageQueue;  /* 首页刷新请求：预留（阶段2 SensorDataUpdateTask 用） */

void User_Tasks_Init(void);
void LvHandlerTask(void *argument);
static void OnKeyPress(void);


void User_Tasks_Init(void)
{
    /* ① 创建消息队列：
     *    osMessageQueueNew(槽数量, 每个槽字节数, 属性(NULL=用默认))
     *    按键队列开 4 槽：消抖/将来长按、连击有富余；
     *    首页刷新队列 1 槽：谁要刷首页，投一条即可 */
    Key_MessageQueue        = osMessageQueueNew(4, 1, NULL);
    HomeUpdata_MessageQueue = osMessageQueueNew(1, 1, NULL);

    /* ② 创建任务：
     *    osThreadNew(任务函数, 传给函数的参数(NULL=不传), 属性结构体地址)
     *    任务创建后立即进入就绪态，等 osKernelStart() 一声令下同时开跑 */
    // KeyTaskHandle       = osThreadNew(KeyTask, NULL, &KeyTask_attributes);
    LvHandlerTaskHandle = osThreadNew(LvHandlerTask, NULL, &LvHandlerTask_attributes);
}


void LvHandlerTask(void *argument)
{
    uint8_t msg;   /* 接收队列消息的临时变量 */

    /* ---------- 一次性初始化（任务一启动就跑一次） ---------- */
    lv_init();                     /* ① LVGL 库自身初始化：内部内存池、样式、主题等 */
    lv_tick_set_cb(HAL_GetTick);   /* ② 告诉 LVGL："读时间用 HAL_GetTick()"。
                                    *    动画/刷新节拍全靠它，不设置动画不会走 */
    lv_port_disp_init();           /* ③ 屏幕驱动：初始化 ST7789、注册绘制回调、申请双缓冲 */
    page_home_create();            /* ④ 创建表盘：时间/日期/状态栏/三张卡片，全在 page_home.c 里 */

    /* ---------- 主循环：每 5ms 转一圈 ---------- */
    for (;;)
    {
        /* ① 非阻塞收队列消息：timeout=0 表示"没有消息立刻返回，不傻等"。
         *    收到 KEY_MSG_PRESS → 切背光。
         *    以后阶段4 按键消息在这里换成跳页面。 */
        if (osMessageQueueGet(Key_MessageQueue, &msg, NULL, 0) == osOK)
        {
            if (msg == KEY_MSG_PRESS)
            {
                OnKeyPress();
            }
        }

        /* ② 刷新表盘数据：内部按"秒变化"门控（RTC 秒没变就直接返回），开销极小 */
        page_home_refresh();

        /* ③ LVGL 执行一帧：把上面所有控件状态的变化真正绘制到屏幕。
         *    画完这一帧它自己会算好下次该什么时候画（默认 33ms 刷新周期）。
         *    这条语句只能出现在这个任务里，多任务同时调会死锁/花屏。 */
        lv_timer_handler();

        /* ④ 睡 5ms，把 CPU 让给其他任务（尤其 KeyTask） */
        osDelay(5);
    }
}

/* 按键动作：阶段1 先做背光开关，验证"按键→队列→LVGL任务"链路 */
static uint8_t s_backlight_on = 1;
static void OnKeyPress(void)
{
    if (s_backlight_on)
    {
        LCD_Close_Light();
        s_backlight_on = 0;
    }
    else
    {
        LCD_Open_Light();
        s_backlight_on = 1;
    }
}

