#include "sys.h"
#include "delay.h"


#if OS_SUPPORT
#include "FreeRTOS.h"
#include "task.h"

uint8_t delay_osrunning = 0;
u16     fac_ms = 0;

void delay_osschedlock(void)
{
    vTaskSuspendAll();
}

void delay_osschedunlock(void)
{
    xTaskResumeAll();
}

void delay_ostimedly(u32 ticks)
{
    vTaskDelay(ticks);
}
#endif


void delay_init(void)
{
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK); // 设置 SysTick 时钟源 = **HCLK 内核时钟**（不是 HCLK/8）
    HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq));
    #if OS_SUPPORT
    // fac_ms：一个RTOS节拍等于多少ms
    fac_ms = (u16)(1000U / configTICK_RATE_HZ);
    #endif

}

#if OS_SUPPORT
/**
 * freertos 的 延时函数
 */

void delay_us(u32 nus)
{
    u32 ticks;
    u32 told, tnow, tcnt = 0;
    u32 reload = SysTick->LOAD;
    ticks = nus*SYS_CLK;
    delay_osschedlock();
    told = SysTick->VAL;
    while (1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told ) tcnt+= told - tnow;
            else tcnt += reload - tnow +told;
            told = tnow;
            if (tcnt >= ticks)break;

            
        }

    };
    delay_osschedunlock();
    

}
void delay_ms(u16 nms)
{
    if( delay_osrunning && (__get_IPSR() == 0) )
    {
        if(nms>=fac_ms)						
		{ 
   			delay_ostimedly(nms/fac_ms);	
		}
		nms%=fac_ms;
    			
    }
    delay_us((u32)(nms*1000));

}

#else

void delay_ms(u16 nms)
{
   

}
void delay_us(u32 nus)
{

}

#endif
