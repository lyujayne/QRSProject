#include "iic.h"
#include "delay.h"

/**
 * @brief SDA 设置 上拉输入模式
 */
void SDA_Input_Modle(iic_bus_t *bus)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = bus->IIC_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(bus->IIC_SDA_PORT, &GPIO_InitStruct);
}

/**
 * @brief SDA 设置输出开漏模式
 */
void SDA_Ouput_Modle(iic_bus_t *bus)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = bus->IIC_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(bus->IIC_SDA_PORT, &GPIO_InitStruct);
}

/**
 * @brief SDA 输出一位电平
 * @param bus IIC 总线句柄
 * @param val 1 输出高电平 0 输出低电平
 * @retval none
 */
void SDA_Output(iic_bus_t *bus, uint16_t val)
{
    if (val)
    {
        /* 输出高电平：BSRR 低16位对应位写1，置位引脚 */
        bus->IIC_SDA_PORT->BSRR |= bus->IIC_SDA_PIN;
    }
    else
    {
        /* 输出低电平：BSRR 高16位对应位写1，复位引脚 */
        bus->IIC_SDA_PORT->BSRR = (uint32_t)bus->IIC_SDA_PIN << 16u;
    }
    
}

/**
 * @brief SCL 输出一位电平
 * @param bus IIC 总线句柄
 * @param val 1 输出高电平 0 输出低电平
 * @retval none
 */
void SCL_Output(iic_bus_t *bus, uint16_t val)
{
    if (val)
    {
        /* 输出高电平：BSRR 低16位对应位写1，置位引脚 */
        bus->IIC_SCL_PORT->BSRR |= bus->IIC_SCL_PIN;
    }
    else
    {
        /* 输出低电平：BSRR 高16位对应位写1，复位引脚 */
        bus->IIC_SCL_PORT->BSRR = (uint32_t)bus->IIC_SCL_PIN << 16u;
    }
    
}

/**
 * @brief SDA 读取一位电平
 * @param  IIC 总线句柄
 * @retval 返回 1 表示 SDA 为高电平，返回 0 表示 SDA 为低电平
 */
uint8_t SDA_Input(iic_bus_t *bus)
{
    if (HAL_GPIO_ReadPin(bus->IIC_SDA_PORT, bus->IIC_SDA_PIN) == GPIO_PIN_SET)
    {
        return 1;
    }
    else 
    {
        return 0 ;
    }
}

/**
 * @brief IIC 起始信号
 * @param bus IIC 句柄
 * @retval None
 */
void IICStart(iic_bus_t *bus)
{
    SDA_Output(bus, 1);  // 1. SDA 先拉高，为产生下降沿做准备
    delay_us(2);
    SCL_Output(bus,1);   // 2. SCL 拉高
    delay_us(1);
    SDA_Output(bus, 0);  // 3. 在 SCL 为高时，SDA 拉低 → 产生 START（下降沿）
    delay_us(1);
    SCL_Output(bus, 0); // 4. SCL 拉低，为后续收发数据位做准备（数据在 SCL 低时更新）
    delay_us(1);

}


/**
 * @brief  IIC 停止信号（STOP）
 * @param  bus  IIC 总线句柄
 * @retval None
 *
 * I2C 停止条件定义：SCL 保持高电平期间，SDA 产生一个上升沿（低→高）。
 * 该信号表示本次 I2C 传输结束，总线释放回空闲态（空闲时 SDA/SCL 均为高）。
 */
void IICStop(iic_bus_t *bus)
{
    SCL_Output(bus,0);      // 1. SCL 拉低
    delay_us(2);
    SDA_Output(bus,0);      // 2. SDA 拉低，为产生上升沿做准备
    delay_us(1);
    SCL_Output(bus,1);      // 3. SCL 拉高
    delay_us(1);
    SDA_Output(bus,1);      // 4. 在 SCL 为高时，SDA 拉高 → 产生 STOP（上升沿）
    delay_us(1);
}


/**
 * @brief  IIC 等待从机应答信号（ACK）
 * @param  bus  IIC 总线句柄
 * @retval SUCCESS 收到应答；ERROR 从机无应答（超时）
 *
 * 工作原理：
 *   I2C 协议中，主机每发送完一个字节后，第 9 个时钟周期由"从机"控制 SDA：
 *   - 从机拉低 SDA → 应答 ACK（正常）
 *   - 从机保持 SDA 高 → 非应答 NACK（无此设备/无法应答）
 *   因此主机需把 SDA 切换为输入模式，并释放 SCL 一个时钟，检测 SDA 是否被拉低。
 *
 * 超时保护：若从机不存在或异常，SDA 一直为高，等待 cErrTime 次后主动
 * STOP 并返回 ERROR，避免程序在这里无限挂死。
 * 
 *  返回 ERROR=1
 */
unsigned char IICWaitAck(iic_bus_t *bus)
{
    unsigned short cErrtim = 5 ;
    SDA_Input_Modle(bus);
    SCL_Output(bus,1);

    while (SDA_Input(bus))
    {
        cErrtim--;
        delay_us(1);
        if (0 == cErrtim)
        {
            SDA_Ouput_Modle(bus);
            IICStop(bus);
            return ERROR;
        }
        
    }
    /* 到这里说明 SDA 被拉低，收到 ACK */
    SDA_Ouput_Modle(bus);
    SCL_Output(bus, 0);
    delay_us(2);
    return SUCCESS;

}


/**
 * @brief  IIC 发送应答信号（ACK）—— 主机作为接收方时使用
 * @param  bus  IIC 总线句柄
 * @retval None
 *
 * 使用场景：主机读取从机数据时，每读到一个字节后需要回复 ACK，
 * 表示"继续发送下一个字节"。此时主机把 SDA 拉低一个时钟。
 */
void IICSendAck(iic_bus_t *bus)
{
    SDA_Output(bus,0);
    delay_us(1);
    SCL_Output(bus,1);
    delay_us(1);
    SCL_Output(bus,1);
    delay_us(1);

}

/**
 * @brief  IIC 发送非应答信号（NACK）—— 主机作为接收方时使用
 * @param  bus  IIC 总线句柄
 * @retval None
 *
 * 使用场景：主机读到最后一个数据字节后，回复 NACK，
 * 表示"不再需要更多数据，停止发送"。此时主机让 SDA 保持高电平。
 */
void IICSendNotAck(iic_bus_t *bus)
{
    SDA_Output(bus,1);
    delay_us(1);
    SCL_Output(bus,1);
    delay_us(1);
    SCL_Output(bus,0);
    delay_us(2);
}

/**
 * @brief  IIC 发送一个字节（8 位数据）
 * @param  bus        IIC 总线句柄
 * @param  cSendByte  待发送的字节
 * @retval None
 *
 * I2C 数据格式：高位先行（MSB first），先发 bit7，最后发 bit0。
 * 每发完一位：
 *   - SCL 为低时更新 SDA（数据只在 SCL 低电平期间允许变化）
 *   - SCL 拉高时，从机采样 SDA 电平
 * 8 位全部发完后，SDA 交由从机应答（由调用方随后调用 IICWaitAck）。
 */
void IICSendByte(iic_bus_t *bus, unsigned char cSendByte)
{
    unsigned char i = 8;
    while (i--)
    {
        SCL_Output(bus,0);
        delay_us(2);
        SDA_Output(bus,cSendByte & 0x80);
        delay_us(1);
        cSendByte += cSendByte;
        delay_us(1);
        SCL_Output(bus,1);
        delay_us(2);
    }
    SCL_Output(bus,0);
    delay_us(2);
    /* 注意：这里没有自动应答，调用方需在之后调用 IICWaitAck 等待从机应答 */
}

/**
 * @brief  IIC 接收一个字节（8 位数据）
 * @param  bus  IIC 总线句柄
 * @retval 接收到的字节
 *
 * 说明：
 *   主机作为接收方，需要把 SDA 切换为输入模式，由从机驱动 SDA 输出数据。
 *   每个 bit：SCL 拉低 → 拉高（采样）→ 把读到的电平拼进结果。
 *   同样高位先行，先读到的是最高位。
 *   读完 8 位后恢复 SDA 为输出模式，随后由调用方决定回复 ACK 或 NACK。
 */
unsigned char IICReceiveByte(iic_bus_t *bus)
{
    unsigned char i  =8;
    unsigned char cR_Byte = 0;
    SDA_Input_Modle(bus);

    while (i--)
    {
        cR_Byte += cR_Byte;
        SCL_Output(bus, 1);
        delay_us(2);
        SCL_Output(bus,1);
        delay_us(1);
        cR_Byte |= SDA_Input(bus);
    }
    SCL_Output(bus, 0);
    SDA_Ouput_Modle(bus);

    return cR_Byte;
}


/**
 * @brief  IIC 写单个寄存器（单字节写）
 * @param  bus     IIC 总线句柄
 * @param  daddr   从机设备地址（7 位地址，不含读写位）
 * @param  reg     从机内部寄存器地址
 * @param  data    要写入寄存器的数据
 * @retval 0 成功；1 失败（无应答）
 *
 * 通信流程：
 *   1. START                    起始信号
 *   2. 发送设备地址(写方向)       daddr << 1，最低位=0 表示写
 *   3. 等待 ACK                  从机应答
 *   4. 发送寄存器地址             reg
 *   5. 等待 ACK
 *   6. 发送数据                   data
 *   7. 等待 ACK
 *   8. STOP                      停止信号
 */
uint8_t IIC_Write_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t data)
{
    IICStart(bus);
    IICSendByte(bus, daddr << 1);
    if (IICWaitAck(bus))   // 返回 ERROR=1
    {
        IICStop(bus);
        return 1;
    }
    IICSendByte(bus,reg);
    IICWaitAck(bus);
    IICSendByte(bus,data);
    IICWaitAck(bus);
    IICStop(bus);
    delay_us(1);
    return 0;
}


/**
 * @brief  IIC 连续写多个字节（从某寄存器开始连续写）
 * @param  bus     IIC 总线句柄
 * @param  daddr   从机设备地址（7 位地址，不含读写位）
 * @param  reg     起始寄存器地址
 * @param  length  要写入的数据字节数
 * @param  buff    待写入数据缓冲区
 * @retval 0 成功；1 失败（无应答）
 *
 * 流程：START → 写地址 → 写起始寄存器 → 连续写 length 个字节 → STOP。
 */
uint8_t IIC_Write_Multi_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t length,uint8_t buff[])
{
    unsigned char i;
    IICStart(bus);
    IICSendByte(bus,daddr << 1);
    if (IICWaitAck(bus))
    {
        IICStop(bus);
        return 1;
    }
    IICSendByte(bus,reg);
    IICWaitAck(bus);
    for ( i = 0; i < length; i++)
    {
        IICSendByte(bus,buff[i]);
        IICWaitAck(bus);
    }
    IICStop(bus);
    delay_us(1);
    return 0;
}

/**
 * @brief  IIC 读单个寄存器（随机读）
 * @param  bus     IIC 总线句柄
 * @param  daddr   从机设备地址（7 位地址，不含读写位）
 * @param  reg     要读取的从机内部寄存器地址
 * @retval 读取到的寄存器数据
 *
 * 通信流程（关键：使用重复起始信号 Repeated Start，中间不发 STOP）：
 *   1. START                    起始信号
 *   2. 发送设备地址(写方向)       告诉从机接下来写寄存器号
 *   3. 发送寄存器地址             reg
 *   4. 重复起始信号 Repeated Start（SCL 高时 SDA 下降沿）
 *   5. 发送设备地址(读方向)       (daddr<<1)+1，最低位=1 表示读
 *   6. 读取 1 字节数据
 *   7. 发送 NACK                 最后一个字节回复非应答
 *   8. STOP                      停止信号
 */
unsigned char IIC_Read_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg)
{
    unsigned char dat;
    IICStart(bus);                            // 1. 起始信号
    IICSendByte(bus,daddr<<1);                // 2. 发送设备地址(写方向)
    IICWaitAck(bus);                          // 等待应答
    IICSendByte(bus,reg);                     // 3. 发送寄存器地址
    IICWaitAck(bus);                          // 等待应答
    IICStart(bus);                            // 4. 重复起始信号（Repeated Start），不释放总线
    IICSendByte(bus,(daddr<<1)+1);            // 5. 发送设备地址(读方向)
    IICWaitAck(bus);                          // 等待应答
    dat = IICReceiveByte(bus);                // 6. 接收 1 字节
    IICSendNotAck(bus);                       // 7. 发送 NACK（只读一字节，不再续读）
    IICStop(bus);                             // 8. 停止信号
    return dat;                               // 返回读取的数据
}

/**
 * @brief  IIC 连续读多个字节（从某寄存器开始连续读）
 * @param  bus     IIC 总线句柄
 * @param  daddr   从机设备地址（7 位地址，不含读写位）
 * @param  reg     起始寄存器地址
 * @param  length  要读取的字节数
 * @param  buff    用于存放读取数据的缓冲区
 * @retval 0 成功；1 失败（无应答）
 *
 * 应答规则：
 *   - 读取"前 length-1 个字节"时回复 ACK，通知从机继续输出数据
 *   - 读取"最后一个字节"时回复 NACK，通知从机停止发送
 */
uint8_t IIC_Read_Multi_Byte(iic_bus_t *bus, uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[])
{
     unsigned char i;
  IICStart(bus);                            // 起始信号
  IICSendByte(bus,daddr<<1);                // 发送设备地址(写方向)
  if(IICWaitAck(bus))                       // 等待应答，无应答则失败
  {
    IICStop(bus);
    return 1;
  }
  IICSendByte(bus,reg);                     // 发送起始寄存器地址
  IICWaitAck(bus);                          // 等待应答
  IICStart(bus);                            // 重复起始信号（Repeated Start）
  IICSendByte(bus,(daddr<<1)+1);            // 发送设备地址(读方向)
  IICWaitAck(bus);                          // 等待应答
  for(i=0;i<length;i++)                     // 连续读取 length 个字节
  {
    buff[i] = IICReceiveByte(bus);          // 接收一个字节存入缓冲区
    if(i<length-1)
    {
      IICSendAck(bus);                      // 不是最后一个字节 → 回复 ACK，继续读
    }
  }
  IICSendNotAck(bus);                       // 最后一个字节 → 回复 NACK，停止发送
  IICStop(bus);                             // 停止信号
  return 0;                                 // 成功

}




/**
 * @brief  IIC 初始化
 * @param  bus  IIC 总线句柄
 * @retval None
 *
 * 说明：上电时将 SDA / SCL 配置为推挽输出（总线空闲态为高电平），
 * 使能 GPIO 时钟等。真正通信时，驱动内部会把 SDA 自动切换为开漏输出模式。
 *
 * 注意：部分工程会通过 bus->CLK_ENABLE() 开启 GPIO 时钟，
 * 具体取决于引脚是否已在别处使能时钟，这里已注释掉。
 */
void IICInit(iic_bus_t *bus)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0}; 

    //bus->CLK_ENABLE();                    // 开启对应 GPIO 时钟（可选）

    /* 配置 SDA 引脚：推挽输出 + 上拉 + 高速 */
    GPIO_InitStruct.Pin   = bus->IIC_SDA_PIN ;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(bus->IIC_SDA_PORT, &GPIO_InitStruct);

    /* 配置 SCL 引脚：同样推挽输出 + 上拉 + 高速 */
    GPIO_InitStruct.Pin   = bus->IIC_SCL_PIN ;
    HAL_GPIO_Init(bus->IIC_SCL_PORT, &GPIO_InitStruct);

}

