#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_gpio.h"

#include "stm32746g_discovery.h"
#include "config.h"
#include "i2c.h"

#define I2C_SCL_PIN                  GPIO_PIN_9
#define I2C_SCL_GPIO_PORT            GPIOB
#define I2C_SCL_GPIO_CLK             RCC_APB2Periph_GPIOB
#define I2C_SDA_PIN                  GPIO_PIN_8
#define I2C_SDA_GPIO_PORT            GPIOB
#define I2C_SDA_GPIO_CLK             RCC_APB2Periph_GPIOB

static inline void _delayUs(uint32_t us)
{
    asm volatile("           mov   r2, #14    \n"
                 "           mov   r1, #0     \n"
                 "2:         cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   2b         \n"
                 "           mov   r1, #8     \n"
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "1:         nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   1b  \n"::"r"(us):"r1", "r2");
}

static void scl0(void)
{
    HAL_GPIO_WritePin(I2C_SCL_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
    _delayUs(2);
}

static void scl1(void)
{
    HAL_GPIO_WritePin(I2C_SCL_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    _delayUs(2);
}

static void sda0(void)
{
    HAL_GPIO_WritePin(I2C_SDA_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
    _delayUs(2);
}

static void sda1(void)
{
    HAL_GPIO_WritePin(I2C_SDA_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    _delayUs(2);
}

static int sda(void)
{
    return HAL_GPIO_ReadPin(I2C_SDA_GPIO_PORT, I2C_SDA_PIN) ? 1 : 0;
}

void i2c_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    //RCC_APB2PeriphClockCmd(I2C_SCL_GPIO_CLK | I2C_SDA_GPIO_CLK, ENABLE);
    //GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN;
    //GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    //GPIO_Init(I2C_SCL_GPIO_PORT, &GPIO_InitStructure);
    //GPIO_InitStructure.GPIO_Pin = I2C_SDA_PIN;
    //GPIO_Init(I2C_SDA_GPIO_PORT, &GPIO_InitStructure);

    sda0();
    scl1();
    sda1();
}

void i2c_start(void)
{
    sda1();
    scl1();
    sda0();
    scl0();
}

void i2c_stop(void)
{
    sda0();
    scl1();
    sda1();
}

void i2c_write(uint8_t data)
{
    uint8_t mask = 0x80;
    while(mask)
    {
        scl0();
        //Set data bit
        if (data & mask)
            sda1();
        else
            sda0();
        scl1();
        mask >>= 1;
    }
    scl0();
    sda1();
    scl1();
    //read ack here?
    scl0();
}

uint8_t i2c_read_ack(void)
{
    uint8_t mask = 0x80;
    uint8_t byte = 0;
    while(mask)
    {
        scl1();
        //Get data bit
        if (sda())
            byte |= mask;
        scl0();
        mask >>= 1;
    }
    sda0();
    scl1();
    scl0();
    sda1();
    return byte;
}

uint8_t i2c_read_nack(void)
{
    uint8_t mask = 0x80;
    uint8_t byte = 0;
    while(mask)
    {
        scl1();
        //Get data bit
        if (sda())
            byte |= mask;
        scl0();
        mask >>= 1;
    }
    sda1();
    scl1();
    scl0();
    return byte;
}

uint8_t i2c_status(void)
{
    return 0;
}
