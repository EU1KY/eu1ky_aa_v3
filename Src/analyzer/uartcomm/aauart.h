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
int AAUART_Getchar(void);
void AAUART_PutString(const char* str);
void AAUART_PutBytes(const uint8_t* bytes, uint32_t len);
uint32_t AAUART_GetRxOvfCount(void);
void AAUART_IRQHandler(void);
uint32_t AAUART_IsBusy(void);

#ifdef __cplusplus
}
#endif

#endif //AAUART_H_INCLUDED
