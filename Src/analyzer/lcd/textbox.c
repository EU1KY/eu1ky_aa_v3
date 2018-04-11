#include "textbox.h"
#include "LCD.h"
#include "touch.h"

//TODO: make it more portable
#define IS_IN_RAM(ptr) ((uint32_t)ptr >= 0x20000000)

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
    if (0 == tb || !IS_IN_RAM(tb))
        return;
    if (TEXTBOX_TYPE_TEXT != tb->type)
        return;
    TEXTBOX_Clear(ctx, idx);
    tb->text = txt;
    tb->width = FONT_GetStrPixelWidth(tb->font, tb->text);
    tb->height = FONT_GetHeight(tb->font);
    FONT_Write(tb->font, tb->fgcolor, tb->bgcolor, tb->x0, tb->y0, tb->text);
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

            int w = pbox->width;
            int h = pbox->height;

            // Calculate width and height if not given
            if (0 == w)
            {
                w = FONT_GetStrPixelWidth(pbox->font, pbox->text);
                h = FONT_GetHeight(pbox->font);

                if (IS_IN_RAM(pbox))
                {
                    pbox->width = w;
                    pbox->height = h;
                }
            }

            // Calculate our corners
            int x0 = pbox->x0;
            int y0 = pbox->y0;
            int x1 = x0 + w;
            int y1 = y0 + h;

            // Draw the box in the appropriate border style
            if (pbox->border == TEXTBOX_BORDER_NONE ||
                pbox->border == TEXTBOX_BORDER_SOLID)
            {
                LCD_FillRect(LCD_MakePoint(x0, y0), LCD_MakePoint(x1, y1), pbox->bgcolor);

                if (pbox->border == TEXTBOX_BORDER_SOLID)
                {
                    LCD_Rectangle(LCD_MakePoint(x0, y0), LCD_MakePoint(x1, y1),
                                  pbox->fgcolor);
                }
            }
            else if (pbox->border == TEXTBOX_BORDER_BUTTON)
            {
                LCDPoint outline[9];
                int r = h <= 20 ? 2 : 3; // Rounded corner radius

                outline[0] = LCD_MakePoint(x0 + r, y1);
                outline[1] = LCD_MakePoint(x0, y1 - r);
                outline[2] = LCD_MakePoint(x0, y0 + r);
                outline[3] = LCD_MakePoint(x0 + r, y0);
                outline[4] = LCD_MakePoint(x1 - r, y0);
                outline[5] = LCD_MakePoint(x1, y0 + r);
                outline[6] = LCD_MakePoint(x1, y1 - r);
                outline[7] = LCD_MakePoint(x1 - r, y1);
                outline[8] = LCD_MakePoint(x0 + r, y1);

                LCD_FillPolygon(outline, 9, pbox->bgcolor);
                LCD_PolyLine(outline, 6, LCD_TintColor(pbox->bgcolor, 1.5));
                LCD_PolyLine(outline + 5, 4, LCD_TintColor(pbox->bgcolor, 0.6));
            }

            // Draw the text
            if (pbox->center && pbox->width && pbox->height)
            {
                h = FONT_GetHeight(pbox->font);
                w = FONT_GetStrPixelWidth(pbox->font, pbox->text);
                x0 += (int)pbox->width / 2 - w / 2;
                y0 += (int)pbox->height / 2 - h / 2;
            }
            FONT_Write(pbox->font, pbox->fgcolor, 0, x0, y0, pbox->text);
        }
        else if (TEXTBOX_TYPE_BMP == pbox->type)
        {
            LCD_DrawBitmap(LCD_MakePoint(pbox->x0, pbox->y0), pbox->bmp, pbox->bmpsize);
            if (pbox->border == TEXTBOX_BORDER_SOLID)
            {
                LCD_Rectangle(LCD_MakePoint(pbox->x0, pbox->y0),
                              LCD_MakePoint(pbox->x0 + pbox->width, pbox->y0 + pbox->height),
                              pbox->fgcolor);
            } //TODO else if (pbox->border == TEXTBOX_BORDER_BUTTON)
        }
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
                    Sleep(pbox->nowait);
                    return 0;
                }
                //Invert text colors while touch is pressed
                LCD_InvertRect(LCD_MakePoint(pbox->x0, pbox->y0), LCD_MakePoint(pbox->x0 + pbox->width, pbox->y0 + pbox->height));
                //Wait for touch release
                while (TOUCH_IsPressed());
                //Restore textbox colors
                LCD_InvertRect(LCD_MakePoint(pbox->x0, pbox->y0), LCD_MakePoint(pbox->x0 + pbox->width, pbox->y0 + pbox->height));
                return 1;
            }
        }
        pbox = pbox->next;
    }
    return 0;
}
