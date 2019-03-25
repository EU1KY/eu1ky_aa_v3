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
#include <math.h>
#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "num_keypad.h"
#include "textbox.h"
#include "mainwnd.h"

extern void Sleep(uint32_t);

static uint32_t kbdRqExit = 0;
static char txtbuf[12];
static int8_t CurPos;
static int8_t max_len;
static uint32_t digit;
static uint32_t value;
static uint32_t save;
static uint32_t rest;
static uint32_t minvalue;
static uint32_t maxvalue;
static uint32_t result;
static uint32_t edited = 0;

#define KEYW 60
#define KEYH 40
#define KBDX0 20
//#define KBDY0 50
#define KBDY0 90
#define KBDX(col) (KBDX0 + col * KEYW + 8 * col)
#define KBDY(row) (KBDY0 + row * KEYH + 6 * row)


static void Show_value(uint8_t col) {
uint8_t x;
uint32_t color = LCD_GREEN;
    if(col==0)
        color = LCD_GREEN;
    else color = LCD_RED;
    if(max_len>4)
        sprintf(txtbuf, "%08u", value);// date
    else sprintf(txtbuf, "%04u", value);// time
    if (value < minvalue || value > maxvalue)
        color = LCD_RED;
    LCD_FillRect((LCDPoint){0,0},(LCDPoint){240,38}, LCD_BLACK);
    FONT_Write(FONT_FRANBIG, color, LCD_BLACK, 10, 2, txtbuf);

    x=10+(max_len-CurPos)*17;
    if(x>17) LCD_FillRect((LCDPoint){x-17,34},(LCDPoint){x+35,38}, LCD_BLACK);//delete the old cursor
    else LCD_FillRect((LCDPoint){x,34},(LCDPoint){x+35,38}, LCD_BLACK);//delete the old cursor
    LCD_FillRect((LCDPoint){x,34},(LCDPoint){x+16,38}, LCD_WHITE);// Set new cursor
}

static uint8_t testValue(void){
int dd,mm,yyyy,hh,mi;
char txt[5];

    if(value==0) return 100; // no date
    if ((value<minvalue)||(value>maxvalue)) return 7;
    if(max_len>4){//test , if guilty date
        sprintf(txtbuf, "%08u", value);// date
        dd=atoi(&txtbuf[6]);
        if((dd==0)||(dd>31)) return 1;// day false
        strncpy(txt,&txtbuf[4],2);// test month 0..12
        txt[2]=0;
        mm=atoi(txt);
        if((mm==0)||(mm>12))  return 2;// month false
        strncpy(txt,&txtbuf[4],4);// test month 0..12
        txt[4]=0;
        yyyy=atoi(txt);
        if(mm==2){
            if((yyyy%4==0)&&(yyyy%100!=0)){
                if(dd>29) return 3;
            }
            else if(dd>28) return 4;
        }
        switch(mm){
            case 4:
            case 6:
            case 9:
            case 11:
                if(dd>30) return 5;
            default:
                if(dd>31) return 6;
        }
    }
    else{// test of time
        sprintf(txtbuf, "%04u", value);
        strncpy(txt,&txtbuf[0],2);//hour
        txt[2]=0;
        hh=atoi(txt);
        if(hh>23) return 7;// hour
        mi=atoi(&txtbuf[2]);
        if(mi>59) return 8;// minute
    }
    return 0;
}

static void KeybHitCb(const TEXTBOX_t* tb) {
uint32_t k;
uint8_t i;

    save=value;
    digit = tb->text[0] - '0';
    k=(uint32_t)pow(10.0,CurPos);
    rest = value%(k);
    value = ((value/(10*k))*10 +digit)*k+rest;
    if(CurPos>0) CurPos--;
    if(testValue()!=0) Show_value(1);// red colour, if out of range
    else  Show_value(0);
}

static void KeybHitLeftCb(void)
{
    if(CurPos<max_len) CurPos+=1;
    Show_value(0);
    edited = 1;
}

static void KeybHitRightCb(void)
{
    if(CurPos>0 ) CurPos-=1;
    Show_value(0);
    edited = 1;
}

static void KeybHitOKCb(void)
{
uint8_t k;
long val;
char* ptr;

    val=strtoul(&txtbuf[0],&ptr,10);
    result =(uint32_t) val;
    value=result;
    if(testValue()==0){
        kbdRqExit = 1;// return
        Show_value(0);//green
    }
    else Show_value(1);//red, no return
}

static void KeybHitCancelCb(void)
{
    result = 0;
    kbdRqExit = 1;
    if(max_len>4) NoDate=1;// cancel suppresses date/ time
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
    (TEXTBOX_t){ .x0 = KBDX(0), .y0 = KBDY(3), .text = "<", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(200,0,0), .cb = KeybHitLeftCb, .next = (void*)&tb_keybd[10] },
    (TEXTBOX_t){ .x0 = KBDX(1), .y0 = KBDY(3), .text = "0", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLUE, .cb = (void(*)(void))KeybHitCb, .cbparam = 1, .next = (void*)&tb_keybd[11] },
    (TEXTBOX_t){ .x0 = KBDX(2), .y0 = KBDY(3), .text = ">", .font = FONT_FRANBIG, .width = KEYW, .height = KEYH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RGB(200,0,0), .cb = KeybHitRightCb, .next = (void*)&tb_keybd[12] },
    (TEXTBOX_t){ .x0 = KBDX(3)+40, .y0 = KBDY(3) + 4, .text = "Cancel", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_BLUE, .bgcolor = LCD_YELLOW, .cb = KeybHitCancelCb, .next = (void*)&tb_keybd[13] },
    (TEXTBOX_t){ .x0 = KBDX(3) + 150, .y0 = KBDY(3) + 4, .text = "OK", .font = FONT_FRANBIG, .border = 1, .center = 1, .width = 90, .height = 32,
                 .fgcolor = LCD_YELLOW, .bgcolor = LCD_RGB(0,128,0), .cb = KeybHitOKCb },

};
#define KBDNUMKEYS (sizeof(tb_keybd) / sizeof(TEXTBOX_t))

uint32_t NumKeypad(uint32_t initial, uint32_t min_value, uint32_t max_value, const char* header_text)
{
    //LCD_FillAll(LCD_BLACK);

    kbdRqExit = 0;
    minvalue = min_value;
    maxvalue = max_value;
    //if(initial<1111) initial=1111;
    value = initial;
    edited = 0;

    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, KBDX0+240, 0, "                            ");
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, KBDX0+240, 0, header_text);

    CurPos=0;
    if(initial>2359) sprintf(txtbuf, "%08u", value);
    else sprintf(txtbuf, "%04u", value);
    max_len=strlen(txtbuf)-1;
    Show_value(0);
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
