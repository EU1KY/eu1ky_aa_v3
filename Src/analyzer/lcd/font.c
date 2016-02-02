/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include "font.h"
#include "fran.h"
#include "franbig.h"
#include "consbig.h"
#include "sdigits.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static LCDColor _fgColor = LCD_WHITE;
static LCDColor _bgColor = LCD_BLACK;
static FONTS _font = FONT_FRAN;

static void FONT_DrawByte(uint8_t byte, uint8_t nBits, LCDColor fg, uint16_t x, uint16_t y)
{
    uint8_t mask = 1;
    while (nBits--)
    {
        if (byte & mask)
            LCD_SetPixel(LCD_MakePoint(x, y), fg);
        mask <<= 1;
        x -= 1;
        if (0 == mask)
            break;
    }
}

struct _fontparams
{
    const uint8_t** pFont;
    uint8_t charHeight;
    uint8_t charSpacing;
};

static void FONT_GetParams(FONTS fnt, struct _fontparams* pRes)
{
    if (0 == pRes)
    {
        return;
    }
    switch (fnt)
    {
    default:
    case FONT_FRAN:
        pRes->pFont = (const uint8_t**)fran;
        pRes->charHeight = fran_height;
        pRes->charSpacing = fran_spacing;
        break;
    case FONT_FRANBIG:
        pRes->pFont = (const uint8_t**)franbig;
        pRes->charHeight = franbig_height;
        pRes->charSpacing = franbig_spacing;
        break;
    case FONT_CONSBIG:
        pRes->pFont = (const uint8_t**)consbig;
        pRes->charHeight = consbig_height;
        pRes->charSpacing = consbig_spacing;
        break;
    case FONT_SDIGITS:
        pRes->pFont = (const uint8_t**)sdigits;
        pRes->charHeight = sdigits_height;
        pRes->charSpacing = sdigits_spacing;
        break;
    }
}

int FONT_Write(FONTS fnt, LCDColor fg, LCDColor bg, uint16_t x, uint16_t y, const char* pStr)
{
    return FONT_Write_N(fnt, fg, bg, x, y, pStr, strlen(pStr));
}

void FONT_ClearLine(FONTS fnt, LCDColor bg, uint16_t y0)
{
    //Fill the rectangle with background color
    struct _fontparams fp = {0};
    FONT_GetParams(fnt, &fp);
    LCD_FillRect(LCD_MakePoint(0, y0), LCD_MakePoint(LCD_GetWidth(), y0 + fp.charHeight), bg);
}

int FONT_Write_N(FONTS fnt, LCDColor fg, LCDColor bg, uint16_t x, uint16_t y, const char* pStr, int nChars)
{
    int nPrinted = 0;
    struct _fontparams fp = {0};
    uint8_t ch;

    FONT_GetParams(fnt, &fp);

    //Fill the rectangle with background color
    LCD_FillRect(LCD_MakePoint(x, y), LCD_MakePoint(x + FONT_GetStrPixelWidth(fnt, pStr), y + fp.charHeight), bg);

    while (*pStr != '\0' && nChars--)
    {
        uint32_t charIndex = (uint32_t)*pStr;
        const uint8_t* pCharData = fp.pFont[charIndex];
        uint8_t charWidth = pCharData[0];
        uint8_t charWidthCurrent;
        uint16_t yCurr;
        uint8_t lines = fp.charHeight;

        if ((x + charWidth) > LCD_GetWidth())
        {
            return nPrinted;
        }
        ++pCharData; // Now points to data beginning

        yCurr = y;
        while (lines--)
        {
            charWidthCurrent  = charWidth;
            while (charWidthCurrent)
            {
                //First byte is right part of the character bitmap
                uint8_t currByteWidth;

                currByteWidth = charWidthCurrent >= 8 ? 8 : charWidthCurrent;
                ch = *pCharData++;
                if (0 != ch) //Don't draw empty byte
                    FONT_DrawByte(ch, currByteWidth, fg, x + charWidthCurrent, yCurr);
                if (charWidthCurrent >= 8)
                    charWidthCurrent -= 8;
                else
                    charWidthCurrent = 0;
            }
            yCurr++;
        }
        ++nPrinted;

        if (*++pStr == '\0')
            break;
/* optimized away: background is already drawn
        x = x + charWidth;
        //Draw character spacing
        {
            pCharData = fp.pFont[0x20];
            uint8_t byteCtr = 0;
            yCurr = y;
            while (byteCtr < fp.charHeight)
            {
                FONT_DrawByte(*pCharData++, fp.charSpacing, fg, x + fp.charSpacing, yCurr++);
                ++byteCtr;
            }
        }
        x = x + fp.charSpacing;
*/
        x = x + charWidth + fp.charSpacing;
    } //while(*pStr != '\0')
    return nPrinted;
}

void FONT_SetAttributes(FONTS fnt, LCDColor fg, LCDColor bg)
{
    _fgColor = fg;
    _bgColor = bg;
    _font = fnt;
}

static char tmpBuf[256];

int FONT_Printf(uint16_t x, uint16_t y, const char *fmt, ...)
{

    int np = 0;
    va_list ap;
    va_start (ap, fmt);
    np = vsnprintf(tmpBuf, 255, fmt, ap);
    tmpBuf[np] = '\0';
    va_end(ap);
    return FONT_Write(_font, _fgColor, _bgColor, x, y, tmpBuf);
}

int FONT_Print(FONTS fnt, LCDColor fg, LCDColor bg, uint16_t x, uint16_t y, const char *fmt, ...)
{
    int np = 0;
    va_list ap;
    va_start (ap, fmt);
    np = vsnprintf(tmpBuf, 255, fmt, ap);
    tmpBuf[np] = '\0';
    va_end(ap);
    return FONT_Write(fnt, fg, bg, x, y, tmpBuf);
}

int FONT_GetStrPixelWidth(FONTS fnt, const char* pStr)
{
    int w = 0;
    struct _fontparams fp = {0};

    FONT_GetParams(fnt, &fp);

    while (*pStr != '\0')
    {
        uint32_t charIndex = (uint32_t)*pStr;
        const uint8_t* pCharData = fp.pFont[charIndex];
        w += pCharData[0];
        if (*++pStr == '\0')
            break;
        w += fp.charSpacing;
    }
    return w;
}
