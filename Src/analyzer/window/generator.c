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
#include "gen.h"
#include "config.h"
#include "hit.h"
#include "main.h"
#include "num_keypad.h"

static uint32_t fChanged = 0;
static uint32_t rqExit = 0;
static uint32_t f_maxstep = 500000;
static uint32_t redrawWindow = 0;

void Sleep(uint32_t ms);

static void ShowF()
{
    char str[50];
    sprintf(str, "F: %u kHz", (unsigned int)(CFG_GetParam(CFG_PARAM_GEN_F) / 1000));
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 40);
    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 160 - FONT_GetStrPixelWidth(FONT_FRANBIG, str) / 2, 40, str);
}

static void GENERATOR_SwitchWindow(void)
{
    rqExit = 1;
}

static void FDecr(uint32_t step)
{
    uint32_t MeasurementFreq = CFG_GetParam(CFG_PARAM_GEN_F);
    if(MeasurementFreq > step && MeasurementFreq % step != 0)
    {
        MeasurementFreq -= (MeasurementFreq % step);
        CFG_SetParam(CFG_PARAM_GEN_F, MeasurementFreq);
        fChanged = 1;
    }
    if(MeasurementFreq < BAND_FMIN)
    {
        MeasurementFreq = BAND_FMIN;
        CFG_SetParam(CFG_PARAM_GEN_F, MeasurementFreq);
        fChanged = 1;
    }
    if(MeasurementFreq > BAND_FMIN)
    {
        if(MeasurementFreq > step && (MeasurementFreq - step) >= BAND_FMIN)
        {
            MeasurementFreq = MeasurementFreq - step;
            CFG_SetParam(CFG_PARAM_GEN_F, MeasurementFreq);
            fChanged = 1;
        }
    }
}

static void FIncr(uint32_t step)
{
    uint32_t MeasurementFreq = CFG_GetParam(CFG_PARAM_GEN_F);
    if(MeasurementFreq > step && MeasurementFreq % step != 0)
    {
        MeasurementFreq -= (MeasurementFreq % step);
        CFG_SetParam(CFG_PARAM_GEN_F, MeasurementFreq);
        fChanged = 1;
    }
    if(MeasurementFreq < BAND_FMIN)
    {
        MeasurementFreq = BAND_FMIN;
        CFG_SetParam(CFG_PARAM_GEN_F, MeasurementFreq);
        fChanged = 1;
    }
    if(MeasurementFreq < CFG_GetParam(CFG_PARAM_BAND_FMAX))
    {
        if ((MeasurementFreq + step) > CFG_GetParam(CFG_PARAM_BAND_FMAX))
            MeasurementFreq = CFG_GetParam(CFG_PARAM_BAND_FMAX);
        else
            MeasurementFreq = MeasurementFreq + step;
        CFG_SetParam(CFG_PARAM_GEN_F, MeasurementFreq);
        fChanged = 1;
    }
}

static void GENERATOR_FDecr_500k(void)
{
    FDecr(f_maxstep);
}
static void GENERATOR_FDecr_100k(void)
{
    FDecr(100000);
}
static void GENERATOR_FDecr_5k(void)
{
    FDecr(5000);
}
static void GENERATOR_FIncr_5k(void)
{
    FIncr(5000);
}
static void GENERATOR_FIncr_100k(void)
{
    FIncr(100000);
}
static void GENERATOR_FIncr_500k(void)
{
    FIncr(f_maxstep);
}

static void GENERATOR_SetFreq(void)
{
    while(TOUCH_IsPressed());
    uint32_t val = NumKeypad(CFG_GetParam(CFG_PARAM_GEN_F)/1000, BAND_FMIN/1000, CFG_GetParam(CFG_PARAM_BAND_FMAX)/1000, "Set generator frequency, kHz");
    CFG_SetParam(CFG_PARAM_GEN_F, val * 1000);
    CFG_Flush();
    redrawWindow = 1;
}

static const struct HitRect hitArr[] =
{
    //        x0,  y0, width, height, callback
    HITRECT(   0, 200,   100,     79, GENERATOR_SwitchWindow),
    HITRECT( 150, 200,   100,     79, GENERATOR_SetFreq),
    HITRECT(   0,   0,  80, 150, GENERATOR_FDecr_500k),
    HITRECT(  80,   0,  80, 150, GENERATOR_FDecr_100k),
    HITRECT( 160,   0,  70, 150, GENERATOR_FDecr_5k),
    HITRECT( 250,   0,  70, 150, GENERATOR_FIncr_5k),
    HITRECT( 320,   0,  80, 150, GENERATOR_FIncr_100k),
    HITRECT( 400,   0,  80, 150, GENERATOR_FIncr_500k),
    HITEND
};


void GENERATOR_Window_Proc(void)
{
    {
        uint32_t fbkup = CFG_GetParam(CFG_PARAM_GEN_F);
        if (!(fbkup >= BAND_FMIN && fbkup <= CFG_GetParam(CFG_PARAM_BAND_FMAX) && (fbkup % 1000) == 0))
        {
            CFG_SetParam(CFG_PARAM_GEN_F, 14000000);
            CFG_Flush();
        }
    }
GENERATOR_REDRAW:
    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 70, 2, "Generator mode");

    while(TOUCH_IsPressed());

    uint32_t speedcnt = 0;
    f_maxstep = 500000;

    //Draw freq change areas bar
    uint16_t y;
    for (y = 0; y <2; y++)
    {
        LCD_Line(LCD_MakePoint(0,y), LCD_MakePoint(79,y), LCD_RGB(15,15,63));
        LCD_Line(LCD_MakePoint(80,y), LCD_MakePoint(159,y), LCD_RGB(31,31,127));
        LCD_Line(LCD_MakePoint(160,y), LCD_MakePoint(229,y),  LCD_RGB(64,64,255));
        LCD_Line(LCD_MakePoint(250,y), LCD_MakePoint(319,y), LCD_RGB(64,64,255));
        LCD_Line(LCD_MakePoint(320,y), LCD_MakePoint(399,y), LCD_RGB(31,31,127));
        LCD_Line(LCD_MakePoint(400,y), LCD_MakePoint(479,y), LCD_RGB(15,15,63));
    }

    FONT_Write(FONT_FRAN, LCD_GREEN, LCD_RGB(0, 0, 64), 0, 250, "    Exit    ");
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLUE, 150, 250, "  Set frequency...  ");

    rqExit = 0;
    fChanged = 0;
    redrawWindow = 0;

    GEN_SetMeasurementFreq(CFG_GetParam(CFG_PARAM_GEN_F));
    ShowF();
    while(1)
    {
        LCDPoint pt;
        while (TOUCH_Poll(&pt))
        {
            HitTest(hitArr, pt.x, pt.y);
            if (rqExit)
            {
                GEN_SetMeasurementFreq(0);
                while(TOUCH_IsPressed());
                return; //Change window
            }
            if (redrawWindow)
            {
                goto GENERATOR_REDRAW;
            }
            if (fChanged)
            {
                ShowF();
            }
            speedcnt++;
            if (speedcnt < 20)
                Sleep(100);
            else
                Sleep(30);
            if (speedcnt > 50)
                f_maxstep = 2000000;
        }
        speedcnt = 0;
        f_maxstep = 500000;
        if (fChanged)
        {
            CFG_Flush();
            GEN_SetMeasurementFreq(CFG_GetParam(CFG_PARAM_GEN_F));
            fChanged = 0;
        }

        //Measure without changing current frequency and without any corrections
        DSP_Measure(0, 1, 0, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        FONT_SetAttributes(FONT_FRAN, LCD_WHITE, LCD_BLACK);
        LCD_WaitForRedraw();
        FONT_ClearLine(FONT_FRAN, LCD_BLACK, 100);
        FONT_Printf(0, 100, "Raw I: %.1f, V: %.1f. Diff %.2f dB", DSP_MeasuredMagImv(),
                     DSP_MeasuredMagVmv(), DSP_MeasuredDiffdB());
        FONT_ClearLine(FONT_FRAN, LCD_BLACK, 120);
        FONT_Printf(0, 120, "Raw phase diff: %.1f deg", DSP_MeasuredPhaseDeg());
        static DSP_RX rx;
        rx = DSP_MeasuredZ();
        FONT_ClearLine(FONT_FRAN, LCD_BLACK, 140);
        FONT_Printf(0, 140, "Raw R: %.1f X: %.1f", crealf(rx), cimagf(rx));

        if (DSP_MeasuredMagVmv() < 20.)
        {
            FONT_Write(FONT_FRAN, LCD_BLACK, LCD_RED, 0, 160,   "No signal  ");
        }
        else
        {
            FONT_Write(FONT_FRAN, LCD_GREEN, LCD_BLACK, 0, 160, "Signal OK   ");
        }
        Sleep(100);
    }
}
