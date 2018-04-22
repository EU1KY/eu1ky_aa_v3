/*
 *   (c) Wolfgang Kiefer, DH1AKF, 2018
 *   woki@online.de
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include "arm_math.h"
#include <complex.h>
#include <string.h>

#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "config.h"
#include "ff.h"
#include "crash.h"
#include "dsp.h"
#include "gen.h"
#include "oslfile.h"
#include "stm32746g_discovery_lcd.h"
#include "screenshot.h"
#include "panvswr2.h"
#include "panfreq.h"
#include "textbox.h"
#include "spectr.h"
#include "fftwnd.h"

extern int16_t audioBuf[(NSAMPLES + NDUMMY) * 2];
extern void GEN_SetLOFreq(uint32_t frqu1);
extern unsigned long GetUpper(int i);
extern unsigned long GetLower(int i);
extern float rfft_mags[NSAMPLES/2];

extern int16_t audioBuf[(NSAMPLES + NDUMMY) * 2];
extern float rfft_input[NSAMPLES];
extern float rfft_output[NSAMPLES];
extern const float complex *prfft;
extern float windowfunc[NSAMPLES];

#define Fieldw1 70
#define FieldH 36

#define fMin 400000ul
static int16_t minMag;
static int16_t maxMag;
static int16_t mag;
static int32_t magnitude ;
static int i, AreaSelected;
static unsigned long upper, lower;
uint32_t f1;
BANDSPAN span;
int selector, sExit;
unsigned long Freq1;


uint32_t MeasPower(unsigned long frqu1){
    minMag = 32767;
    maxMag = -32767;
    GEN_SetLOFreq(frqu1);
    DSP_Sample16();
  //Calc max magnitude for channel 1
    for (i = 2 * 2; i <= (2 + 10) * 2; i += 2){// 20 samples
        mag=audioBuf[i];
        if (minMag > mag)
            minMag = mag;
        if (maxMag < mag)
            maxMag = mag;
    }

    magnitude = (maxMag - minMag) / 2;
    return magnitude;
}

static unsigned long actFreq;
static uint32_t  power00;

void MeasPower00(unsigned long fUpper){
uint32_t  power01, power02;
    power00=MeasPower(fUpper);// 3 testing frequencies
    power01=MeasPower(fUpper-20000ul);
    power02=MeasPower(fUpper-40000ul);

    if(power00>power01){// search for median
        if(power02<power01)
            power00=power01;
        else if(power02<power00)
            power00=power02;
    } else{                 // power00<power01
        if(power01<power02)
            power00=power01;
        else if(power02>power00)
            power00=power02;
    }
}

static void Calc_fft_audiobuf(int ch)
{
    //Only one channel
    int i;
    arm_rfft_fast_instance_f32 S;

    int16_t* pBuf = &audioBuf[NDUMMY + (ch != 0)];
    for(i = 0; i < NSAMPLES; i++)
    {
        rfft_input[i] = (float)*pBuf * windowfunc[i];
        pBuf += 2;
    }

    arm_rfft_fast_init_f32(&S, NSAMPLES);
    arm_rfft_fast_f32(&S, rfft_input, rfft_output, 0);

    for (i = 0; i < NSAMPLES/2; i++)
    {
        float complex binf = prfft[i];
        rfft_mags[i] = cabsf(binf) / (NSAMPLES/2);
    }
}


uint32_t SearchFrequ(unsigned long upper1, unsigned long lower1){
uint32_t power, lastpower;
uint32_t i,found=0;

char string1[40];
LCDPoint pt;

    MeasPower00(upper1);
    // search for first interference
    for(actFreq=upper1;actFreq>=lower1; actFreq-=20000ul){// 3 ms per step
        lastpower=power=MeasPower(actFreq);
        if(power>4*power00) {
             found=1;
             break;
        }
        if(2*power>3*power00){
           power=MeasPower(actFreq);// second measurement
           if(power>2*power00) {
                   found=1;
                   break;
           }
        }
        power00=(5*power00+power)/6;// sliding, weighted medium
        if((actFreq%200000)==0){
            sprintf(string1,"power: %d, F: %d kHz", power, actFreq/1000);
            LCD_FillRect((LCDPoint){180 ,100},(LCDPoint){479,160},BackGrColor);
            FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 180, 100, string1);
        }
         if (TOUCH_Poll(&pt))  return found;
    }
    DSP_Sample();
    Calc_fft_audiobuf(0);

    //Calculate max magnitude bin
    float maxmag = 0.;
    int idxmax = -1;
    for (i = 1; i < NSAMPLES/2 ; i++)
    {
        if (rfft_mags[i] > maxmag)
        {
            maxmag = rfft_mags[i];
            idxmax = i;
        }
    }
    actFreq=actFreq-((idxmax)*24000)/(NSAMPLES/2);


    sprintf(string1,"Power: %d,  F: %.1f kHz", power, (float)actFreq/1000);
    LCD_FillRect((LCDPoint){180 ,100},(LCDPoint){479,160},BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 180, 100, string1);
    sprintf(string1,"i: %d  ", idxmax);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 180, 160, string1);

    return found;
}




bool OneBand(void){
    /*
int band;
    selector=1;
    if (AreaSelected==0){
        PanFreqWindow(&f1, &span);
        band=GetBandNr((unsigned long)f1*1000);
        if(band!=-1){
           upper=GetUpper(band);
           lower=GetLower(band);
        }
        else{
            upper=(f1 + BSVALUES[span] / 2)*1000;
            lower=(f1 - BSVALUES[span] / 2)*1000;
        }
        if(lower<400000) lower = 400000;
        AreaSelected=1;
    }
    if(SearchFrequ(upper, lower)==1) return true;*/
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 180, 100, "Null   ");
    return false;
}

bool Full(void){
    while(TOUCH_IsPressed());
    Sleep(100);
    selector=3;
    if(SearchFrequ(30000000ul,400000ul)==1) return true;
    return false;
}

bool HamBands(void){
    while(TOUCH_IsPressed());
    Sleep(100);
    selector=2;
    int i;


    if(AreaSelected==1){// HF
        for(i=8;i>=0;i--){
            if(SearchFrequ(GetUpper(i), GetLower(i))==1) return true;
        }

    }
    else if(AreaSelected==2){
         for(i=11;i>=10;i--){
            if(SearchFrequ(GetUpper(i), GetLower(i))==1) return true;
        }
    }
    else {
           if(SearchFrequ(GetUpper(12), GetLower(12))==1) return true;
    }

    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 180, 100, "Null   ");
    return false;
}

void Auto(void){
    for(;;){
        if(TOUCH_IsPressed()) return;
        if(selector==1) {
            if(OneBand())
                return;
        }
        else if (selector==2) {
            if(HamBands()) return;
        }
        else if(Full()) return;
    }
}

void SCExit(void){
    sExit=1;
}


static const TEXTBOX_t tb_Scan[] = {
    (TEXTBOX_t){ .x0 = 20, .y0 = 60, .text = "Full Scale", .font = FONT_FRANBIG, .width = 120, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))Full, .cbparam = 1, .next = (void*)&tb_Scan[1] },
    (TEXTBOX_t){ .x0 = 20, .y0 = 110, .text = "Ham Bands", .font = FONT_FRANBIG, .width = 120, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))HamBands, .cbparam = 1,  .next = (void*)&tb_Scan[2] },
    (TEXTBOX_t){ .x0 = 20, .y0 = 160, .text = "One Band", .font = FONT_FRANBIG, .width = 120, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RED, .cb = (void(*)(void))OneBand, .cbparam = 1,  .next = (void*)&tb_Scan[3] },
    (TEXTBOX_t){ .x0 = 2, .y0 = 234, .text = " Exit ", .font = FONT_FRANBIG, .width = 100, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))SCExit, .cbparam = 1, .next =(void*)&tb_Scan[4] },
    (TEXTBOX_t){ .x0 = 80, .y0 = 234, .text = " Auto", .font = FONT_FRANBIG, .width = 150, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))Auto, .cbparam = 1,},

};



void SPECTR_FindFreq(void){
TEXTBOX_CTX_t fctx;
    sExit=0;
    AreaSelected=1;
    selector=2;
    f1=3700;
    GEN_Init();// fOsc, fLO off
    SetColours();
    LCD_FillAll(BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 120, 20, "Find frequency scan mode");
    Sleep(600);
    while(TOUCH_IsPressed());
    TEXTBOX_InitContext(&fctx);
    TEXTBOX_Append(&fctx, (TEXTBOX_t*)tb_Scan); //Append the very first element of the list in the flash.
                                                      //It is enough since all the .next fields are filled at compile time.
    TEXTBOX_DrawContext(&fctx);
    //PanFreqWindow(&f1, &span);

    while(TOUCH_IsPressed());

    for(;;){
        if (TEXTBOX_HitTest(&fctx))
        {
            Sleep(50);
        }
        if(sExit==1) return;
       /* LCDPoint pt;
        if (TOUCH_Poll(&pt))
            return;*/
    }
}
