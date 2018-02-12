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
#include "panfreq.h"
extern uint32_t BackGrColor;
extern uint32_t TextColor;

static uint32_t fChanged = 0;
static uint32_t rqExit = 0;
static uint32_t f_maxstep = 500000;
static uint32_t redrawWindow = 0;
static uint32_t redrawWindowCompl;

void Sleep(uint32_t ms);

static void ShowF()
{
char str[20] = "";
uint8_t i,j;
    FONT_ClearHalfLine(FONT_FRANBIG, BackGrColor, 72);// WK
    sprintf(str, "F: %#u Hz", (unsigned int)(CFG_GetParam(CFG_PARAM_GEN_F) ));
    for(i=3;i<14;i++){
        if(str[i]==' ') break;// search space before "Hz"
    }
    for(j=i+3;j>i-4;j--){
       str[j+2]=str[j];
    }
     for(j=i-4;j>i-7;j--){
       str[j+1]=str[j];
    }
    str[i-6]='.';
    str[i-2]='.';
    str[i+5]=0;
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 0, 72, str );
}

static void GENERATOR_SwitchWindow(void)
{
    rqExit = 1;
}

static void FDecr(uint32_t step)
{
    uint32_t MeasurementFreq = CFG_GetParam(CFG_PARAM_GEN_F);
   /* if((MeasurementFreq > step) && (MeasurementFreq % step != 0))
    {
        MeasurementFreq -= (MeasurementFreq % step);
        CFG_SetParam(CFG_PARAM_GEN_F, MeasurementFreq);
        fChanged = 1;
    }*/
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
  /*  if(MeasurementFreq > step && MeasurementFreq % step != 0)
    {
        MeasurementFreq -= (MeasurementFreq % step);
        CFG_SetParam(CFG_PARAM_GEN_F, MeasurementFreq);
        fChanged = 1;
    }*/
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
static void GENERATOR_FDecr_10k(void)
{
    FDecr(10000);
}
static void GENERATOR_FDecr_1k(void)
{
    FDecr(1000);
}
static void GENERATOR_FDecr_100Hz(void)// WK
{
    FDecr(100);
}
static void GENERATOR_FDecr_10Hz(void)// WK
{
    FDecr(10);
}

static void GENERATOR_FIncr_10Hz(void)
{
    FIncr(10);
}

static void GENERATOR_FIncr_100Hz(void)
{
    FIncr(100);
}

static void GENERATOR_FIncr_1k(void)
{
    FIncr(1000);
}
static void GENERATOR_FIncr_10k(void)
{
    FIncr(10000);
}
static void GENERATOR_FIncr_100k(void)
{
    FIncr(100000);
}
static void GENERATOR_FIncr_500k(void)
{
    FIncr(f_maxstep);
}
static uint32_t fx = 14000000ul; //Scan range start frequency, in Hz
static uint32_t fxkHz;//Scan range start frequency, in kHz
static BANDSPAN *pBs1;

static void GENERATOR_SetFreq(void)
{
//    while(TOUCH_IsPressed()); WK
    fx=CFG_GetParam(CFG_PARAM_GEN_F);
    fxkHz=fx/1000;
    if (PanFreqWindow(&fxkHz, (BANDSPAN*)&pBs1))
        {
            //Span or frequency has been changed
            CFG_SetParam(CFG_PARAM_GEN_F, fxkHz*1000);
        }
   // uint32_t val = NumKeypad(CFG_GetParam(CFG_PARAM_MEAS_F)/1000, BAND_FMIN/1000, CFG_GetParam(CFG_PARAM_BAND_FMAX)/1000, "Set measurement frequency, kHz");
    CFG_Flush();
    redrawWindow = 1;
    Sleep(200);
}
void GENERATOR_ChgColrs(void){
    if(ColourSelection==0)ColourSelection=1;
    else ColourSelection=0;
    SetColours();
    redrawWindowCompl = 1;
 //   while(TOUCH_IsPressed());
}
static const struct HitRect GENERATOR_hitArr[] =
{
    //        x0,  y0, width, height, callback
    HITRECT(   0, 233,  80, 38, GENERATOR_SwitchWindow),//200
    HITRECT(  90, 233, 240, 38, GENERATOR_SetFreq),
    HITRECT( 334, 233, 144, 38, GENERATOR_ChgColrs),
    HITRECT( 290,   1,  90, 35, GENERATOR_FDecr_500k),
    HITRECT( 290,  40,  90, 35, GENERATOR_FDecr_100k),
    HITRECT( 290,  79,  90, 35, GENERATOR_FDecr_10k),
    HITRECT( 290, 118,  90, 35, GENERATOR_FDecr_1k),
    HITRECT( 290, 157,  90, 35, GENERATOR_FDecr_100Hz),
    HITRECT( 290, 196,  90, 35, GENERATOR_FDecr_10Hz),
    HITRECT( 380, 196,  90, 35, GENERATOR_FIncr_10Hz),
    HITRECT( 380, 157,  90, 35, GENERATOR_FIncr_100Hz),
    HITRECT( 380, 118,  90, 35, GENERATOR_FIncr_1k),
    HITRECT( 380,  79,  90, 35, GENERATOR_FIncr_10k),
    HITRECT( 380,  40,  90, 35, GENERATOR_FIncr_100k),
    HITRECT( 380,   1,  90, 35, GENERATOR_FIncr_500k),
    HITEND
};
void ShowIncDec1(void){
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,300,   2, "-0.5M");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,300,  41, "-0.1M");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,300,  80, "-10k");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,300, 119, "-1k");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,300, 158, "-0.1k");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,300, 197, "-10Hz");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,380, 197, "+10Hz");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,380, 158, "+0.1k");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,380, 119, "+1k");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,380,  80, "+10k");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,380,  41, "+0.1M");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,380,   2, "+0.5M");
}
void SetColours(void){
 if(ColourSelection==0){// Daylight
        BackGrColor=LCD_WHITE;
        CurvColor=LCD_COLOR_DARKGREEN;// dark blue
        TextColor=LCD_BLACK;
        Color1=LCD_COLOR_DARKGREEN;//LCD_COLOR_DARKBLUE;
        Color2=LCD_COLOR_DARKGRAY;
        Color3=LCD_YELLOW;// -ham bands area at daylight
        Color4=LCD_MakeRGB(128,255,128);//Light green

    }
    else{// Inhouse
        BackGrColor=LCD_BLACK;
        CurvColor=LCD_COLOR_LIGHTGREEN;
        TextColor=LCD_WHITE;
        Color1=LCD_COLOR_LIGHTBLUE;
        Color2=LCD_COLOR_GRAY;
        Color3=LCD_MakeRGB(0, 0, 64);// very dark blue -ham bands area
        Color4=LCD_MakeRGB(0, 64, 0);// very dark green
    }
}

void GENERATOR_Window_Proc(void)
{
   uint32_t activeLayer;
   LCDPoint pt;
   uint8_t i;

    while(TOUCH_IsPressed());
    SetColours();
    {
        uint32_t fbkup = CFG_GetParam(CFG_PARAM_GEN_F);
        if (!(fbkup >= BAND_FMIN && fbkup <= CFG_GetParam(CFG_PARAM_BAND_FMAX) && (fbkup % 1000) == 0))
        {
            CFG_SetParam(CFG_PARAM_GEN_F, 14000000);
            CFG_Flush();
        }
    }
    redrawWindowCompl = 1;
GENERATOR_REDRAW:
    GEN_SetMeasurementFreq(CFG_GetParam(CFG_PARAM_GEN_F));
    if(redrawWindowCompl == 1){
        redrawWindowCompl = 0;
        LCD_FillAll(BackGrColor);
        ShowHitRect(GENERATOR_hitArr);
        ShowIncDec1();
        FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 0, 36, "Generator mode");// WK
        FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 2, 235, "  Exit ");
        FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 95, 235, "Set frequency (kHz)");
        FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 339, 235, "ChgColours");
    }
    ShowF();

    uint32_t speedcnt = 0;
    f_maxstep = 500000;
    rqExit = 0;
    fChanged = 1;// WK
    redrawWindow = 0;

    while(1)
    {
        for(i=0;i<10;i++){
            if(redrawWindowCompl == 1)   goto GENERATOR_REDRAW;
            while (TOUCH_Poll(&pt))
            {
                if(HitTest(GENERATOR_hitArr, pt.x, pt.y)==1){
                    if (rqExit)
                    {
                        GEN_SetMeasurementFreq(0);
                        return; //Change window
                    }
                    if (redrawWindow)
                    {
                        redrawWindowCompl = 0;
                        goto GENERATOR_REDRAW;
                    }
                    if (fChanged)
                    {
                        ShowF();
                        GEN_SetMeasurementFreq(CFG_GetParam(CFG_PARAM_GEN_F));
                    }
                    speedcnt++;
                    if (speedcnt < 10)
                        Sleep(500);// WK
                    else
                        Sleep(200);// WK
                    if (speedcnt > 50)
                        f_maxstep = 2000000;
                }
            }
            Sleep(50);
        }
        speedcnt = 0;
        f_maxstep = 500000;
        if (fChanged)
        {
            CFG_Flush();// save all settings
            GEN_SetMeasurementFreq(CFG_GetParam(CFG_PARAM_GEN_F));
            fChanged = 0;
            ShowF();
        }

        //Measure without changing current frequency and without OSL
        DSP_Measure(0, 1, 0, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));

        FONT_SetAttributes(FONT_FRAN, TextColor, BackGrColor);
        FONT_ClearHalfLine(FONT_FRAN, BackGrColor, 100);
        FONT_Printf(0, 100, "Raw Vi: %.2f mV, Vv: %.2f mV Diff %.2f dB", DSP_MeasuredMagImv(),
                     DSP_MeasuredMagVmv(), DSP_MeasuredDiffdB());
        FONT_ClearHalfLine(FONT_FRAN, BackGrColor, 120);
        FONT_Printf(0, 120, "Raw phase diff: %.1f deg", DSP_MeasuredPhaseDeg());
        static DSP_RX rx;
        rx = DSP_MeasuredZ();
        FONT_ClearHalfLine(FONT_FRAN, BackGrColor, 140);
        FONT_Printf(0, 140, "Raw R: %.1f X: %.1f", crealf(rx), cimagf(rx));

        if (DSP_MeasuredMagVmv() < 1.f)
        {
            FONT_Write(FONT_FRAN, LCD_RED, BackGrColor, 0, 160,   "No signal  ");
        }
        else
        {
            FONT_Write(FONT_FRAN, Color1, BackGrColor, 0, 160, "Signal OK   ");
        }
        if(TOUCH_Poll(&pt)!=0) continue;
        DSP_Measure(0, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        rx = DSP_MeasuredZ();
        FONT_ClearHalfLine(FONT_FRAN, BackGrColor, 180);
        FONT_Printf(0, 180, "With OSL R: %.1f X: %.1f", crealf(rx), cimagf(rx));
    }
}
