#ifndef __SYS_H__
#define __SYS_H__

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#define OS_SUPPORT 	1 // 不开启操作系统支持，使用裸机版本延时。如果要跑 FreeRTOS 改成`1`。
#define SYS_CLK 100 //系统时钟，单位 **MHz**。这里写 100，代表你的 STM32F4 主频配置为 100MHz。

typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef const int32_t sc32;  
typedef const int16_t sc16;  
typedef const int8_t sc8;  

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;  
typedef __I int16_t vsc16; 
typedef __I int8_t vsc8;   

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef const uint32_t uc32;  
typedef const uint16_t uc16;  
typedef const uint8_t uc8; 

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;  
typedef __I uint16_t vuc16; 
typedef __I uint8_t vuc8; 

/*
- `__IO`：HAL 库宏，等价于 `volatile`，易变变量，用于硬件寄存器。告诉编译器不要优化这个变量，每次都要真实读内存，不能缓存到寄存器。
- `__I`：只读，`const volatile`，只读寄存器。
*/

#endif
