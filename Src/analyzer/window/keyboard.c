/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

//Alphanumeric keyboard window implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "keyboard.h"
#include "textbox.h"

extern void Sleep(uint32_t);

static uint32_t kbdRqExit = 0;
static char txtbuf[33];
static char *presult;
static uint32_t maxlen;
static uint32_t isChanged;

#define KEYW 40
#define KEYH 40
#define KBDX0 15
#define KBDY0 50
#define KBDX(col) (KBDX0 + col * KEYW + 6 * col)
#define KBDY(row) (KBDY0 + row * KEYH + 6 * row)

static void KeybHitCb(const TEXTBOX_t* tb)
{
    uint32_t len = strlen(txtbuf);
    if (len >= maxlen)
        return;
    txtbuf[len] = tb->text[0];
    txtbuf[len+1] = '\0';
    uint32_t color = LCD_GREEN;
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 17);
    FONT_Write(FONT_FRANBIG, color, LCD_BLACK, KBDX0, 17, txtbuf);
}

static void KeybHitBackspaceCb(void)
{
    uint32_t len = strlen(txtbuf);
    if (len == 0)
        return;
    txtbuf[len-1] = '\0';
    uint32_t color = LCD_GREEN;
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 17);
    FONT_Write(FONT_FRANBIG, color, LCD_BLACK, KBDX0, 17, txtbuf);
}

static void KeybHitOKCb(void)
{
    if (strlen(txtbuf) != 0)
    {
        strcpy(presult, txtbuf);
        kbdRqExit = 1;
        isChanged = 1;
    }
}

static void KeybHitCancelCb(void)
{
    kbdRqExit = 1;
}

//This array is placed in flash memory, so any changes in its contents are prohibited. All the fields
//should be properly filled during the compile time
static const TEXTBOX_t tb_keybal[] = {
    (TEXTBOX_t){ .x0 = KBDX(0), .y0 = KBDY(0), .text = "1", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[1] },
    (TEXTBOX_t){ .x0 = KBDX(1), .y0 = KBDY(0), .text = "2", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[2] },
    (TEXTBOX_t){ .x0 = KBDX(2), .y0 = KBDY(0), .text = "3", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[3] },
    (TEXTBOX_t){ .x0 = KBDX(3), .y0 = KBDY(0), .text = "4", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[4] },
    (TEXTBOX_t){ .x0 = KBDX(4), .y0 = KBDY(0), .text = "5", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[5] },
    (TEXTBOX_t){ .x0 = KBDX(5), .y0 = KBDY(0), .text = "6", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[6] },
    (TEXTBOX_t){ .x0 = KBDX(6), .y0 = KBDY(0), .text = "7", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[7] },
    (TEXTBOX_t){ .x0 = KBDX(7), .y0 = KBDY(0), .text = "8", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[8] },
    (TEXTBOX_t){ .x0 = KBDX(8), .y0 = KBDY(0), .text = "9", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[9] },
    (TEXTBOX_t){ .x0 = KBDX(9), .y0 = KBDY(0), .text = "0", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[10] },

    (TEXTBOX_t){ .x0 = KBDX(0), .y0 = KBDY(1), .text = "Q", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[11] },
    (TEXTBOX_t){ .x0 = KBDX(1), .y0 = KBDY(1), .text = "W", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[12] },
    (TEXTBOX_t){ .x0 = KBDX(2), .y0 = KBDY(1), .text = "E", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[13] },
    (TEXTBOX_t){ .x0 = KBDX(3), .y0 = KBDY(1), .text = "R", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[14] },
    (TEXTBOX_t){ .x0 = KBDX(4), .y0 = KBDY(1), .text = "T", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[15] },
    (TEXTBOX_t){ .x0 = KBDX(5), .y0 = KBDY(1), .text = "Y", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[16] },
    (TEXTBOX_t){ .x0 = KBDX(6), .y0 = KBDY(1), .text = "U", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[17] },
    (TEXTBOX_t){ .x0 = KBDX(7), .y0 = KBDY(1), .text = "I", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[18] },
    (TEXTBOX_t){ .x0 = KBDX(8), .y0 = KBDY(1), .text = "O", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[19] },
    (TEXTBOX_t){ .x0 = KBDX(9), .y0 = KBDY(1), .text = "P", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[20] },

    (TEXTBOX_t){ .x0 = KBDX(0), .y0 = KBDY(2), .text = "A", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[21] },
    (TEXTBOX_t){ .x0 = KBDX(1), .y0 = KBDY(2), .text = "S", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[22] },
    (TEXTBOX_t){ .x0 = KBDX(2), .y0 = KBDY(2), .text = "D", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[23] },
    (TEXTBOX_t){ .x0 = KBDX(3), .y0 = KBDY(2), .text = "F", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[24] },
    (TEXTBOX_t){ .x0 = KBDX(4), .y0 = KBDY(2), .text = "G", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[25] },
    (TEXTBOX_t){ .x0 = KBDX(5), .y0 = KBDY(2), .text = "H", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[26] },
    (TEXTBOX_t){ .x0 = KBDX(6), .y0 = KBDY(2), .text = "J", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[27] },
    (TEXTBOX_t){ .x0 = KBDX(7), .y0 = KBDY(2), .text = "K", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[28] },
    (TEXTBOX_t){ .x0 = KBDX(8), .y0 = KBDY(2), .text = "L", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[29] },
    (TEXTBOX_t){ .x0 = KBDX(9), .y0 = KBDY(2), .text = "_", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[30] },


    (TEXTBOX_t){ .x0 = KBDX(0), .y0 = KBDY(3), .text = "Z", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[31] },
    (TEXTBOX_t){ .x0 = KBDX(1), .y0 = KBDY(3), .text = "X", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[32] },
    (TEXTBOX_t){ .x0 = KBDX(2), .y0 = KBDY(3), .text = "C", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[33] },
    (TEXTBOX_t){ .x0 = KBDX(3), .y0 = KBDY(3), .text = "V", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[34] },
    (TEXTBOX_t){ .x0 = KBDX(4), .y0 = KBDY(3), .text = "B", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[35] },
    (TEXTBOX_t){ .x0 = KBDX(5), .y0 = KBDY(3), .text = "N", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[36] },
    (TEXTBOX_t){ .x0 = KBDX(6), .y0 = KBDY(3), .text = "M", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybal[37] },

    (TEXTBOX_t){ .x0 = KBDX(8), .y0 = KBDY(3), .text = "<-", .font = FONT_FRANBIG, .width = 2* KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = KeybHitBackspaceCb, .next = (void*)&tb_keybal[38] },

    (TEXTBOX_t){ .x0 = KBDX(0), .y0 = KBDY(4) + 4, .text = "OK", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_YELLOW, .bgcolor = LCD_RGB(0,128,0), .cb = KeybHitOKCb, .next = (void*)&tb_keybal[39] },
    (TEXTBOX_t){ .x0 = KBDX(7), .y0 = KBDY(4) + 4, .text = "Cancel", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_BLUE, .bgcolor = LCD_YELLOW, .cb = KeybHitCancelCb },
};
#define KBDNUMKEYS (sizeof(tb_keybal) / sizeof(TEXTBOX_t))

uint32_t KeyboardWindow(char* buffer, uint32_t max_len, const char* header_text)
{
    LCD_Push(); //Store current LCD bitmap
    LCD_FillAll(LCD_BLACK);

    kbdRqExit = 0;
    isChanged = 0;
    if (max_len > (sizeof(txtbuf) - 1))
        max_len = sizeof(txtbuf) - 1;
    maxlen = max_len;
    memcpy(txtbuf, buffer, max_len);
    presult = buffer;

    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLACK, KBDX0, 0, header_text);
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, KBDX0, 17, txtbuf);

    TEXTBOX_CTX_t keybd_ctx;
    TEXTBOX_InitContext(&keybd_ctx);

    TEXTBOX_Append(&keybd_ctx, (TEXTBOX_t*)tb_keybal); //Append the very first element of the list in the flash.
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

    while(TOUCH_IsPressed())
        Sleep(0);
    LCD_Pop(); //Restore last saved LCD bitmap
    return isChanged;
}
