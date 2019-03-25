/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef _TOUCH_H_
#define _TOUCH_H_

#include "LCD.h"
#include "FreqCounter.h"

#ifdef __cplusplus
extern "C" {
#endif

void TOUCH_Init(void);

uint8_t TOUCH_Poll(LCDPoint* pCoord);

uint8_t TOUCH_IsPressed(void);

#ifdef __cplusplus
}
#endif
#endif //_TOUCH_H_

