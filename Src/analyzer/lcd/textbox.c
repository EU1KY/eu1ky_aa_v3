#include "textbox.h"
#include "LCD.h"
#include "touch.h"

extern void Sleep(uint32_t);

void TEXTBOX_InitContext(TEXTBOX_CTX_t* ctx)
{
    ctx->start = 0;
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

//Returns textbox ID n context
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
        pbox->next = hbox;
        hbox->next = 0;
        idx++;
    }
    if (TEXTBOX_TYPE_TEXT == hbox->type)
    {
        if (0 == hbox->width)
            hbox->width = FONT_GetStrPixelWidth(hbox->font, hbox->text);
        if (0 == pbox->height)
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
    LCD_FillRect(LCD_MakePoint(tb->x0, tb->y0), LCD_MakePoint(tb->x0 + tb->width, tb->y0 + tb->height), tb->bgcolor);
}

void TEXTBOX_SetText(TEXTBOX_CTX_t *ctx, uint32_t idx, const char* txt)
{
    if (0 == ctx || 0 == txt)
        return;
    TEXTBOX_t *tb = TEXTBOX_Find(ctx, idx);
    if (0 == tb)
        return;
    if (TEXTBOX_TYPE_HITRECT == tb->type)
        return;
    TEXTBOX_Clear(ctx, idx);
    tb->text = txt;
    tb->width = FONT_GetStrPixelWidth(tb->font, tb->text);
    tb->height = FONT_GetHeight(tb->font);
    FONT_Write(tb->font, tb->fgcolor, tb->bgcolor, tb->x0, tb->y0, tb->text);
}

void TEXTBOX_DrawContext(TEXTBOX_CTX_t *ctx)
{
    TEXTBOX_t* pbox = ctx->start;
    while (pbox)
    {
        if (TEXTBOX_TYPE_TEXT == pbox->type)
            FONT_Write(pbox->font, pbox->fgcolor, pbox->bgcolor, pbox->x0, pbox->y0, pbox->text);
        pbox = pbox->next;
    }
}

uint32_t TEXTBOX_HitTest(TEXTBOX_CTX_t *ctx)
{
    LCDPoint coord;
    if (!TOUCH_Poll(&coord))
        return 0;
    TEXTBOX_t* pbox = ctx->start;
    while (pbox)
    {
        if (TEXTBOX_TYPE_TEXT == pbox->type)
        {
            if (0 == pbox->width)
                pbox->width = FONT_GetStrPixelWidth(pbox->font, pbox->text);
            if (0 == pbox->height)
                pbox->height = FONT_GetHeight(pbox->font);
        }
        if (coord.x >= pbox->x0 && coord.x < pbox->x0 + pbox->width)
        {
            if (coord.y >= pbox->y0 && coord.y < pbox->y0 + pbox->height)
            {
                if (pbox->cb)
                    pbox->cb();
                if (pbox->nowait)
                {
                    Sleep(pbox->nowait);
                    return 0;
                }
                if (TEXTBOX_TYPE_TEXT == pbox->type)
                    FONT_Write(pbox->font, pbox->bgcolor, pbox->fgcolor, pbox->x0, pbox->y0, pbox->text);
                while (TOUCH_IsPressed());
                if (TEXTBOX_TYPE_TEXT == pbox->type)
                    FONT_Write(pbox->font, pbox->fgcolor, pbox->bgcolor, pbox->x0, pbox->y0, pbox->text);
                return 1;
            }
        }
        pbox = pbox->next;
    }
    return 0;
}
