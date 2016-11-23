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

static FIFO_Descr rxfifo, txfifo;
static volatile int32_t AAUART_busy = 0;
static volatile uint32_t rx_overflow_ctr = 0;
static volatile uint32_t tx_overflow_ctr = 0;
static UART_HandleTypeDef UartHandle = {0};
static int IRQn;

void AAUART_Init(void)
{
    uint32_t comport = CFG_GetParam(CFG_PARAM_COM_PORT);
    uint32_t comspeed = CFG_GetParam(CFG_PARAM_COM_SPEED);
    FIFO_Init(&rxfifo);
    FIFO_Init(&txfifo);

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
    if ((isrflags & USART_ISR_TXE) != RESET)
    {
        UartHandle.Instance->ICR  = USART_ISR_TXE;
        if (FIFO_OK != FIFO_Get(&txfifo, (uint8_t*)&byte))
        {
            //All bytes have been transmitted
            //Disable TXE interrupt
            CLEAR_BIT(UartHandle.Instance->CR1, (USART_CR1_TXEIE | USART_CR1_TCIE));
            byte = UartHandle.Instance->CR1;
            AAUART_busy = 0;
        }
        else
        {
            UartHandle.Instance->TDR = byte;  //Writing TDR clears interrupt flag
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

//======================================================
int AAUART_Putchar (int ch)
{
    FIFO_STATUS res;

    __disable_irq(); //Useless - it works in privileged mode only
    res = FIFO_Put(&txfifo, (uint8_t)ch);
    __enable_irq();
    if (FIFO_OK != res)
    {
        while(AAUART_busy && FIFO_IsFull(&txfifo)); //Wait until space in the buffer appears
        //Warning: it is a potential place to hang, but this
        //should not happen if USART hardware works OK.
        __disable_irq();
        res = FIFO_Put(&txfifo, (uint8_t)ch);
        __enable_irq();
        if (res)
        {
            tx_overflow_ctr++;
            return res;
        }
    }

    //start transmission if there's something in the buffer and transmission is off
    if (!AAUART_busy && !FIFO_IsEmpty(&txfifo))
    {
        uint8_t ch;
        __disable_irq();
        FIFO_Get(&txfifo, &ch);
        __enable_irq();
        AAUART_busy = 1;
        UartHandle.Instance->TDR = ch; //Start transmission. Any pending TXE bit will be reset by this write to TDR.
        MODIFY_REG(UartHandle.Instance->CR1, 0, USART_CR1_TXEIE); //Enable TXE interrupt
    }
    return 0;
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
int AAUART_PutString(const char* str)
{
    int res = AAUART_PutBytes((const unsigned char*)str, strlen(str));
    while(AAUART_busy);//|| !FIFO_IsEmpty(&txfifo)); //Temporary solution. TODO: fix
    return res;
}

//======================================================
int AAUART_PutBytes(const unsigned char* bytes, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        AAUART_Putchar(bytes[i]);
    }
    return len;
}

//======================================================
uint32_t AAUART_GetRxOvfCount(void)
{
    return rx_overflow_ctr;
}

//======================================================
uint32_t AAUART_GetTxOvfCount(void)
{
    return tx_overflow_ctr;
}

//============================================
int AAUART_IsTxBusy(void)
{
    return AAUART_busy;
}
