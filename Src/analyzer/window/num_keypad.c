/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

//Numeric keypad window implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "num_keypad.h"
#include "textbox.h"

extern void Sleep(uint32_t);

static uint32_t kbdRqExit = 0;
static char txtbuf[32];
static uint32_t minvalue;
static uint32_t maxvalue;
static uint32_t result;
static uint32_t edited = 0;

#define KEYW 60
#define KEYH 40
#define KBDX0 20
#define KBDY0 50
#define KBDX(col) (KBDX0 + col * KEYW + 8 * col)
#define KBDY(row) (KBDY0 + row * KEYH + 6 * row)

static void KeybHitCb(const TEXTBOX_t* tb)
{
    if (!edited)
    {
        txtbuf[0] = '0';
        txtbuf[1] = '\0';
        edited = 1;
    }
    uint32_t digit = (uint32_t)(tb->text[0] - '0');
    uint32_t val = strtoul(txtbuf, 0, 10);
    if (val > UINT32_MAX/10)
        return;
    val = val * 10 + digit;
    sprintf(txtbuf, "%u", val);
    uint32_t color = LCD_GREEN;
    if (val < minvalue || val > maxvalue)
        color = LCD_RED;
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 17);
    FONT_Write(FONT_FRANBIG, color, LCD_BLACK, KBDX0, 17, txtbuf);
}

static void KeybHitBackspaceCb(void)
{
    uint32_t val = strtoul(txtbuf, 0, 10);
    val /= 10;
    sprintf(txtbuf, "%u", val);
    uint32_t color = LCD_GREEN;
    if (val < minvalue || val > maxvalue)
        color = LCD_RED;
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 17);
    FONT_Write(FONT_FRANBIG, color, LCD_BLACK, KBDX0, 17, txtbuf);
    edited = 1;
}

static void KeybHitOKCb(void)
{
    uint32_t val = strtoul(txtbuf, 0, 10);
    if (val >= minvalue && val <= maxvalue)
        result = val;
    kbdRqExit = 1;
}

static void KeybHitCancelCb(void)
{
    kbdRqExit = 1;
}

//This array is placed in flash memory, so any changes in its contents are prohibited. All the fields
//should be properly filled during the compile time
static const TEXTBOX_t tb_keybd[] = {
    (TEXTBOX_t){ .x0 = KBDX(0), .y0 = KBDY(0), .text = "1", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[1] },
    (TEXTBOX_t){ .x0 = KBDX(1), .y0 = KBDY(0), .text = "2", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[2] },
    (TEXTBOX_t){ .x0 = KBDX(2), .y0 = KBDY(0), .text = "3", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[3] },
    (TEXTBOX_t){ .x0 = KBDX(0), .y0 = KBDY(1), .text = "4", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[4] },
    (TEXTBOX_t){ .x0 = KBDX(1), .y0 = KBDY(1), .text = "5", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[5] },
    (TEXTBOX_t){ .x0 = KBDX(2), .y0 = KBDY(1), .text = "6", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[6] },
    (TEXTBOX_t){ .x0 = KBDX(0), .y0 = KBDY(2), .text = "7", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[7] },
    (TEXTBOX_t){ .x0 = KBDX(1), .y0 = KBDY(2), .text = "8", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[8] },
    (TEXTBOX_t){ .x0 = KBDX(2), .y0 = KBDY(2), .text = "9", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[9] },
    (TEXTBOX_t){ .x0 = KBDX(1), .y0 = KBDY(3), .text = "0", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[10] },
    (TEXTBOX_t){ .x0 = KBDX(2), .y0 = KBDY(3), .text = "<-", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(200,0,0), .cb = KeybHitBackspaceCb, .next = (void*)&tb_keybd[11] },
    (TEXTBOX_t){ .x0 = KBDX(0)+200, .y0 = KBDY(4) + 4, .text = "Cancel", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_BLUE, .bgcolor = LCD_YELLOW, .cb = KeybHitCancelCb, .next = (void*)&tb_keybd[12] },
    (TEXTBOX_t){ .x0 = KBDX(0) + 310, .y0 = KBDY(4) + 4, .text = "OK", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_YELLOW, .bgcolor = LCD_RGB(0,128,0), .cb = KeybHitOKCb },

};
#define KBDNUMKEYS (sizeof(tb_keybd) / sizeof(TEXTBOX_t))

uint32_t NumKeypad(uint32_t initial, uint32_t min_value, uint32_t max_value, const char* header_text)
{
    LCD_FillAll(LCD_BLACK);

    kbdRqExit = 0;
    minvalue = min_value;
    maxvalue = max_value;
    result = initial;
    edited = 0;

    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLACK, KBDX0, 0, header_text);
    sprintf(txtbuf, "%u", initial);
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, KBDX0, 17, txtbuf);

    TEXTBOX_CTX_t keybd_ctx;
    TEXTBOX_InitContext(&keybd_ctx);

    TEXTBOX_Append(&keybd_ctx, (TEXTBOX_t*)tb_keybd); //Append the very first element of the list in the flash.
                                                      //It is enough since all the .next fields are filled at compile time.
    TEXTBOX_DrawContext(&keybd_ctx);

    for(;;)
    {
        if (TEXTBOX_HitTest(&keybd_ctx))
        {
            Sleep(50);
        }
        if (kbdRqExit)
            break;
        Sleep(10);
    }

    while(TOUCH_IsPressed());
        Sleep(0);

    return result;
}
