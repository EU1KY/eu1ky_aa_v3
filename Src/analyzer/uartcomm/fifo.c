/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include "fifo.h"

void FIFO_Init(FIFO_Descr *pFifo)
{
    pFifo->count = 0;
    pFifo->in = 0;
    pFifo->out = 0;
}

FIFO_STATUS FIFO_Put(FIFO_Descr *pFifo, uint8_t ch)
{
    if(pFifo->count == FIFO_SIZE)
        return FIFO_ERROR;
    pFifo->buff[pFifo->in++]=ch;
    pFifo->count++;
    if(pFifo->in == FIFO_SIZE)
        pFifo->in=0;
    return FIFO_OK;
}

FIFO_STATUS FIFO_Get(FIFO_Descr *pFifo, uint8_t *ch)
{
    if (pFifo->count == 0)
        return FIFO_ERROR;
    *ch = pFifo->buff[pFifo->out++];
    pFifo->count--;
    if (pFifo->out == FIFO_SIZE)
        pFifo->out = 0;//start from beginning
    return FIFO_OK;
}

int FIFO_IsEmpty(FIFO_Descr *pFifo)
{
    if (pFifo->count == 0)
        return 1;
    return 0;
}

int FIFO_IsFull(FIFO_Descr *pFifo)
{
    if (pFifo->count == FIFO_SIZE)
        return 1;
    return 0;
}

