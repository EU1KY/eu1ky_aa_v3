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

//Flags to enable Smith chart arcs and circles in the flags field
#define SMITH_R50  (1<<0)
#define SMITH_R10  (1<<1)
#define SMITH_R25  (1<<2)
#define SMITH_R100 (1<<3)
#define SMITH_R200 (1<<4)
#define SMITH_R500 (1<<5)
#define SMITH_Y50  (1<<6)
#define SMITH_J10  (1<<7)
#define SMITH_J25  (1<<8)
#define SMITH_J50  (1<<9)
#define SMITH_J100 (1<<10)
#define SMITH_J200 (1<<11)
#define SMITH_J500 (1<<12)
#define SMITH_SWR2 (1<<13)
#define SMITH_LABELS (1<<14)

void SMITH_DrawGrid(int32_t x, int32_t y, int32_t r, LCDColor color, LCDColor bgcolor, uint32_t flags);

void SMITH_DrawLabels(LCDColor color, LCDColor bgcolor, uint32_t flags);

void SMITH_ResetStartPoint(void);

void SMITH_DrawG(int index, float complex G, LCDColor color);

void SMITH_DrawGEndMark(LCDColor color);

#ifdef __cplusplus
}
#endif

#endif //SMITH__INCLUDED
