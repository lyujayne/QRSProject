/**
 * @file    user_TasksInit.h
 * @brief   任务初始化模块的头文件
 *          作用：把"任务句柄、消息队列"这些跨文件共享的东西声明出来，
 *          别的 .c 文件只要 #include 这个头文件，就能用这些变量。
 *          （就像 page_home.h 声明 page_home_create() 一样）
 */
#ifndef __USER_TASKSINIT_H       // 防止重复包含的经典写法：
#define __USER_TASKSINIT_H       // 如果这个宏没定义过，就定义它并继续；下次再包含时，宏已存在，直接跳过整个文件

#include "cmsis_os.h" //写任务代码必须包含它。

#ifdef __cplusplus               // 如果被 C++ 文件包含，用 C 的链接规则（防止函数名被 C++ 改写）
extern "C" {
#endif
 
/* ------------------------------------------------------------------
 * 任务句柄：osThreadId_t 本质是 void*，指向 FreeRTOS 创建的任务控制块(TCB)。
 * extern 表示"变量定义在别的 .c 文件里（user_TasksInit.c），这里只是声明"。
 * 有了它，其他文件可以随时拿到任务的状态（比如调试时挂起/恢复这个任务）。
 * ------------------------------------------------------------------ */
extern osThreadId_t LvHandlerTaskHandle;   /* LVGL 渲染任务 */
extern osThreadId_t KeyTaskHandle;         /* 按键扫描任务 */

/* ------------------------------------------------------------------
 * 消息队列：任务间通信的"信箱"。
 * extern 声明 + user_TasksInit.c 里定义 → KeyTask 往里投，LvHandlerTask 往外取
 * ------------------------------------------------------------------ */
extern osMessageQueueId_t Key_MessageQueue;          /* 按键事件队列，消息值=KEY_MSG_* */
extern osMessageQueueId_t HomeUpdata_MessageQueue;   /* 首页数据刷新请求队列（阶段2/3 才用，先建好占位） */

/* 按键消息值定义：队列里传的就是这个数字 */
#define KEY_MSG_PRESS   1   /* 短按 */

/* 任务创建入口：由 freertos.c 的 MX_FREERTOS_Init() 在调度器启动前调用 */
void User_Tasks_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_TASKSINIT_H */
