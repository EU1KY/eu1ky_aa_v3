/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include "hit.h"
#include "config.h"
#include "FreqCounter.h"
#include "panvswr2.h"


void ShowHitRect(const struct HitRect* r){// WK

    while(r->x1!=0xFFFFFFFFul){
            LCD_Rectangle(LCD_MakePoint(r->x1,r->y1), LCD_MakePoint(r->x2-1,r->y2-1),LCD_RGB(150,150,150));
            LCD_Rectangle(LCD_MakePoint(r->x1+1,r->y1+1), LCD_MakePoint(r->x2-2,r->y2-2),LCD_RGB(150,150,150));
            ++r;
    }
}

void (*LastR)(void) ;
extern int BeepIsActive;
int HitTest(const struct HitRect* r, uint32_t x, uint32_t y)
{

    while (r->x1 != 0xFFFFFFFFul)
    {

        if (x >= r->x1 && x <= r->x2 && y >= r->y1 && y <= r->y2)
        {
            if (r->HitCallback){
                if(LastR!=r->HitCallback) {
                    LastR=r->HitCallback;
                    if(SWRTone==0){// SWR Sound against beep
                        if(BeepIsOn==1){
                            BeepIsActive=1;
                            UB_TIMER2_Init_FRQ(880);
                            UB_TIMER2_Start();
                            Sleep(100);
                            BeepIsActive=0;
                        }
                    }
                }
                r->HitCallback();
            }
            return 1;
        }
        ++r;
    }
    return 0;
}
