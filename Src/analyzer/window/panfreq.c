/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

//Panoramic window frequency and bandspan setting window

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "num_keypad.h"
#include "textbox.h"
#include "config.h"
#include "panfreq.h"
#include "panvswr2.h"

extern void Sleep(uint32_t);
static void DigitHitCb(const TEXTBOX_t* tb);
static void ClearHitCb(void);
static void CancelHitCb(void);
static void OKHitCb(void);
static void BandHitCb(const TEXTBOX_t* tb);
static void M10HitCb(void);
static void M1HitCb(void);
static void M01HitCb(void);
static void P10HitCb(void);
static void P1HitCb(void);
static void P01HitCb(void);
static void BSPrevHitCb(void);
static void BSNextHitCb(void);

static uint32_t rqExit = 0;
static uint32_t _f1;
static BANDSPAN _bs;
static bool update_allowed = false;

#define NUMKEYH 36
#define NUMKEYW 40
#define NUMKEYX0 10
#define NUMKEY0 60
#define NUMKEYX(col) (NUMKEYX0 + col * NUMKEYW + 4 * col)
#define NUMKEYY(row) (NUMKEY0 + row * NUMKEYH + 4 * row)

#define BANDKEYH 36
#define BANDKEYW 54
#define BANDKEYX0 160
#define BANDKEY0 110
#define BANDKEYX(col) (BANDKEYX0 + col * BANDKEYW + 4 * col)
#define BANDKEYY(row) (BANDKEY0 + row * BANDKEYH + 4 * row)

static const TEXTBOX_t tb_pan[] = {
    (TEXTBOX_t){ .x0 = NUMKEYX(0), .y0 = NUMKEYY(0), .text = "1", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[1] },
    (TEXTBOX_t){ .x0 = NUMKEYX(1), .y0 = NUMKEYY(0), .text = "2", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[2] },
    (TEXTBOX_t){ .x0 = NUMKEYX(2), .y0 = NUMKEYY(0), .text = "3", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[3] },
    (TEXTBOX_t){ .x0 = NUMKEYX(0), .y0 = NUMKEYY(1), .text = "4", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[4] },
    (TEXTBOX_t){ .x0 = NUMKEYX(1), .y0 = NUMKEYY(1), .text = "5", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[5] },
    (TEXTBOX_t){ .x0 = NUMKEYX(2), .y0 = NUMKEYY(1), .text = "6", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[6] },
    (TEXTBOX_t){ .x0 = NUMKEYX(0), .y0 = NUMKEYY(2), .text = "7", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[7] },
    (TEXTBOX_t){ .x0 = NUMKEYX(1), .y0 = NUMKEYY(2), .text = "8", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[8] },
    (TEXTBOX_t){ .x0 = NUMKEYX(2), .y0 = NUMKEYY(2), .text = "9", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[9] },
    (TEXTBOX_t){ .x0 = NUMKEYX(1), .y0 = NUMKEYY(3), .text = "0", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))DigitHitCb, .cbparam = 1, .next = (void*)&tb_pan[10] },
    (TEXTBOX_t){ .x0 = NUMKEYX(2), .y0 = NUMKEYY(3), .text = "<", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(200,0,0), .cb = ClearHitCb, .cbparam = 0, .next = (void*)&tb_pan[11] },

    (TEXTBOX_t){ .x0 = BANDKEYX(0), .y0 = BANDKEYY(0), .text = "160", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[12] },
    (TEXTBOX_t){ .x0 = BANDKEYX(1), .y0 = BANDKEYY(0), .text = "80", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[13] },
    (TEXTBOX_t){ .x0 = BANDKEYX(2), .y0 = BANDKEYY(0), .text = "60", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[14] },
    (TEXTBOX_t){ .x0 = BANDKEYX(3), .y0 = BANDKEYY(0), .text = "40", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[15] },
    (TEXTBOX_t){ .x0 = BANDKEYX(4), .y0 = BANDKEYY(0), .text = "30", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[16] },

    (TEXTBOX_t){ .x0 = BANDKEYX(0), .y0 = BANDKEYY(1), .text = "20", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[17] },
    (TEXTBOX_t){ .x0 = BANDKEYX(1), .y0 = BANDKEYY(1), .text = "17", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[18] },
    (TEXTBOX_t){ .x0 = BANDKEYX(2), .y0 = BANDKEYY(1), .text = "15", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[19] },
    (TEXTBOX_t){ .x0 = BANDKEYX(3), .y0 = BANDKEYY(1), .text = "12", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[20] },
    (TEXTBOX_t){ .x0 = BANDKEYX(4), .y0 = BANDKEYY(1), .text = "10", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[21] },

    (TEXTBOX_t){ .x0 = BANDKEYX(0), .y0 = BANDKEYY(2), .text = "6", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[22] },
    (TEXTBOX_t){ .x0 = BANDKEYX(1), .y0 = BANDKEYY(2), .text = "4", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[23] },
    (TEXTBOX_t){ .x0 = BANDKEYX(2), .y0 = BANDKEYY(2), .text = "2", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(0, 0, 128), .cb = (void(*)(void))BandHitCb, .cbparam = 1, .next = (void*)&tb_pan[24] },
    (TEXTBOX_t){ .x0 = BANDKEYX(3), .y0 = BANDKEYY(2), .text = "1.3m", .font = FONT_FRAN, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_RGB(128, 128, 128), .bgcolor = LCD_RGB(0, 0, 128), .cb = 0, .cbparam = 1, .next = (void*)&tb_pan[25] }, //For future use
    (TEXTBOX_t){ .x0 = BANDKEYX(4), .y0 = BANDKEYY(2), .text = "70cm", .font = FONT_FRAN, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_RGB(128, 128, 128), .bgcolor = LCD_RGB(0, 0, 128), .cb = 0, .cbparam = 1, .next = (void*)&tb_pan[26] }, //For future use

    (TEXTBOX_t){ .x0 = 20, .y0 =238, .text = "OK", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_YELLOW, .bgcolor = LCD_RGB(0,128,0), .cb = OKHitCb, .next = (void*)&tb_pan[27] },
    (TEXTBOX_t){ .x0 = 370, .y0 = 238, .text = "Cancel", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_BLUE, .bgcolor = LCD_YELLOW, .cb = CancelHitCb, .next = (void*)&tb_pan[28] },

    (TEXTBOX_t){ .x0 = 5, .y0 = 5, .text = "-10", .font = FONT_FRAN, .width = 46, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .cb = (void(*)(void))M10HitCb, .cbparam = 1, .next = (void*)&tb_pan[29] },
    (TEXTBOX_t){ .x0 = 55, .y0 = 5, .text = "-1", .font = FONT_FRAN, .width = 46, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .cb = (void(*)(void))M1HitCb, .cbparam = 1, .next = (void*)&tb_pan[30] },
    (TEXTBOX_t){ .x0 = 105, .y0 = 5, .text = "-0.1", .font = FONT_FRAN, .width = 46, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .cb = (void(*)(void))M01HitCb, .cbparam = 1, .next = (void*)&tb_pan[31] },

    (TEXTBOX_t){ .x0 = 320, .y0 = 5, .text = "+0.1", .font = FONT_FRAN, .width = 46, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .cb = (void(*)(void))P01HitCb, .next = (void*)&tb_pan[32] },
    (TEXTBOX_t){ .x0 = 370, .y0 = 5, .text = "+1", .font = FONT_FRAN, .width = 46, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .cb = (void(*)(void))P1HitCb, .next = (void*)&tb_pan[33] },
    (TEXTBOX_t){ .x0 = 420, .y0 = 5, .text = "+10", .font = FONT_FRAN, .width = 46, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .cb = (void(*)(void))P10HitCb, .next = (void*)&tb_pan[34]},

    (TEXTBOX_t){ .x0 = 160, .y0 = 60, .text = "<<", .font = FONT_FRANBIG, .width = 50, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .cb = (void(*)(void))BSPrevHitCb, .next = (void*)&tb_pan[35] },
    (TEXTBOX_t){ .x0 = 400, .y0 = 60, .text = ">>", .font = FONT_FRANBIG, .width = 50, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .cb = (void(*)(void))BSNextHitCb },
};

static bool IsValidRange(void)
{
    uint32_t fstart, fend;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F))
    {
        fstart = _f1 - BSVALUES[_bs]/2;
        fend = _f1 + BSVALUES[_bs]/2;
    }
    else
    {
        fstart = _f1;
        fend = _f1 + BSVALUES[_bs];
    }
    return (fstart < fend) && (fstart >= BAND_FMIN/1000) && (fend <= BAND_FMAX/1000);
}

static void Show_F(void)
{
    uint32_t color = IsValidRange() ? LCD_GREEN : LCD_RED;
    uint32_t dp = (_f1 % 1000) / 100;
    uint32_t mhz = _f1 / 1000;
    LCD_FillRect(LCD_MakePoint(160, 8), LCD_MakePoint(319, FONT_GetHeight(FONT_FRANBIG) + 8), LCD_BLACK);
    LCD_FillRect(LCD_MakePoint(240, 63), LCD_MakePoint(399, FONT_GetHeight(FONT_FRANBIG) + 63), LCD_BLACK);
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F))
    {
        FONT_Print(FONT_FRANBIG, color, LCD_BLACK, 160, 8, "%u.%u MHz", mhz, dp);
        FONT_Print(FONT_FRANBIG, color, LCD_BLACK, 240, 63, " +/- %s", BSSTR_HALF[_bs]);
    }
    else
    {
        FONT_Print(FONT_FRANBIG, color, LCD_BLACK, 160, 8, "%u.%u MHz", mhz, dp);
        FONT_Print(FONT_FRANBIG, color, LCD_BLACK, 240, 63, " +%s", BSSTR[_bs]);
    }
}

static void BSPrevHitCb(void)
{
    if (_bs == BS200)
    {
        _bs = BS40M;
        if (!IsValidRange())
            _bs = BS200;
    }
    else
        _bs -= 1;
    Show_F();
}

static void BSNextHitCb(void)
{
    if (_bs == BS40M)
        _bs = BS200;
    else
    {
        _bs += 1;
        if (!IsValidRange())
            _bs -= 1;
    }
    Show_F();
}

static void F0_Decr(uint32_t khz)
{
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F))
    {
        if (_f1 >= (BAND_FMIN / 1000 + khz + BSVALUES[_bs]/2))
            _f1 -= khz;
    }
    else
    {
        if (_f1 >= (BAND_FMIN / 1000 + khz))
            _f1 -= khz;
    }
    Show_F();
}

static void F0_Incr(uint32_t khz)
{
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F))
    {
        if (_f1 <= (BAND_FMAX / 1000 - khz - BSVALUES[_bs]/2))
            _f1 += khz;
    }
    else
    {
        if (_f1 <= (BAND_FMAX / 1000 - khz))
            _f1 += khz;
    }
    Show_F();
}

static void M10HitCb(void)
{
    F0_Decr(10000);
}

static void M1HitCb(void)
{
    F0_Decr(1000);
}

static void M01HitCb(void)
{
    F0_Decr(100);
}

static void P10HitCb(void)
{
    F0_Incr(10000);
}

static void P1HitCb(void)
{
    F0_Incr(1000);
}

static void P01HitCb(void)
{
    F0_Incr(100);
}

static void OKHitCb(void)
{
    rqExit = 1;
    if (IsValidRange())
        update_allowed = true;
}

static void CancelHitCb(void)
{
    rqExit = 1;
}

static void ClearHitCb(void)
{
    _f1 /= 10;
    _f1 = (_f1 / 100) * 100; //Round to 100 kHz
    Show_F();
}

static void DigitHitCb(const TEXTBOX_t* tb)
{
    if (_f1 < BAND_FMAX / 1000)
    {
        uint32_t digit = tb->text[0] - '0';
        _f1 = _f1 * 10 + digit * 100;
    }
    Show_F();
}

static void BandHitCb(const TEXTBOX_t* tb)
{
    if (0 == strcmp(tb->text, "160"))
    {
        _f1 = 1800;
        _bs = BS200;
    }
    else if (0 == strcmp(tb->text, "80"))
    {
        _f1 = 3300;
        _bs = BS800;
    }
    else if (0 == strcmp(tb->text, "60"))
    {
        _f1 = 5200;
        _bs = BS400;
    }
    else if (0 == strcmp(tb->text, "40"))
    {
        _f1 = 6900;
        _bs = BS400;
    }
    else if (0 == strcmp(tb->text, "30"))
    {
        _f1 = 9900;
        _bs = BS400;
    }
    else if (0 == strcmp(tb->text, "20"))
    {
        _f1 = 13800;
        _bs = BS800;
    }
    else if (0 == strcmp(tb->text, "17"))
    {
        _f1 = 17900;
        _bs = BS400;
    }
    else if (0 == strcmp(tb->text, "15"))
    {
        _f1 = 20800;
        _bs = BS800;
    }
    else if (0 == strcmp(tb->text, "12"))
    {
        _f1 = 24700;
        _bs = BS400;
    }
    else if (0 == strcmp(tb->text, "10"))
    {
        _f1 = 27900;
        _bs = BS2M;
    }
    else if (0 == strcmp(tb->text, "6"))
    {
        _f1 = 49500;
        _bs = BS4M;
    }
    else if (0 == strcmp(tb->text, "4"))
    {
        _f1 = 69000;
        _bs = BS2M;
    }
    else if (0 == strcmp(tb->text, "2"))
    {
        _f1 = 143000;
        _bs = BS4M;
    }
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F))
        _f1 += BSVALUES[_bs] / 2;
    Show_F();
}

bool PanFreqWindow(uint32_t *pFkhz, BANDSPAN *pBs)
{
    LCD_Push(); //Store current LCD bitmap
    LCD_FillAll(LCD_BLACK);
    rqExit = 0;
    _f1 = *pFkhz;
    _bs = *pBs;
    update_allowed = false;
    bool isUpdated = false;

    TEXTBOX_CTX_t fctx;
    TEXTBOX_InitContext(&fctx);

    TEXTBOX_Append(&fctx, (TEXTBOX_t*)tb_pan); //Append the very first element of the list in the flash.
                                                      //It is enough since all the .next fields are filled at compile time.
    TEXTBOX_DrawContext(&fctx);

    Show_F();

    for(;;)
    {
        if (TEXTBOX_HitTest(&fctx))
        {
            Sleep(50);
        }
        if (rqExit)
        {
            if (update_allowed && (_f1 != *pFkhz || _bs != *pBs))
            {
                *pFkhz = _f1;
                *pBs = _bs;
                CFG_SetParam(CFG_PARAM_PAN_F1, _f1);
                CFG_SetParam(CFG_PARAM_PAN_SPAN, _bs);
                CFG_Flush();
                isUpdated = true;
            }
            break;
        }
        Sleep(10);
    }

    while(TOUCH_IsPressed())
        Sleep(0);
    LCD_Pop(); //Restore last saved LCD bitmap
    return isUpdated;
}
