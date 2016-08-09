/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef FIFO_H_INCLUDED
#define FIFO_H_INCLUDED

#include <stdint.h>

#define FIFO_SIZE 256 //HC-06 minimum to avoid overflows

typedef struct
{
    uint8_t in;
    uint8_t out;
    uint8_t count;
    uint8_t buff[FIFO_SIZE];
} FIFO_Descr;

typedef enum
{
    FIFO_OK,
    FIFO_ERROR
} FIFO_STATUS;

#ifdef __cplusplus
extern "C" {
#endif

void FIFO_Init(FIFO_Descr *pFifo);
FIFO_STATUS FIFO_Put(FIFO_Descr *pFifo, uint8_t ch);
FIFO_STATUS FIFO_Get(FIFO_Descr *pFifo, uint8_t *ch);
int FIFO_IsEmpty(FIFO_Descr *pFifo);
int FIFO_IsFull(FIFO_Descr *pFifo);

#ifdef __cplusplus
}
#endif

#endif // FIFO_H_INCLUDED
