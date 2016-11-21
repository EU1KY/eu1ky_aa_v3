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
#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "num_keypad.h"
#include "textbox.h"
#include "config.h"
#include "panfreq.h"

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

static uint32_t rqExit = 0;

#define NUMKEYH 36
#define NUMKEYW 36
#define NUMKEYX0 20
#define NUMKEY0 50
#define NUMKEYX(col) (NUMKEYX0 + col * NUMKEYW + 2 * col)
#define NUMKEYY(row) (NUMKEY0 + row * NUMKEYH + 2 * row)

#define BANDKEYH 36
#define BANDKEYW 50
#define BANDKEYX0 160
#define BANDKEY0 160
#define BANDKEYX(col) (BANDKEYX0 + col * BANDKEYW + 2 * col)
#define BANDKEYY(row) (BANDKEY0 + row * BANDKEYH + 2 * row)

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
    (TEXTBOX_t){ .x0 = NUMKEYX(2), .y0 = NUMKEYY(3), .text = "C", .font = FONT_FRANBIG, .width = NUMKEYW, .height = NUMKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = ClearHitCb, .cbparam = 0, .next = (void*)&tb_pan[11] },

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
    (TEXTBOX_t){ .x0 = BANDKEYX(3), .y0 = BANDKEYY(2), .text = "1.3m", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_RGB(0, 0, 128), .bgcolor = LCD_RGB(0, 0, 128), .cb = 0, .cbparam = 1, .next = (void*)&tb_pan[25] }, //For future use
    (TEXTBOX_t){ .x0 = BANDKEYX(4), .y0 = BANDKEYY(2), .text = "70cm", .font = FONT_FRANBIG, .width = BANDKEYW, .height = BANDKEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_RGB(0, 0, 128), .bgcolor = LCD_RGB(0, 0, 128), .cb = 0, .cbparam = 1, .next = (void*)&tb_pan[26] }, //For future use

    (TEXTBOX_t){ .x0 = 20, .y0 =240, .text = "OK", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_YELLOW, .bgcolor = LCD_RGB(0,128,0), .cb = OKHitCb, .next = (void*)&tb_pan[27] },
    (TEXTBOX_t){ .x0 = 200, .y0 = 240, .text = "Cancel", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_BLUE, .bgcolor = LCD_YELLOW, .cb = CancelHitCb, .next = (void*)&tb_pan[28] },

    (TEXTBOX_t){ .x0 = 10, .y0 = 20, .text = "-10", .font = FONT_FRANBIG, .width = 50, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .nowait = 100, .cb = (void(*)(void))M10HitCb, .cbparam = 1, .next = (void*)&tb_pan[29] },
    (TEXTBOX_t){ .x0 = 60, .y0 = 20, .text = "-1", .font = FONT_FRANBIG, .width = 50, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .nowait = 100, .cb = (void(*)(void))M1HitCb, .cbparam = 1, .next = (void*)&tb_pan[30] },
    (TEXTBOX_t){ .x0 = 110, .y0 = 20, .text = "-0.1", .font = FONT_FRANBIG, .width = 50, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .nowait = 100, .cb = (void(*)(void))M01HitCb, .cbparam = 1, .next = (void*)&tb_pan[31] },

    (TEXTBOX_t){ .x0 = 250, .y0 = 20, .text = "+0.1", .font = FONT_FRANBIG, .width = 50, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .nowait = 100, .cb = (void(*)(void))P01HitCb, .next = (void*)&tb_pan[32] },
    (TEXTBOX_t){ .x0 = 300, .y0 = 20, .text = "+1", .font = FONT_FRANBIG, .width = 50, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .nowait = 100, .cb = (void(*)(void))P1HitCb, .next = (void*)&tb_pan[33] },
    (TEXTBOX_t){ .x0 = 350, .y0 = 20, .text = "+10", .font = FONT_FRANBIG, .width = 50, .height = 36, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(96,96,96), .nowait = 100, .cb = (void(*)(void))P10HitCb},

};

static void M10HitCb(void)
{
}

static void M1HitCb(void)
{
}

static void M01HitCb(void)
{
}

static void P10HitCb(void)
{
}

static void P1HitCb(void)
{
}

static void P01HitCb(void)
{
}

static void OKHitCb(void)
{
    rqExit = 1;
}

static void CancelHitCb(void)
{
    rqExit = 1;
}

static void ClearHitCb(void)
{
}

static void DigitHitCb(const TEXTBOX_t* tb)
{
}

static void BandHitCb(const TEXTBOX_t* tb)
{
}

void PanFreqWindow(void)
{
}
