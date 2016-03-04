#ifndef _TEXTBOX_H_
#define _TEXTBOX_H_

#include <stdint.h>
#include "font.h"

typedef struct
{
    uint16_t x0;       //Origin x
    uint16_t y0;       //Origin y
    const char* text;  //Text of the box
    FONTS font;        //Font of the box
    uint32_t fgcolor;  //Foreground color
    uint32_t bgcolor;  //Background color
    void (*cb)(void);  //Callback function to be called
    //These are filled automatically:
    uint16_t width;
    uint16_t height;
    void *next;
} TEXTBOX_t;

typedef struct
{
    TEXTBOX_t *start;
} TEXTBOX_CTX_t;

void TEXTBOX_InitContext(TEXTBOX_CTX_t* ctx);
uint32_t TEXTBOX_Append(TEXTBOX_CTX_t* ctx, TEXTBOX_t* hbox);
void TEXTBOX_DrawContext(TEXTBOX_CTX_t *ctx);
void TEXTBOX_Clear(TEXTBOX_CTX_t *ctx, uint32_t idx);
void TEXTBOX_SetText(TEXTBOX_CTX_t *ctx, uint32_t idx, const char *txt);
uint32_t TEXTBOX_HitTest(TEXTBOX_CTX_t *ctx);

#endif //_TEXTBOX_H_
