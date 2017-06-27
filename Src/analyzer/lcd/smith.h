/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef SMITH_H_INCLUDED
#define SMITH_H_INCLUDED

#include <stdint.h>
#include <complex.h>
#include "lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

void SMITH_DrawGrid(int32_t x, int32_t y, int32_t r, LCDColor color, LCDColor bgcolor);

void SMITH_ResetStartPoint(void);

void SMITH_DrawG(float complex G, LCDColor color);

#ifdef __cplusplus
}
#endif

#endif //SMITH__INCLUDED
