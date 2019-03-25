#include "textbox.h"
#include "LCD.h"
#include "touch.h"
#include "config.h"

//TODO: make it more portable
#define IS_IN_RAM(ptr) ((uint32_t)ptr >= 0x20000000)

extern void Sleep(uint32_t);

static uint8_t TEXTBOX_repeats;
static  void* TEXTBOX_previous;


void TEXTBOX_InitContext(TEXTBOX_CTX_t* ctx)
{
    ctx->start = 0;
    TEXTBOX_repeats=0;// 5 times slow, then rapid
    TEXTBOX_previous=0;
}

TEXTBOX_t* TEXTBOX_Find(TEXTBOX_CTX_t *ctx, uint32_t idx)
{
    if (0 == ctx)
        return 0;
    TEXTBOX_t* pbox = ctx->start;
    uint32_t ctr = 0;
    if (0 == pbox)
        return 0;
    else
    {
        while (0 != pbox)
        {
            if (ctr == idx)
                return pbox;
            pbox = pbox->next;
            ctr++;
        }
    }
    return 0;

}

//Returns textbox ID in context
uint32_t TEXTBOX_Append(TEXTBOX_CTX_t* ctx, TEXTBOX_t* hbox)
{
    TEXTBOX_t* pbox = ctx->start;
    uint32_t idx = 0;
    if (0 == pbox)
        ctx->start = hbox;
    else
    {
        while (0 != pbox->next)
        {
            pbox = pbox->next;
            idx++;
        }
        if (IS_IN_RAM(pbox))
            pbox->next = hbox;
        if (IS_IN_RAM(hbox))
            hbox->next = 0;
        idx++;
    }
    if (TEXTBOX_TYPE_TEXT == hbox->type)
    {
        if ( IS_IN_RAM(hbox) && 0 == hbox->width)
            hbox->width = FONT_GetStrPixelWidth(hbox->font, hbox->text);
        if ( IS_IN_RAM(hbox) && 0 == hbox->height)
            hbox->height = FONT_GetHeight(hbox->font);
    }
    return idx;
}

void TEXTBOX_Clear(TEXTBOX_CTX_t *ctx, uint32_t idx)
{
    TEXTBOX_t *tb = TEXTBOX_Find(ctx, idx);
    if (0 == tb)
        return;
    if (TEXTBOX_TYPE_HITRECT == tb->type)
        return;
    LCD_FillRect(LCD_MakePoint(tb->x0, tb->y0),
                 LCD_MakePoint(tb->x0 + tb->width, tb->y0 + tb->height),
                 tb->bgcolor);
}

void TEXTBOX_SetText(TEXTBOX_CTX_t *ctx, uint32_t idx, const char* txt)
{
    if (0 == ctx || 0 == txt)
        return;
TEXTBOX_t *tb = TEXTBOX_Find(ctx, idx);
    if (TEXTBOX_TYPE_TEXT != tb->type)
        return;
    TEXTBOX_Clear(ctx, idx);
    tb->text = txt;
    if(IS_IN_RAM(tb)){
        tb->width = FONT_GetStrPixelWidth(tb->font, tb->text);
        tb->height = FONT_GetHeight(tb->font);
        FONT_Write(tb->font, tb->fgcolor, tb->bgcolor, tb->x0, tb->y0, tb->text);
    }
    else{
        int x,y;
        if (tb->center){
            int h = FONT_GetHeight(tb->font);
            y = (int)tb->y0 + (int)tb->height / 2  - h / 2;
            int w = FONT_GetStrPixelWidth(tb->font, tb->text);
            x = (int)tb->x0 + (int)tb->width / 2  - w / 2;
        }
        else {
            x=(int)tb->x0;
            y=(int)tb->y0;
        }
        FONT_Write(tb->font, tb->fgcolor, tb->bgcolor, x, y, tb->text);
    }
    if (tb->border)
    {
        LCD_Rectangle(LCD_MakePoint(tb->x0, tb->y0),
                      LCD_MakePoint(tb->x0 + tb->width, tb->y0 + tb->height),
                      tb->fgcolor);
    }
}

void TEXTBOX_DrawContext(TEXTBOX_CTX_t *ctx)
{
    TEXTBOX_t* pbox = ctx->start;
    while (pbox)
    {
        if (TEXTBOX_TYPE_TEXT == pbox->type)
        {
            if (0 != pbox->width)
            {
                LCD_FillRect(LCD_MakePoint(pbox->x0, pbox->y0),
                             LCD_MakePoint(pbox->x0 + pbox->width, pbox->y0 + pbox->height),
                             pbox->bgcolor);
            }
            else
            {
                if (IS_IN_RAM(pbox))
                {
                    pbox->width = FONT_GetStrPixelWidth(pbox->font, pbox->text);
                    pbox->height = FONT_GetHeight(pbox->font);
                }
            }
            if (pbox->center && pbox->width && pbox->height)
            {
                int h = FONT_GetHeight(pbox->font);
                int w = FONT_GetStrPixelWidth(pbox->font, pbox->text);
                int x = (int)pbox->x0 + (int)pbox->width / 2  - w / 2;
                int y = (int)pbox->y0 + (int)pbox->height / 2  - h / 2;
                FONT_Write(pbox->font, pbox->fgcolor, pbox->bgcolor, x, y, pbox->text);
            }
            else
                FONT_Write(pbox->font, pbox->fgcolor, pbox->bgcolor, pbox->x0, pbox->y0, pbox->text);
            if (pbox->border)
            {
                LCD_Rectangle(LCD_MakePoint(pbox->x0, pbox->y0),
                              LCD_MakePoint(pbox->x0 + pbox->width, pbox->y0 + pbox->height),
                              pbox->fgcolor);
            }
        }
        else if (TEXTBOX_TYPE_BMP == pbox->type)
        {
            LCD_DrawBitmap(LCD_MakePoint(pbox->x0, pbox->y0), pbox->bmp, pbox->bmpsize);
            if (pbox->border)
            {
                LCD_Rectangle(LCD_MakePoint(pbox->x0, pbox->y0),
                              LCD_MakePoint(pbox->x0 + pbox->width, pbox->y0 + pbox->height),
                              pbox->fgcolor);
            }
        }
        pbox = pbox->next;
    }
}

uint32_t TEXTBOX_HitTest(TEXTBOX_CTX_t *ctx)
{
    LCDPoint coord;

    if (!TOUCH_Poll(&coord)){// no touch
        TEXTBOX_previous=0;
        TEXTBOX_repeats=0;
        return 0;
    }
    TEXTBOX_t* pbox = ctx->start;
    while (pbox)
    {
        if (TEXTBOX_TYPE_TEXT == pbox->type)
        {
            if (0 == pbox->width && IS_IN_RAM(pbox))
                pbox->width = FONT_GetStrPixelWidth(pbox->font, pbox->text);
            if (0 == pbox->height && IS_IN_RAM(pbox))
                pbox->height = FONT_GetHeight(pbox->font);
        }
        if (coord.x >= pbox->x0 && coord.x < pbox->x0 + pbox->width)
        {
            if (coord.y >= pbox->y0 && coord.y < pbox->y0 + pbox->height)
            {
                //Execute hit callback
                if(BeepIsOn==1){
                    UB_TIMER2_Init_FRQ(880);
                    UB_TIMER2_Start();
                    Sleep(100);
                    UB_TIMER2_Stop();
                }
                if (pbox->cb)
                {
                    if (pbox->cbparam)
                    {
                        ((void(*)(const TEXTBOX_t*))pbox->cb)(pbox);
                    }
                    else
                        pbox->cb();
                }
                if (pbox->nowait)
                {
                    if(TEXTBOX_repeats<5){      //  WK
                        TEXTBOX_previous=pbox;
                        TEXTBOX_repeats++;
                        Sleep(400);
                        return 0;
                    }
                    if(pbox==TEXTBOX_previous){
                        if(TEXTBOX_repeats>=5){
                            Sleep(pbox->nowait);
                            return 0;
                        }
                    }
                }

                else {
                    Sleep(400); // WK
                }
                if(pbox->type!=TEXTBOX_TYPE_HITRECT){
                //Invert text colors while touch is pressed
                    LCD_InvertRect(LCD_MakePoint(pbox->x0, pbox->y0), LCD_MakePoint(pbox->x0 + pbox->width, pbox->y0 + pbox->height));
                //Wait for touch release
               // while (TOUCH_IsPressed());// WK
                //Restore textbox colors
                    LCD_InvertRect(LCD_MakePoint(pbox->x0, pbox->y0), LCD_MakePoint(pbox->x0 + pbox->width, pbox->y0 + pbox->height));
                }
                return 1;
            }
        }
        pbox = pbox->next;
    }
    return 0;
}
