/**
 ******************************************************************************
 * @file    board_cfg.h  (模板)
 * @brief   集中式板级引脚配置表 —— 从零实现时建议新增的"唯一改引脚的地方"
 *
 * 【为什么要有这个文件】
 *   原 OV-Watch 工程没有集中引脚表：LCD/按键/电源引脚散在各自驱动头文件，
 *   I2C 器件的 SDA/SCL 散在各自 .c 的 iic_bus_t 实例里，SPI/UART 的 AF 又在
 *   CubeMX 生成的 MspInit 里 —— 换一块板子要改七八个文件。
 *   本模板把全部可改引脚收拢到一个文件：以后换板子只改这里。
 *
 * 【用法】
 *   1. 复制本文件到你的工程（建议放 BSP/ 目录，如 BSP/board_cfg.h），
 *      "把所有宏改成你开发板的实际接线"。
 *   2. 各驱动 .c/.h 统一 #include "board_cfg.h"，禁止再在别处硬编码引脚。
 *   3. 本文件内所有默认值 = 原 OV-Watch 参考引脚，仅作对照，不代表你的接线！
 *
 * 【前提】
 *   需要先包含 STM32 头文件（宏 GPIOA / GPIO_PIN_x 等来自 stm32f4xx.h /
 *   stm32f4xx_hal.h），建议在各驱动 include 完 HAL 后再 include 本文件；
 *   或在本文件顶部自行 #include "main.h"（若你的工程有）。
 ******************************************************************************
 */
# include "main.h"

#ifndef __BOARD_CFG_H
#define __BOARD_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 0) 用到的 GPIO 时钟（建议集中在"板级初始化函数"里一次开齐，
 *    下面列出的是"可能用到"的端口，按实际器件删减）
 * ----------------------------------------------------------------------------
 * 推荐在你的 BSP 初始化处（如 Board_Init()）这样写：
 *
 *   __HAL_RCC_GPIOA_CLK_ENABLE();   // 用到了 PA 口的器件就保留
 *   __HAL_RCC_GPIOB_CLK_ENABLE();   // PB
 *   __HAL_RCC_GPIOC_CLK_ENABLE();   // PC
 *   __HAL_RCC_GPIOD_CLK_ENABLE();   // PD
 *
 * 或者你希望更省事，把下面 4 个宏在板级 init 里展开（等价于上面 4 行）：
 * ==========================================================================*/
#define BOARD_GPIO_CLK_ENABLE()         \
    do {                                \
        __HAL_RCC_GPIOA_CLK_ENABLE();   \
        __HAL_RCC_GPIOB_CLK_ENABLE();   \
        __HAL_RCC_GPIOC_CLK_ENABLE();   \
        __HAL_RCC_GPIOD_CLK_ENABLE();   \
    } while (0)

/* ============================================================================
 * 1) 显示屏 ST7789（最重要，先把这块调通）
 * ----------------------------------------------------------------------------
 * 两种数据通路，二选一：
 *   A. 硬件 SPI（推荐，快）：SCL/MOSI 用 SPI1 复用脚，需同时在 CubeMX 或
 *      spi.c 里配置 AF（参考：PB3=SPI1_SCK, PB5=SPI1_MOSI, Mode3, 50MHz）；
 *   B. 软件模拟 SPI：任意两个 GPIO 即可，最稳、不依赖 SPI 外设。
 * 首版建议 LCD_USE_SOFT_SPI 设 1（先点亮再说），跑通后再改硬件 SPI。
 * ==========================================================================*/
#define LCD_USE_SOFT_SPI        1   /* 1=软件SPI(用下面SCL/SDA两脚) 0=硬件SPI1(PB3/PB5) */

/* --- LCD 时钟/数据线（软件SPI时必填；硬件SPI时这两行仅作记录）--- */
#define LCD_SCL_PORT            GPIOB      /* 原参考:PB3=SPI1_SCK  | 你的板子:______ */
#define LCD_SCL_PIN             GPIO_PIN_3
#define LCD_SDA_PORT            GPIOB      /* 原参考:PB5=SPI1_MOSI | 你的板子:______ */
#define LCD_SDA_PIN             GPIO_PIN_5

/* --- LCD 控制线（普通推挽 GPIO，必填）--- */
#define LCD_RES_PORT            GPIOB      /* 原参考:PB4  | 你的板子:______ */
#define LCD_RES_PIN             GPIO_PIN_4
#define LCD_DC_PORT             GPIOC      /* 原参考:PC12 | 你的板子:______ */
#define LCD_DC_PIN              GPIO_PIN_12
#define LCD_CS_PORT             GPIOD      /* 原参考:PD2  | 你的板子:______ */
#define LCD_CS_PIN              GPIO_PIN_2

/* --- 背光 ---
 * 1 = TIM PWM 调光（需配一个 TIM 通道，参考:PA15=TIM2_CH1，ARR300/PSC99，占空比5~100%）
 * 0 = 普通 GPIO 开关（先这样用，验证画面最简单）  */
#define LCD_BL_MODE_PWM         0           /* 0=先GPIO常亮 1=后改PWM */
#define LCD_BL_PORT             GPIOA       /* 原参考:PA15 | 你的板子:______ */
#define LCD_BL_PIN              GPIO_PIN_15

/* ============================================================================
 * 2) 按键（至少 1 个；建议 2 个方便返回/回主页）
 *    按键通常"上拉输入、按下为低"；想支持 STOP 唤醒再配 EXTI 下降沿。
 * ==========================================================================*/
#define KEY1_PORT               GPIOA       /* 原参考:PA0  | 你的板子:______ */
#define KEY1_PIN                GPIO_PIN_0
#define KEY1_ACTIVE_LOW         1           /* 1=按下为低 */

/* 第二颗键（可选；没有就保留宏但不用） */
#define KEY2_PORT               GPIOA       /* 原参考:无(宏恒0) | 你的板子:______ */
#define KEY2_PIN                GPIO_PIN_1
#define KEY2_ACTIVE_LOW         1

/* ============================================================================
 * 3) 电源/电池（可选，裸核心板先全不接，宏保留不用即可）
 * ==========================================================================*/
#define BAT_CHECK_PORT          GPIOA       /* 电池分压采样脚(配ADC) | 原:PA1 */
#define BAT_CHECK_PIN           GPIO_PIN_1
#define CHARGE_PORT             GPIOA       /* 充电状态检测 | 原:PA2 */
#define CHARGE_PIN              GPIO_PIN_2
#define POWER_PORT              GPIOA       /* 电源锁存开关(拉低=断电) | 原:PA3 */
#define POWER_PIN               GPIO_PIN_3

/* ============================================================================
 * 4) I2C 器件引脚（软件 I2C，所有器件的 SDA/SCL 都在这统一改）
 * ----------------------------------------------------------------------------
 * 设计建议：不必照抄原工程"每个器件一对引脚"。你的板子没有那么多冲突时，
 * 把所有 I2C 器件挂到【同一组 SDA/SCL】即可（器件靠 7bit 地址区分），
 * 例如全挂 TOUCH 那组 —— 把下面各组都填成同一对引脚。
 * 每组仍需给独立 iic_bus_t 实例（或共用一个实例），这是驱动层的事。
 * ==========================================================================*/

/* 4.1 触摸 CST816（屏带触摸才有；地址 0x15） */
#define CST816_SDA_PORT         GPIOB       /* 原参考:PB7 | 你的板子:______ */
#define CST816_SDA_PIN          GPIO_PIN_7
#define CST816_SCL_PORT         GPIOB       /* 原参考:PB6 | 你的板子:______ */
#define CST816_SCL_PIN          GPIO_PIN_6
#define CST816_RST_PORT         GPIOB       /* 原参考:PB8 | 你的板子:______ */
#define CST816_RST_PIN          GPIO_PIN_8
#define CST816_INT_PORT         GPIOB       /* 中断脚(先不用,仅记录) 原:PB9 */
#define CST816_INT_PIN          GPIO_PIN_9

/* 4.2 温湿度 AHT21（地址 0x38） */
#define AHT21_SDA_PORT          GPIOC       /* 原参考:PC9 | 你的板子:______ */
#define AHT21_SDA_PIN           GPIO_PIN_9
#define AHT21_SCL_PORT          GPIOA       /* 原参考:PA8 | 你的板子:______ */
#define AHT21_SCL_PIN           GPIO_PIN_8

/* 4.3 气压计 SPL06-001（地址 0x76） */
#define SPL06_SDA_PORT          GPIOB       /* 原参考:PB13 | 你的板子:______ */
#define SPL06_SDA_PIN           GPIO_PIN_13
#define SPL06_SCL_PORT          GPIOB
#define SPL06_SCL_PIN           GPIO_PIN_14

/* 4.4 IMU MPU6050（地址 0x68） */
#define MPU6050_SDA_PORT        GPIOB       /* 原参考:PB13 | 你的板子:______ */
#define MPU6050_SDA_PIN         GPIO_PIN_13
#define MPU6050_SCL_PORT        GPIOB
#define MPU6050_SCL_PIN         GPIO_PIN_14
#define MPU6050_INT_PORT        GPIOB       /* 中断脚(可选) 原参考:PB12 */
#define MPU6050_INT_PIN         GPIO_PIN_12

/* 4.5 罗盘 LSM303DLH（加速 0x19 / 磁力 0x1E） */
#define LSM303_SDA_PORT         GPIOB       /* 原参考:PB13 | 你的板子:______ */
#define LSM303_SDA_PIN          GPIO_PIN_13
#define LSM303_SCL_PORT         GPIOB
#define LSM303_SCL_PIN          GPIO_PIN_14

/* 4.6 心率 EM7028（地址 0x24） */
#define EM7028_SDA_PORT         GPIOB       /* 原参考:PB13 | 你的板子:______ */
#define EM7028_SDA_PIN          GPIO_PIN_13
#define EM7028_SCL_PORT         GPIOB
#define EM7028_SCL_PIN          GPIO_PIN_14

/* 4.7 EEPROM BL24C02（地址 0x50，存设置/步数） */
#define BL24C02_SDA_PORT        GPIOA       /* 原参考:PA11 | 你的板子:______ */
#define BL24C02_SDA_PIN         GPIO_PIN_11
#define BL24C02_SCL_PORT        GPIOA
#define BL24C02_SCL_PIN         GPIO_PIN_12

/* ============================================================================
 * 5) 由 CubeMX 管理、无需在此定义的引脚（只作记录，防止你重复占用）：
 *    - UART1: PA9=TX / PA10=RX        （在 usart.c MspInit 配 AF）
 *    - SPI1 : PB3=SCK / PB5=MOSI       （若用硬件 SPI，在 spi.c MspInit 配 AF）
 *    - SWD  : PA13=SWDIO / PA14=SWCLK  （下载口，别挪作他用）
 *    - LSE  : PC14/PC15                （32.768kHz 给 RTC，可选）
 *    - LED  : PC13                     （心跳灯，占位演示用）
 * ==========================================================================*/

/* ============================================================================
 * 6) 填写后自查清单（重要！）
 *    [ ] 1. 用你的原理图/杜邦线实物，逐条核对上面每一个 PORT/PIN；
 *    [ ] 2. 检查冲突：同一引脚不能同时给两个外设
 *          （例如你如果用了硬件 SPI1=PB3/PB5，软件 SPI 那两行应忽略/置不同脚）；
 *    [ ] 3. 触摸/传感器/EEPROM 若没有该硬件：宏保留不动即可（驱动不初始化它），
 *          但注意别把它们的引脚复用给别的器件；
 *    [ ] 4. 若屏幕模组不带触摸，CST816 整组留空不接；
 *    [ ] 5. GPIO 时钟：在板级初始化调用一次 BOARD_GPIO_CLK_ENABLE()。
 * ==========================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_CFG_H */
