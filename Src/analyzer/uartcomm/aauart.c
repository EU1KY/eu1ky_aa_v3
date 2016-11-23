/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "main.h"
#include "stm32f7xx_hal_uart.h"
#include "config.h"
#include "aauart.h"
#include "fifo.h"

static FIFO_Descr rxfifo;
static volatile int32_t AAUART_busy = 0;
static volatile uint32_t rx_overflow_ctr = 0;
static UART_HandleTypeDef UartHandle = {0};

static const uint8_t* volatile txptr = 0;
static volatile uint32_t txctr = 0;

void AAUART_Init(void)
{
    uint32_t comport = CFG_GetParam(CFG_PARAM_COM_PORT);
    uint32_t comspeed = CFG_GetParam(CFG_PARAM_COM_SPEED);
    int IRQn;

    FIFO_Init(&rxfifo);

    if (COM1 == comport)
        IRQn = DISCOVERY_COM1_IRQn;
    else if (COM2 == comport)
        IRQn = DISCOVERY_COM2_IRQn;
    else
        return;

    UartHandle.Init.BaudRate = comspeed;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits = UART_STOPBITS_1;
    UartHandle.Init.Parity = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode = UART_MODE_TX_RX;

    BSP_COM_Init(comport, &UartHandle);
    // NVIC for USART
    HAL_NVIC_SetPriority(IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(IRQn);
    MODIFY_REG(UartHandle.Instance->CR1, 0, USART_CR1_RXNEIE); //Enable RXNE interrupts
}

//======================================================
void AAUART_IRQHandler(void)
{
    uint32_t isrflags   = READ_REG(UartHandle.Instance->ISR);
    volatile uint8_t byte;
    UartHandle.Instance->ICR  = 0x3ffff; //Clear all flags
    if ((isrflags & USART_ISR_TC) != RESET)
    {
        UartHandle.Instance->ICR  = USART_ISR_TC;
        if (txctr)
        {
            UartHandle.Instance->TDR = *txptr++;  //Writing TDR clears interrupt flag
            txctr--;
        }
        else
        {
            //All bytes have been transmitted
            //Disable TXE interrupt
            MODIFY_REG(UartHandle.Instance->CR1, 0, USART_CR1_TCIE);
            AAUART_busy = 0;
        }
    }

    if ((isrflags & USART_ISR_RXNE) != RESET)
    {
        //Read one byte from the receive data register
        byte = (uint8_t)(UartHandle.Instance->RDR); //Reading RDR clears interrupt flag
        if (FIFO_OK != FIFO_Put(&rxfifo, byte))
        {
            rx_overflow_ctr++;
        }
    }
    __DSB();
}

uint32_t AAUART_IsBusy(void)
{
    return AAUART_busy;
}
//======================================================
int AAUART_Getchar(void)
{
    uint8_t ch;
    __disable_irq();
    FIFO_STATUS res = FIFO_Get(&rxfifo, &ch);
    __enable_irq();
    if (FIFO_OK != res)
        return EOF;
    return (int)ch;
}

//======================================================
void AAUART_PutString(const char* str)
{
    AAUART_PutBytes((const uint8_t*)str, strlen(str));
}

//======================================================
void AAUART_PutBytes(const uint8_t* bytes, uint32_t len)
{
    if (0 == len || 0 == bytes)
        return;
    while (AAUART_busy); //Block until previous transmission is over
    txctr = len - 1;
    txptr = &bytes[1];
    AAUART_busy = 1;
    UartHandle.Instance->TDR = bytes[0];
    SET_BIT(UartHandle.Instance->CR1, USART_CR1_TCIE);
}

//======================================================
uint32_t AAUART_GetRxOvfCount(void)
{
    return rx_overflow_ctr;
}
