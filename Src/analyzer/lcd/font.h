/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 */

#ifndef FONT_H_
#define FONT_H_

#include <stdint.h>
#include "LCD.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    FONT_FRAN, FONT_FRANBIG, FONT_CONSBIG, FONT_SDIGITS
} FONTS;


int FONT_Write(FONTS fnt, LCDColor fg, LCDColor bg, uint16_t x, uint16_t y, const char* pStr);
int FONT_Write_N(FONTS fnt, LCDColor fg, LCDColor bg, uint16_t x, uint16_t y, const char* pStr, int nChars);
int FONT_Print(FONTS fnt, LCDColor fg, LCDColor bg, uint16_t x, uint16_t y, const char *fmt, ...);
void FONT_SetAttributes(FONTS fnt, LCDColor fg, LCDColor bg);
int FONT_Printf(uint16_t x, uint16_t y, const char *fmt, ...);
int FONT_GetStrPixelWidth(FONTS fnt, const char* pStr);
void FONT_ClearLine(FONTS fnt, LCDColor bg, uint16_t y0);

#ifdef __cplusplus
}
#endif

#endif //FONT_H_

