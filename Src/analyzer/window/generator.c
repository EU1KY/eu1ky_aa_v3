/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <stdio.h>
#include "LCD.h"
#include "dsp.h"
#include "font.h"
#include "touch.h"
#include "bkup.h"
#include "gen.h"

#define GEN_BAND_FMIN 100000ul
#define GEN_BAND_FMAX 148000000ul

void Sleep(uint32_t ms);

static uint32_t GenFreq = 10000000;

static void ShowF()
{
    char str[50];
    sprintf(str, "F: %u kHz", (unsigned int)(GenFreq / 1000));
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 40);
    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 160 - FONT_GetStrPixelWidth(FONT_FRANBIG, str) / 2, 40, str);
}

//Handle touch screen in generator mode
static int Touch()
{
    LCDPoint pt;
    uint32_t step = 100000;
    while(TOUCH_Poll(&pt))
    {
        if(pt.y > 120)
        {
            return 1;    //request to change window
        }

        if(pt.x < 40 || pt.x > 280)
        {
            step = 500000;
        }
        else if(pt.x < 90 || pt.x > 230)
        {
            step = 100000;
        }
        else
        {
            step = 5000;
        }
        uint32_t currF = GenFreq;
        if(currF > step && currF % step != 0)
        {
            currF -= (currF % step);
        }
        if(currF < GEN_BAND_FMIN)
        {
            currF = GEN_BAND_FMIN;
        }
        if(pt.x > 160)
        {
            if(currF <= GEN_BAND_FMAX)
            {
                if ((currF + step) > GEN_BAND_FMAX)
                    GenFreq = GEN_BAND_FMAX;
                else
                    GenFreq = currF + step;
                GEN_SetMeasurementFreq(GenFreq);
                BKUP_SaveFGen(GenFreq);
            }
            else
            {
                GEN_SetMeasurementFreq(currF);
            }
        }
        else
        {
            if(currF > GEN_BAND_FMIN)
            {
                if(currF > step && (currF - step) >= GEN_BAND_FMIN)
                {
                    GenFreq = currF - step;
                    GEN_SetMeasurementFreq(GenFreq);
                    BKUP_SaveFGen(GenFreq);
                }
                else
                {
                    GEN_SetMeasurementFreq(currF);
                }
            }
            else
            {
                GEN_SetMeasurementFreq(currF);
            }
        }
        ShowF();
        Sleep(50);
    }
    return 0; //no request to change window
}


void GENERATOR_Proc(void)
{
    //Load saved frequency value from BKUP registers 4, 5
    //to GenFreq
    {
        uint32_t fbkup = BKUP_LoadFGen();
        if (fbkup >= GEN_BAND_FMIN && fbkup <= GEN_BAND_FMAX && (fbkup % 5000) == 0)
        {
            GenFreq = fbkup;
        }
        else
        {
            GenFreq = 14000000ul;
            BKUP_SaveFGen(GenFreq);
        }
    }

    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 70, 2, "Generator mode");
    Sleep(250);
    while(TOUCH_IsPressed());

    //Draw freq change areas bar
    uint16_t y;
    for (y = 0; y <2; y++)
    {
        LCD_Line(LCD_MakePoint(0,y), LCD_MakePoint(39,y), LCD_RGB(15,15,63));
        LCD_Line(LCD_MakePoint(40,y), LCD_MakePoint(89,y), LCD_RGB(31,31,127));
        LCD_Line(LCD_MakePoint(90,y), LCD_MakePoint(155,y),  LCD_RGB(64,64,255));
        LCD_Line(LCD_MakePoint(165,y), LCD_MakePoint(230,y), LCD_RGB(64,64,255));
        LCD_Line(LCD_MakePoint(231,y), LCD_MakePoint(280,y), LCD_RGB(31,31,127));
        LCD_Line(LCD_MakePoint(281,y), LCD_MakePoint(319,y), LCD_RGB(15,15,63));
    }

    GEN_SetMeasurementFreq(GenFreq);
    ShowF();
    while(1)
    {
        if(Touch())
        {
            GEN_SetMeasurementFreq(0);
            return; //change window
        }

        //Measure without changing current frequency and without any corrections
        DSP_Measure(0, 0, 0, 7);
        FONT_SetAttributes(FONT_FRAN, LCD_WHITE, LCD_BLACK);
        FONT_ClearLine(FONT_FRAN, LCD_BLACK, 100);
        FONT_Printf(0, 100, "Raw Vi: %.1f mV, Vq: %.1f mV. Diff %.2f dB", DSP_MeasuredMagQmv(),
                     DSP_MeasuredMagImv(), DSP_MeasuredDiffdB());
        FONT_ClearLine(FONT_FRAN, LCD_BLACK, 120);
        FONT_Printf(0, 120, "Raw phase diff: %.1f deg", DSP_MeasuredPhaseDeg());
        static DSP_RX rx;
        rx = DSP_MeasuredZ();
        FONT_ClearLine(FONT_FRAN, LCD_BLACK, 140);
        FONT_Printf(0, 140, "Raw R: %.1f X: %.1f", crealf(rx), cimagf(rx));

        if (DSP_MeasuredMagImv() < 100. && DSP_MeasuredMagQmv() < 100.)
        {
            FONT_Write(FONT_FRAN, LCD_BLACK, LCD_RED, 0, 160,   "No signal  ");
        }
        else
        {
            FONT_Write(FONT_FRAN, LCD_GREEN, LCD_BLACK, 0, 160, "Signal OK   ");
        }

        Sleep(100);
    }
};
