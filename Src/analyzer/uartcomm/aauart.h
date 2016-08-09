/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef AAUART_H_INCLUDED
#define AAUART_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void AAUART_Init(void);
int AAUART_Putchar(int ch);
int AAUART_Getchar(void);
int AAUART_PutString(const char* str);
int AAUART_PutBytes(const unsigned char* bytes, int len);
uint32_t AAUART_GetRxOvfCount(void);
uint32_t AAUART_GetTxOvfCount(void);
int AAUART_IsTxBusy(void);
void AAUART_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif //AAUART_H_INCLUDED
