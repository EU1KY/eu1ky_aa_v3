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

extern float rfft_input[NSAMPLES];
extern float rfft_output[NSAMPLES];
extern const float complex *prfft;
extern float windowfunc[NSAMPLES];

#define Fieldw1 70
#define FieldH 36

const uint32_t Scale_Factors[]={10,8,10,10,8,10,10,8,10,10,8,10,10,8,10,10};
static int16_t maxMag;
static int16_t mag;
static int m, AreaSelected, cycl3;
static unsigned long upper, lower;
static BANDSPAN span;
static int selector, sExit;
static int auto1, sScan_Meas, sRepaint;
static LCDPoint pt;
static int FreqFound;
static uint32_t f1;// kHz
static unsigned long actFreq;
static uint32_t  power00;
static float SumVal;


static void Calc_fft_audiobuf(int ch)
{
    //Only one channel
    int i;
    arm_rfft_fast_instance_f32 S;

    int16_t* pBuf = &audioBuf[NDUMMY + (ch != 0)];
    for(i = 0; i < NSAMPLES; i++)
    {

        rfft_input[i] = (float)*pBuf* windowfunc[i];// Blackman window
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

static float Maxmag, Average;

int CalcMaxBin(unsigned long frqu1){
int n,idxmax = -1;
float Val;

    if(frqu1!=0)// 0: don't toggle frequency
        GEN_SetLOFreq(frqu1);
    DSP_Sample();
    Calc_fft_audiobuf(0);
    //Calculate max magnitude bin
    SumVal=0.0;
    for (n = 4; n < 193; n++)// 400 Hz .. 18 kHz
    {
        Val=rfft_mags[n];
        SumVal+=Val;
        if (Val > Maxmag)// over full search area
        {
            Maxmag = Val;
            idxmax = n;
        }
    }
                    // calculate noise background:
    SumVal-=Maxmag;//exclude maximum
    SumVal-=fabsf(rfft_mags[idxmax-1]);// exclude neighbors of maximum
    SumVal-=fabsf(rfft_mags[idxmax+1]);
    Average=(2*Average+SumVal/186)/3;

    if(Maxmag>2*Average)// Maximum minus medium value/ Maximum
        return idxmax;
    else return 0;// no peak in the bin's
}


static int counter;
static char string1[40];
static unsigned long fx;
static int binx, found;

void    FX_ShowResult (void){

float   PowerMax, powermw, powerdb;
unsigned long testFreq, fmaxSIhz = CFG_GetParam(CFG_PARAM_SI5351_MAX_FREQ);


        PowerMax=Maxmag*5.0f;
        if(fx>fmaxSIhz)   PowerMax*=1.5;
    if((found==1)&&(PowerMax>164.0)){ // trigger value: ~ 2.2mV <=>      -40 dBm: 164
                                        // trigger value: ~ 0.7mV <=>   -50 dBm: 51.4
                                        // trigger value: 0.5 mV        -53 dBm  36.7
                                        // trigger value: ~ 0.224mV <=> -60 dBm: 16.4

        testFreq=fx-(int)(float)(binx*93.75f);
        if(PowerMax>500.0)
            PowerMax=PowerMax*0.0149f-0.5587f;// must be better calibrated !!
        else
            PowerMax=PowerMax*0.0137f-0.0462f;// mV
        powermw=PowerMax*PowerMax/50000.0f;
        powerdb=10.0*log10f(powermw);
        if(auto1==0)
            LCD_FillRect((LCDPoint){132 ,100},(LCDPoint){479,160},BackGrColor);
        if(testFreq>100000000ul)
            sprintf(string1,"U: %.1f mV, F: %.3f MHz     ", PowerMax, (float)(testFreq+500)/1000000);
        else
            sprintf(string1,"U: %.1f mV, F: %.1f kHz     ", PowerMax, (float)(testFreq)/1000);
        FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 132, 100, string1);
        if(powermw>1.0f)
        sprintf(string1,"P: %.3f mW = %.1f dBm  ", powermw, powerdb);
        else sprintf(string1,"P: %.3f uW = %.0f dBm    ", powermw*1000, powerdb);
        FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 132, 160, string1);
    }
    else{
        LCD_FillRect((LCDPoint){132 ,100},(LCDPoint){479,190},BackGrColor);
        FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 150, 160, "No signal found");
    }
}

void SetUpperLower(void){
    if(span>15) span=15;
    if(span>=5){
        if(CFG_GetParam(CFG_PARAM_PAN_CENTER_F)==0){
            lower=f1 *1000;
            upper=(f1 + BSVALUES[span])*1000;
        }
        else{
            upper=(f1 + BSVALUES[span] / 2)*1000;
            lower=(f1 - BSVALUES[span] / 2)*1000;
        }
    }
    else {
        lower=f1*1000;
        upper=(f1 + BSVALUES[span])*1000;
    }
}

int OneBandExec(void){
int res;
    found=0;
    Maxmag=0.5;
    res=SpectrumExec(0);
    if (res>=2) {
        if(fx>100000000ul){
            upper=fx+20000ul;
            lower=fx-20000ul;
            res=SpectrumExec(0);// fine tuning
        }
        if(res==-1) return found;
        found=1;
        FX_ShowResult();
        FreqFound=1;//
    }
    else   {
        LCD_FillRect((LCDPoint){132 ,100},(LCDPoint){479,190},BackGrColor);
        if(res==-1) return -1; // touch interrupt
    }
    return 0;
}

int ScanFull(void){
int k, res;
unsigned long fx1;
int binx1;
    LCD_FillRect((LCDPoint){132 ,100},(LCDPoint){479,190},BackGrColor);

    found=0;
    Maxmag=0.005;
    FreqFound=0;

    upper=30000000ul;
    lower=500000ul;
    res=SpectrumExec(0);// coarse tuning
    if(res>=2) {
        upper=fx+100000ul;
        lower=fx-100000ul;
        res=SpectrumExec(0);// fine tuning
    }
    if(res==-1) return found;
    found=1;
    FX_ShowResult();//show result
    FreqFound=1;
    return found;
}

void Full(void){

    if(sScan_Meas==1) return;
    while(TOUCH_IsPressed());
    Sleep(100);
    if(selector==3) selector=0;
    else    {
        selector=3;
        LCD_Rectangle((LCDPoint){2,60},(LCDPoint){122,96}, 0xffff0000);// red (active)
        LCD_Rectangle((LCDPoint){3,61},(LCDPoint){121,95}, 0xffff0000);// red (active)
        LCD_Rectangle((LCDPoint){2,160},(LCDPoint){122,196}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){3,161},(LCDPoint){121,195}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){2,110},(LCDPoint){122,146}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){3,109},(LCDPoint){121,145}, 0xffffffff);//white
        ScanFull();
    }
    selector=0;
    LCD_Rectangle((LCDPoint){2,60},(LCDPoint){122,96}, 0xffffffff);//white
    LCD_Rectangle((LCDPoint){3,61},(LCDPoint){121,95}, 0xffffffff);//white
}

int OneBand(void){
int band, ret;

    if(sScan_Meas==1) return 0;
    LCD_FillRect((LCDPoint){132 ,100},(LCDPoint){479,190},BackGrColor);
    FreqFound=0;
    cycl3=49;
    while(TOUCH_IsPressed());
    Sleep(100);
    if(selector==1) selector=0;
    else{
        selector=1;
        LCD_Rectangle((LCDPoint){2,160},(LCDPoint){122,196}, 0xffff0000);// red (active)
        LCD_Rectangle((LCDPoint){3,161},(LCDPoint){121,195}, 0xffff0000);// red (active)
        LCD_Rectangle((LCDPoint){2,60},(LCDPoint){122,96}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){3,61},(LCDPoint){121,95}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){2,110},(LCDPoint){122,146}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){3,109},(LCDPoint){121,145}, 0xffffffff);//white
        SetUpperLower();
        if(auto1==0)
            ret=OneBandExec();
        else{
            while (ret>=0){
                ret=OneBandExec();
                cycl3=0;
                while(++cycl3<50){
                    if(TOUCH_IsPressed()) ret=-1;
                    Sleep(20);
                }
                cycl3=0;
            }
        }
    }
    selector=0;
    LCD_Rectangle((LCDPoint){2,160},(LCDPoint){122,196}, 0xffffffff);//white
    LCD_Rectangle((LCDPoint){3,161},(LCDPoint){121,195}, 0xffffffff);//white
    if(ret==-1){

    }
    return ret;
}

void HamBandsExec(void){
int k, ret;
unsigned long fx1;
int binx1;
float Maxmag1;

    Maxmag1=0.5;
    found=0;
    Maxmag=0.5;
    if(AreaSelected==1){// HF
        for(k=0;k<=7;k++){
            upper=GetUpper(k);
            lower=GetLower(k);
            ret=SpectrumExec(0);
            if(ret>=2){
                if(Maxmag>1.1f*Maxmag1){
                   Maxmag1=Maxmag;
                   fx1=fx;
                   binx1=binx;
                   found=1;
                }
            }
            else if (ret==-1) {
                ret=-2;
                break;
            }
        }
        if(ret==-2) return;
        Maxmag=Maxmag1;
        fx=fx1;
        binx=binx1;
        FX_ShowResult();//show result
        FreqFound=1;
        return;
    }
}

void HamBands(void){
    if(sScan_Meas==1) return;
    LCD_FillRect((LCDPoint){132 ,100},(LCDPoint){479,190},BackGrColor);
    FreqFound=0;
    while(TOUCH_IsPressed());
    Sleep(100);
    if(selector==2) selector=0;
    else{
        selector=2;
        LCD_Rectangle((LCDPoint){2,110},(LCDPoint){122,146}, 0xffff0000);// red (active)
        LCD_Rectangle((LCDPoint){3,109},(LCDPoint){121,145}, 0xffff0000);// red (active)
        LCD_Rectangle((LCDPoint){2,60},(LCDPoint){122,96}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){3,61},(LCDPoint){121,95}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){2,160},(LCDPoint){122,196}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){3,161},(LCDPoint){121,195}, 0xffffffff);//white
        HamBandsExec();
    }
    selector=0;
    LCD_Rectangle((LCDPoint){2,110},(LCDPoint){122,146}, 0xffffffff);//white
    LCD_Rectangle((LCDPoint){3,109},(LCDPoint){121,145}, 0xffffffff);//white

}


void SCExit(void){
    if(sScan_Meas==1)
        sRepaint=1;
    else
        sExit=1;
}

void freq(void){
int band;
    while(TOUCH_IsPressed());
    Sleep(100);
    if(span>15) span=15;// (max. 4 MHz)
    PanFreqWindow(&f1, &span);
   // if(selector==1){//  "One Band"
        if(span>=5){
            upper=(f1 + BSVALUES[span] / 2)*1000;
            lower=(f1 - BSVALUES[span] / 2)*1000;
        }
        else {
            lower=f1*1000;
            upper=(f1 + BSVALUES[span])*1000;

       /* else if(span!=100){
            band=GetBandNr(f1*1000);
            if(band!=-1){
               upper=GetUpper(band);
               lower=GetLower(band);
            }
            else{
                upper=(f1 + BSVALUES[span] / 2)*1000;
                lower=(f1 - BSVALUES[span] / 2)*1000;
            }*/

        }
       // OneBandExec();

    AreaSelected=1;
}
static int test;

void Test(void){
    while(TOUCH_IsPressed());
    Sleep(100);
    if(test==0){
        test=1;
        GEN_SetF0Freq(3500000ul);// **** 3500 kHz ****
        LCD_Rectangle((LCDPoint){290,234},(LCDPoint){348,270}, 0xffff0000);//red
        LCD_Rectangle((LCDPoint){291,233},(LCDPoint){347,269}, 0xffff0000);//red
    }
    else {
        test=0;
        GEN_SetF0Freq(0);
        LCD_Rectangle((LCDPoint){290,234},(LCDPoint){348,270}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){291,233},(LCDPoint){347,269}, 0xffffffff);//white
    }
}

static const int dBs[]={10,0,-10,-20,-30,-40,-50,-60,-70};
//static uint32_t color;// for testing purposes
static float signal;
static int breaker;

int Delta(int binMax0, int binMax1, int binMax2){
    breaker=0;
    if((binMax0+binMax1+binMax2==0)){
            breaker=9;
            return 0;
    }
    if((binMax0!=0)&&(binMax1!=0)){
        if((binMax1<=16)&&(binMax0-binMax1>=158)&&(binMax0-binMax1<=162)){
            //color=LCD_COLOR_LIGHTMAGENTA;
            return -binMax0;
        }
        else if((binMax1>=164)&&(binMax1-binMax0>=158)&&(binMax1-binMax0<=162)){// ????****
            //color=LCD_COLOR_YELLOW;
            return binMax0;
        }
    }
     if((binMax1!=0)&&(binMax2!=0)){
        if((binMax2-binMax1>=14)&&(binMax2-binMax1<=18)){
            //color=LCD_COLOR_GREEN;
            return binMax2-176;
        }
        else{
            if((binMax1-binMax2>=14)&&(binMax1-binMax2<=18))
            //color=LCD_COLOR_BLUE;
            return -(176+binMax2);
        }
    }
    if((binMax0>=32)&&(binMax1+binMax2 ==0)){
        //color=LCD_COLOR_RED;
        return binMax0;
    }
    if((binMax0!=0)&&((binMax1!=0)||(binMax2!=0))){
        //color=LCD_COLOR_LIGHTYELLOW;
        return -binMax0;
       }


    if((binMax0+binMax1 ==0)&&(binMax2>=176)){
        //color=LCD_COLOR_LIGHTGREEN;
        return -(binMax2+176);
    }

    if((binMax0!=0)&&(binMax2!=0)&&(binMax0+binMax2>=174)&&(binMax0+binMax2<=178)){
        //color=LCD_COLOR_ORANGE;
        return binMax2-176;
    }
}

void ShowSpectLine(int x){
int y,h;
    h=LCD_GetHeight();

    if((x<480)&&(x>29)&&(signal>0)){
        y = (int) 60 * log10f(signal);
        y = (int)(h- 23 - y);
        if(y<0) y=0;
        if (y >h-69) y=h-69;
        LCD_Line(LCD_MakePoint(x , h-70), LCD_MakePoint(x , y), TextColor);//TextColor
        LCD_Line(LCD_MakePoint(x , y+1), LCD_MakePoint(x , 2), BackGrColor);
    }
}

int SpectrumExec(int graph){

float sum1, flabel, Scalefact,Scalefact1,MaxiMag,steps,MaxMag0,MaxMag1,MaxMag2, Faktor, Faktor1, testFreq, fraction;
int AggregatedPoints,x,lastX,y,l,h,w,k, x0, delta, n, KeyBreaker, case1, UpSb;
int found=0, yofs, LastX, pos;
uint32_t power, t;
unsigned long FLow;
char s[20];
char f[25];
int MaxiBin, binMax0, binMax1, binMax2, linediv, lmod;
unsigned long MaxiFreq, fmaxSIhz = CFG_GetParam(CFG_PARAM_SI5351_MAX_FREQ);

    KeyBreaker=0;
    Maxmag=0.0;
    MaxiMag=0.0;
    Average=0.0;
    h=LCD_GetHeight();
    LastX=LCD_GetWidth()+1;
    steps=(upper-lower)/17500.0f;// in 17.5 kHz steps
    if(graph==1){
        if(Scale_Factors[span]==8) {
            linediv=11;
            lmod=40;
        }
        else {
            linediv=9;
            lmod=50;
        }
        k=linediv*lmod;
    }
    else k=450;
    Faktor1=steps*94/k;
    Faktor=(float)((float)k/(upper-lower));
    AggregatedPoints=(steps+225)/k;// number of measures, aggregated to 1 point (rounded)
    if(AggregatedPoints<=1) AggregatedPoints=1;

    if(graph==1){
        fraction=(float)((upper-lower)/Scale_Factors[span])/5000000.f;
        k=470-k;
        for (n = 0; n <= lmod; n++) {//Scale_Factors[span]
            x = k + n * linediv;
            if (n % 5 == 0){
                if (n % 10 == 0){
                    flabel = (float)(lower/1000000.f + n*fraction);// ??
                    if(fraction<0.001)
                        sprintf(f, "%.4f", flabel);
                    else if(flabel>99.99)
                        sprintf(f, "%.2f", flabel);
                    else if(flabel>9.99)
                        sprintf(f, "%.2f", flabel);
                    else
                        sprintf(f, "%.3f", flabel);

                    w = FONT_GetStrPixelWidth(FONT_SDIGITS, f);
                    l= x -8 - w / 2;
                    if(l<0)l=0;

                    if(l+w>460) l=460-w;
                    FONT_Write(FONT_FRAN, TextColor, BackGrColor, l, 210, f);
                    LCD_VLine(LCD_MakePoint(x-1, 205), 5, LCD_COLOR_RED);
                    LCD_VLine(LCD_MakePoint(x+1, 205), 5, LCD_COLOR_RED);
                }
                LCD_VLine(LCD_MakePoint(x, 205), 5, LCD_COLOR_RED);
            }
            else
            {
                LCD_VLine(LCD_MakePoint(x, 205), 5, LCD_COLOR_YELLOW);
            }
        }
        if(upper-lower<50000){
            GEN_SetLOFreq(lower);
            Faktor=(float)(upper-lower)/40000;
                if(Faktor==1){
                    if(TOUCH_IsPressed()) {
                        if (TOUCH_Poll(&pt))  return -1;
                    }
                    DSP_Sample();
                    Calc_fft_audiobuf(0);
                    for (n = 0; n < 225; n++)// 0 Hz .. 20.625 kHz
                    {
                        signal=rfft_mags[n];
                        x=30+n*0.975f;
                        ShowSpectLine(x);
                    }
                    GEN_SetLOFreq(upper);
                    if(TOUCH_IsPressed()) {
                        if (TOUCH_Poll(&pt))  return -1;
                    }
                    DSP_Sample();
                    Calc_fft_audiobuf(0);
                    for (n = 0; n < 225; n++)// 0 Hz .. 20.625 kHz
                    {
                        signal=rfft_mags[n];
                        x=470-n*0.975f;
                        ShowSpectLine(x);
                    }
                }
                else if(Faktor==0.5f){
                    if(TOUCH_IsPressed()) {
                        if (TOUCH_Poll(&pt))  return -1;
                    }
                    DSP_Sample();
                    Calc_fft_audiobuf(0);
                    for (n = 0; n < 225; n++)// 0 Hz .. 20.625 kHz
                    {
                        signal=rfft_mags[n];
                        x=30+2*n*0.975f;
                        ShowSpectLine(x);
                    }
                }

                else{
                    for(k=0;k<1/Faktor;k++){
                        GEN_SetLOFreq(lower+k*97.5*Faktor);
                        if(TOUCH_IsPressed()) {
                            if (TOUCH_Poll(&pt))  return -1;
                        }
                        DSP_Sample();
                        Calc_fft_audiobuf(0);
                        for (n = 0; n < 2*225*Faktor; n++)// 0 Hz .. 20.625 kHz
                        {
                            signal=rfft_mags[n];
                            x=30+((float)n/Faktor+k)*0.975f;
                            ShowSpectLine(x);
                        }
                    }
                }

                for (n = 0; n < 7; n++) {// paint vertical scale
                    yofs = n*27+16;
                    sprintf(s, "%d", (int)dBs[n]);
                    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 5, yofs-9, s);
                    LCD_HLine(LCD_MakePoint(30, yofs), 450, LCD_COLOR_DARKGRAY);
                }
        return 2;
        }
    }
    k=0;
    Maxmag=0;
    Average=0.;
    if(AggregatedPoints>1){
        for(actFreq=upper+18000;actFreq>=lower-18000; ){// 3 ms per step
            sum1=0.;
            for(l=0;l<AggregatedPoints;l++){
                if(TOUCH_IsPressed()) {
                    if (TOUCH_Poll(&pt))  return -1;
                }
                sum1=0.0;//Maxmag/1000;//++++++++++++++++++++++++++++++++++++++++++
                binMax0=CalcMaxBin(actFreq);
                if(Maxmag>sum1)sum1=Maxmag;
                if(Maxmag>MaxiMag){
                    MaxiMag=Maxmag;
                    MaxiBin=binMax0;
                    MaxiFreq=actFreq;
                }
                actFreq-=17500ul;
            }// end inner "for"

            if(TOUCH_IsPressed()) {
                if (TOUCH_Poll(&pt))  return -1;
            }
            Maxmag=0;
            actFreq+=17500ul;
            if(actFreq>fmaxSIhz) sum1*=1.5;
            signal=sum1;
            x=470-(int)((double)(upper-actFreq)*Faktor);
            if(graph==1){
                if((x>470-linediv*lmod)&&(x!=LastX)){
                    ShowSpectLine(x);
                    LastX=x;
                }
            }
            else{
                k++;
                if(k>20){
                    k=0;
                    sprintf(string1,"F: %d kHz     ",  (int)actFreq/1000);
                    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 132, 100, string1);
                }
            }
            actFreq-=17500ul;
            if(x<28) break;
        }// end for{}
    }

    else{
        breaker=0;
        LastX=LCD_GetWidth()+1;
        for(actFreq=upper+18000;actFreq>=lower-18000; ){// 3 ms per step
            if(TOUCH_IsPressed()) {
                if (TOUCH_Poll(&pt))  return -1;
            }
            Maxmag=0;
            binMax0=CalcMaxBin(actFreq);
            MaxMag0=Maxmag;
            Maxmag=0;
            binMax1=CalcMaxBin(actFreq-15000);
            MaxMag1=Maxmag;
            Maxmag=0;
            binMax2=CalcMaxBin(actFreq-16500);
            MaxMag2=Maxmag;
            if(MaxMag0>MaxMag1) signal=MaxMag0;
            else signal=MaxMag1;
            if(MaxMag2>signal) signal=MaxMag2;
            if(MaxMag0<signal*0.6) binMax0=0;
            if(MaxMag1<signal*0.6) binMax1=0;
            if(MaxMag2<signal*0.6) binMax2=0;
            delta=Delta(binMax0,binMax1,binMax2);
            if(breaker==9) signal=Average;
            breaker=0;
            if(signal>MaxiMag){
                MaxiMag=signal;
                MaxiBin=-delta;
                MaxiFreq=actFreq;
            }
            x=470-(int)((double)(upper-actFreq-delta*93.75f)*Faktor+0.5f);
            if(graph==1){
                if(actFreq>fmaxSIhz) signal*=1.5;
                if(x<LastX){
                    for(n=x;n<LastX;n++){
                        if((n<470)&&(n>29)) {
                            LCD_VLine(LCD_MakePoint(n,2), 201, BackGrColor);//delete old content
                        }
                        else break;
                    }
                }
                LastX=x;
                ShowSpectLine(x);
            }
            else{
                k++;
                if(k>20){
                    k=0;
                    sprintf(string1,"F: %d kHz     ",  (int)actFreq/1000);
                    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 132, 100, string1);
                }
            }
            actFreq-=52500;
        }// end "for"

    }
    if(graph==1){
        for (n = 0; n < 7; n++) {// paint vertical scale
            yofs = n*27+16;
            sprintf(s, "%d", (int)dBs[n]);
            FONT_Write(FONT_FRAN, TextColor, BackGrColor, 5, yofs-9, s);
            LCD_HLine(LCD_MakePoint(30, yofs), 450, LCD_COLOR_DARKGRAY);
        }
    }
    MaxMag0=MaxiMag;

    binx=CalcMaxBin(MaxiFreq-16000);//5000
    if(binx!=0){
        if(Maxmag>MaxiMag){
            MaxiMag=Maxmag;
            MaxiFreq=MaxiFreq-16000;
            MaxiBin=binx;
        }
    }
    binx=CalcMaxBin(MaxiFreq+14000);//2500
   if(binx!=0){
        if(Maxmag>MaxiMag){
            MaxiMag=Maxmag;
            MaxiFreq=MaxiFreq+14000;
            MaxiBin=binx;
        }
    }
    binx=CalcMaxBin(MaxiFreq+8000);//1000
    if(binx!=0){
        if(Maxmag>MaxiMag){
            MaxiMag=Maxmag;
            MaxiFreq=MaxiFreq+8000;
            MaxiBin=binx;
        }
    }
    if(MaxiFreq>fmaxSIhz) signal=1.5 * MaxiMag;
    else signal=MaxiMag;
    fx=MaxiFreq;
    binx=MaxiBin;
    if(graph==1){
        x=470-(int)((double)(upper-MaxiFreq+binx*93.75f)*Faktor+0.5f);
        testFreq=(float)(fx-(int)(float)(binx*93.75f+0.05))/1000;
        if(testFreq<100000)
            sprintf(string1,"%.1f kHz ", (float)testFreq);
        else sprintf(string1,"%.2f MHz ", (float)testFreq/1000);
        if(x<235) k=235;
        else k=45;
        if(signal>164)// trigger value
            FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, k, 10, string1);
        ShowSpectLine(x);// correction
    }
    Maxmag=MaxiMag;
    found=1;
    return 2;
}


void Spectrum(void){
int band;

    while(TOUCH_IsPressed());
    Sleep(100);
    SetUpperLower();
    sScan_Meas=1;
    LCD_Rectangle((LCDPoint){352,234},(LCDPoint){478,270}, 0xffff0000);//red
    LCD_Rectangle((LCDPoint){351,233},(LCDPoint){477,269}, 0xffff0000);//red
    selector=4;
    LCD_FillRect(LCD_MakePoint(0 , 0), LCD_MakePoint(479 , 232), BackGrColor);
    breaker=0;
    if(auto1==0)
        SpectrumExec(1);
    else {
        for(;;){
            cycl3++;
            Sleep(20);
            if(cycl3%10==0)
                if(TOUCH_IsPressed()) break;
            if(cycl3>=50){// 20 * 50 ms = 1s
                cycl3=0;
                SpectrumExec(1);
            }
        }
    }
    auto1=0;
 //   sScan_Meas=0;
    LCD_Rectangle((LCDPoint){352,234},(LCDPoint){478,270}, 0xffffffff);//white
    LCD_Rectangle((LCDPoint){351,233},(LCDPoint){477,269}, 0xffffffff);
}

void Auto(void){
    while(TOUCH_IsPressed());
    Sleep(100);
    if(auto1==0){
        auto1=1;
        LCD_Rectangle((LCDPoint){80,234},(LCDPoint){150,270}, 0xffff0000);// red (active)
        LCD_Rectangle((LCDPoint){81,235},(LCDPoint){149,269}, 0xffff0000);// red (active)
    }
    else {
        auto1=0;
        LCD_Rectangle((LCDPoint){80,234},(LCDPoint){150,270}, 0xffffffff);//white
        LCD_Rectangle((LCDPoint){81,235},(LCDPoint){149,269}, 0xffffffff);//white
    }
}

static const TEXTBOX_t tb_Scan[] = {
    (TEXTBOX_t){ .x0 = 2, .y0 = 60, .text = "Full Scale", .font = FONT_FRANBIG, .width = 120, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))Full, .cbparam = 1, .next = (void*)&tb_Scan[1] },
    (TEXTBOX_t){ .x0 = 2, .y0 = 110, .text = "Ham Bands", .font = FONT_FRANBIG, .width = 120, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))HamBands, .cbparam = 1,  .next = (void*)&tb_Scan[2] },
    (TEXTBOX_t){ .x0 = 2, .y0 = 234, .text = "Exit", .font = FONT_FRANBIG, .width = 70, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_RED, .cb = (void(*)(void))SCExit, .cbparam = 1, .next =(void*)&tb_Scan[3] },
    (TEXTBOX_t){ .x0 = 80, .y0 = 234, .text = "Auto", .font = FONT_FRANBIG, .width = 70, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))Auto, .cbparam = 1, .next =(void*)&tb_Scan[4]},
    (TEXTBOX_t){ .x0 = 160, .y0 = 234, .text = "Frequency", .font = FONT_FRANBIG, .width = 120, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))freq, .cbparam = 1, .next =(void*)&tb_Scan[5]},
    (TEXTBOX_t){ .x0 = 2, .y0 = 160, .text = "One Band", .font = FONT_FRANBIG, .width = 120, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))OneBand, .cbparam = 1, .next =(void*)&tb_Scan[6]},
    (TEXTBOX_t){ .x0 = 290, .y0 = 234, .text = "Test", .font = FONT_FRANBIG, .width = 58, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))Test, .cbparam = 1, .next =(void*)&tb_Scan[7]},
    (TEXTBOX_t){ .x0 = 352, .y0 = 234, .text = "Spectrum", .font = FONT_FRANBIG, .width = 126, .height = FieldH, .center = 1,
                 .border = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = (void(*)(void))Spectrum, .cbparam = 1,},

};



void SPECTR_FindFreq(void){
TEXTBOX_CTX_t fctx;
int cycl1;

    AreaSelected=1;
    upper=3900000ul;
    lower=3500000ul;
    span=7;
    selector=0;
    if(CFG_GetParam(CFG_PARAM_PAN_CENTER_F)==0)
        f1=3500;
    else  f1=3700;
    GEN_Init();// fOsc, fLO off
    SetColours();
Repaint:
    GEN_SetF0Freq(0);// switch to off
    sScan_Meas=0;
    sRepaint=0;
    test=0;
    cycl1=0;
    counter=0;
    auto1=0;
    sExit=0;
    FreqFound=0;
    LCD_FillAll(BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 120, 20, "Find frequency scan mode");
    Sleep(600);

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
        if(sExit==1) {
            while(TOUCH_IsPressed());
            return;
        }
        if(sRepaint==1) {
            while(TOUCH_IsPressed());
            goto Repaint;
        }

        if(auto1==1){
            if (selector==1) {
                cycl1++;
                if(cycl1>=20){// 20 * 50 ms = 1s
                    cycl1=0;
                   ;
                    if( OneBandExec()==-1)
                        continue;

                }
            }
        }
        FreqFound=0;
        Sleep(50);
    }
}
