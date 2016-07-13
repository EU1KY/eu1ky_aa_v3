#ifndef _TEXTBOX_H_
#define _TEXTBOX_H_

#include <stdint.h>
#include "font.h"

typedef enum
{
    TEXTBOX_TYPE_TEXT = 0, //Visible item with text
    TEXTBOX_TYPE_HITRECT,  //Invisible hit rectangle
} TEXTBOX_TYPE_t;

#pragma pack(push,1)
typedef struct
{
    uint8_t type;      //TEXTBOX_TYPE_t
    uint8_t nowait;    //Set to 1 to bypass waiting for touch release and to return 0 from hit test func
    uint16_t x0;       //Origin x
    uint16_t y0;       //Origin y
    const char* text;  //Text of the box for TEXTBOX_TYPE_TEXT type
    FONTS font;        //Font of the box for TEXTBOX_TYPE_TEXT type
    uint32_t fgcolor;  //Foreground color for TEXTBOX_TYPE_TEXT type
    uint32_t bgcolor;  //Background color for TEXTBOX_TYPE_TEXT type
    void (*cb)(void);  //Callback function to be called when textbox is tapped
    uint16_t width;    //Filled automatically for TEXTBOX_TYPE_TEXT type
    uint16_t height;   //Filled automatically for TEXTBOX_TYPE_TEXT type
    void *next;        //Filled automatically in TEXTBOX_Append
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
