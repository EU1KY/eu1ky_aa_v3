/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include "config.h"
#include "LCD.h"
#include "font.h"
#include "touch.h"
#include "hit.h"
#include "dsp.h"
#include "gen.h"
#include "stm32f746xx.h"
#include "oslfile.h"
#include "stm32746g_discovery_lcd.h"
#include "match.h"
#include "num_keypad.h"
#include "screenshot.h"
#include "smith.h"
#include "measurement.h"
#include "panfreq.h"
extern void Sleep(uint32_t ms);

//==============================================================================
static uint32_t rqExit = 0;
static uint32_t redrawWindow = 0;
static uint32_t fChanged = 0;
static uint32_t isMatch = 0;
static uint32_t meas_maxstep = 500000;
static float vswr500[100];
static float complex zFine500[100] = { 0 };
static uint8_t DrawFine;

#define SCAN_ORIGIN_X 20
#define SCAN_ORIGIN_Y 209

uint8_t seconds;// W.Kiefer

static void ShowF()
{
    char str[50];
    uint8_t i,j;
    unsigned int freq=(unsigned int)(CFG_GetParam(CFG_PARAM_MEAS_F) / 1000);
    sprintf(str, "F: %u kHz        ", freq);
    if(freq>999){// WK
        for(i=3;i<10;i++){
            if(str[i]==' ') break;// search space before "kHz"
        }
        for(j=i+3;j>i-4;j--){
           str[j+1]=str[j];
        }
        str[i-3]='.';
        str[i+5]=0;
    }
    LCD_FillRect(LCD_MakePoint(80, 38), LCD_MakePoint(214, 72), BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 0, 36, str);// WK
}

void DrawSmallSmith(int X0, int Y0, int R, float complex rx)
{
    LCD_FillRect(LCD_MakePoint(X0 - R - 20, Y0 - R - 2), LCD_MakePoint(LCD_GetWidth()-1, LCD_GetHeight()-1), BackGrColor);
    if (isMatch)
    { //Draw LC match
        X0 = X0 - R - 20;
        Y0 -= R;
        MATCH_S matches[4];

        uint32_t nMatches = MATCH_Calc(rx, matches);
        if (0 == nMatches)
        {
            FONT_Write(FONT_FRAN, LCD_WHITE, BackGrColor, X0, Y0, "No LC match for this load");
        }
        else
        {
            uint32_t i;
            uint32_t fHz = CFG_GetParam(CFG_PARAM_MEAS_F);
            FONT_Print(FONT_FRAN, TextColor, BackGrColor, X0, Y0, "LC match for SRC Z0 = %d", CFG_GetParam(CFG_PARAM_R0));
            Y0 += FONT_GetHeight(FONT_FRAN) + 4;
            FONT_Write(FONT_FRAN, TextColor, BackGrColor, X0, Y0, "SRCpar   Ser   LoadPar");
            Y0 += FONT_GetHeight(FONT_FRAN) + 4;
            for (i = 0; i < nMatches; i++)
            {
                char strxpl[32];
                char strxps[32];
                char strxs[32];
                MATCH_XtoStr(fHz, matches[i].XPL, strxpl);
                MATCH_XtoStr(fHz, matches[i].XPS, strxps);
                MATCH_XtoStr(fHz, matches[i].XS, strxs);
                FONT_Print(FONT_FRAN, TextColor, BackGrColor, X0, Y0, "%s, %s, %s", strxps, strxs, strxpl);
                Y0 += FONT_GetHeight(FONT_FRAN) + 4;
            }
        /*    char strfreq[32];

            if(TimeFlag>=999) {//test, if timer interrupt is working
                TimeFlag=0;
                seconds++;
            }
            sprintf(strfreq, "F: %u kHz %d", (unsigned int)(Timer5Value),seconds);
            FONT_Write(FONT_FRAN, LCD_RED, LCD_BLACK, X0, Y0, strfreq);*/
        }
        return;
    }
    float complex G = OSL_GFromZ(rx, CFG_GetParam(CFG_PARAM_R0));
    LCDColor sc = LCD_RGB(96, 96, 96);
    SMITH_DrawGrid(X0, Y0, R, Color2, BackGrColor, SMITH_R50 | SMITH_Y50 | SMITH_R25 | SMITH_R10 | SMITH_R100 | SMITH_R200 |
                                     SMITH_J50 | SMITH_J100 | SMITH_J200 | SMITH_J25 | SMITH_J10);
    SMITH_DrawLabels(TextColor, BackGrColor, SMITH_R50 | SMITH_R25 | SMITH_R10 | SMITH_R100 | SMITH_R200 |
                     SMITH_J50 | SMITH_J100 | SMITH_J200 | SMITH_J25 | SMITH_J10);

    //Draw mini-scan points
uint32_t i;
int x, y;
float complex Gx;
    //SMITH_ResetStartPoint();// WK

   for (i = 0; i < 100; i++)
    {
        if(crealf(zFine500[i])!=9999.){
            Gx = OSL_GFromZ(zFine500[i], CFG_GetParam(CFG_PARAM_R0));
            SMITH_DrawG(i, Gx, Color1);
        }
        else SMITH_DrawG(800, Gx, Color1);// trick, to write lastoffsets
    }

    x = (int)(crealf(G) * R) + X0;
    y = Y0 - (int)(cimagf(G) * R);
    LCD_FillRect(LCD_MakePoint(x - 3, y-3), LCD_MakePoint(x + 3, y+3), CurvColor);// Set Cursor
}
void InitScan500(void){
int i;
    for(i=0;i<100;i++){
        zFine500[i]=9999.f+0.fi;
        vswr500[i]=9999.0;
    }
}

//Scan VSWR in +/- 500 kHz range around measurement frequency with 100 kHz step, to draw a small graph below the measurement
void Scan500(int i, int k)
{
DSP_RX  RX;
    int fq = (int)CFG_GetParam(CFG_PARAM_MEAS_F) + (5*i+k - 50) * 10000;
    if (fq > 0)
    {
        GEN_SetMeasurementFreq(fq);
        Sleep(2);// was 10
        DSP_Measure(fq, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        RX=zFine500[5*i+k] = DSP_MeasuredZ();
        vswr500[5*i+k] = DSP_CalcVSWR(RX);
    }
    else
    {
        vswr500[5*i+k] = 9999.0;
    }
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y+20), LCD_MakePoint(SCAN_ORIGIN_X + i * 10, SCAN_ORIGIN_Y+20), LCD_GREEN);
}

//Display measured data
static void MeasurementModeDraw(DSP_RX rx)
{
float r= crealf(rx);
float im= cimagf(rx);
float VSWR = DSP_CalcVSWR(rx);
char str[50] = "";
    sprintf(str, "Magnitude diff %.2f dB     ", DSP_MeasuredDiffdB());
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 215, 38, str);// WK

    FONT_ClearHalfLine(FONT_FRANBIG,BackGrColor, 62);//WK
    if (fabsf(r) >= 999.5f)
        sprintf(str, "R: %.0fk ", r / 1000.f);
    else if (fabsf(r) >= 199.5f)
        sprintf(str, "R: %.0f ", r);
    else
        sprintf(str, "R: %.1f ", r);
    if (fabsf(im) > 999.5f)
        sprintf(&str[strlen(str)], "X: %.0fk", im / 1000.f);
    else if (fabsf(im) >= 199.5f)
        sprintf(&str[strlen(str)], "X: %.0f", im);
    else
        sprintf(&str[strlen(str)], "X: %.1f", im);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 0, 62, str);
    FONT_ClearHalfLine(FONT_FRANBIG, BackGrColor, 92);
    if(VSWR>100)
        sprintf(str, "VSWR: %.0f (Z0 %d)", VSWR, CFG_GetParam(CFG_PARAM_R0));
    else
    sprintf(str, "VSWR: %.1f (Z0 %d)", VSWR, CFG_GetParam(CFG_PARAM_R0));
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 0, 92, str);

    float XX = cimagf(rx);
    //calculate equivalent capacity and inductance (if XX is big enough)
    if(XX > 3.0 && XX < 1000.)
    {
        float Luh = 1e6 * fabs(XX) / (2.0 * 3.1415926 * GEN_GetLastFreq());
        sprintf(str, "%.2f uH           ", Luh);
        FONT_Write(FONT_FRANBIG, Color1, BackGrColor, 0, 122, str);
    }
    else if (XX < -3.0 && XX > -1000.)
    {
        float Cpf = 1e12 / (2.0 * 3.1415926 * GEN_GetLastFreq() * fabs(XX));
        sprintf(str, "%.0f pF              ", Cpf);
        FONT_Write(FONT_FRANBIG, Color1, BackGrColor, 0, 122, str);
    }
    else
    {
        FONT_ClearHalfLine(FONT_FRANBIG, BackGrColor, 122);
    }

    //Calculated matched cable loss at this frequency
    FONT_ClearHalfLine(FONT_FRAN, BackGrColor, 158);
    float ga = cabsf(OSL_GFromZ(rx, CFG_GetParam(CFG_PARAM_R0))); //G amplitude
    if (ga > 0.01)
    {
        float cl = -10. * log10f(ga);
        if (cl < 0.001f)
            cl = 0.f;
        sprintf(str, "MCL: %.2f dB  |Z|: %.1f", cl, cabsf(rx));
        FONT_Write(FONT_FRAN, Color1, BackGrColor, 0, 158, str);
    }
}

//Draw a small (100x30 pixels) VSWR graph for data collected by Scan500()
static void MeasurementModeGraph(DSP_RX in)
{
    LCDPoint p1,p2;
    int idx = 0;
    float vswr;

    LCD_FillRect(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y-9), LCD_MakePoint(SCAN_ORIGIN_X + 200, SCAN_ORIGIN_Y + 21), BackGrColor); // Graph rectangle
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X+100, SCAN_ORIGIN_Y+1), LCD_MakePoint(SCAN_ORIGIN_X+100, SCAN_ORIGIN_Y+11), TextColor);  // Measurement frequency line
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X+50, SCAN_ORIGIN_Y+1), LCD_MakePoint(SCAN_ORIGIN_X+50, SCAN_ORIGIN_Y+11), TextColor);
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X+150, SCAN_ORIGIN_Y+1), LCD_MakePoint(SCAN_ORIGIN_X+150, SCAN_ORIGIN_Y+11), TextColor);
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y + 17), LCD_MakePoint(SCAN_ORIGIN_X+200, SCAN_ORIGIN_Y + 17), TextColor);   // VSWR 2.0 line
    for (idx = 0; idx < 100; idx++)
    {
        vswr = vswr500[idx];
        if(vswr!=9999.0){
            if (vswr > 12.0 || isnan(vswr) || isinf(vswr) || vswr < 1.0) //Graph limit is VSWR 3.0
                vswr = 12.0;                                             //Including uninitialized values
            if(idx==0)
                p1=LCD_MakePoint(SCAN_ORIGIN_X + (idx - 1) *2, SCAN_ORIGIN_Y + 11 - (int)((vswr * 30-140)/11));
            else
            {
                p2=LCD_MakePoint(SCAN_ORIGIN_X + idx * 2, SCAN_ORIGIN_Y + 11 - (int)((vswr * 30-140)/11));
                LCD_Line( p1,p2,CurvColor);
                LCD_Line((LCDPoint){p1.x,p1.y+1},(LCDPoint){p2.x,p2.y+1},CurvColor);
                p1=p2;
            }
        }
    }
}

static void MEASUREMENT_Exit(void)
{
    rqExit = 1;
}

static void FDecr(uint32_t step)
{
    uint32_t MeasurementFreq = CFG_GetParam(CFG_PARAM_MEAS_F);
    if(MeasurementFreq > step && MeasurementFreq % step != 0)
    {
        MeasurementFreq -= (MeasurementFreq % step);
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged = 1;
    }
    if(MeasurementFreq < BAND_FMIN)
    {
        MeasurementFreq = BAND_FMIN;
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged = 1;
    }
    if(MeasurementFreq > BAND_FMIN)
    {
        if(MeasurementFreq > step && (MeasurementFreq - step) >= BAND_FMIN)
        {
            MeasurementFreq = MeasurementFreq - step;
            CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
            fChanged = 1;
        }
    }
    Sleep(500);
}

static void FIncr(uint32_t step)
{
    uint32_t MeasurementFreq = CFG_GetParam(CFG_PARAM_MEAS_F);
    if(MeasurementFreq > step && MeasurementFreq % step != 0)
    {
        MeasurementFreq -= (MeasurementFreq % step);
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged = 1;
    }
    if(MeasurementFreq < BAND_FMIN)
    {
        MeasurementFreq = BAND_FMIN;
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged = 1;
    }
    if(MeasurementFreq < CFG_GetParam(CFG_PARAM_BAND_FMAX))
    {
        if ((MeasurementFreq + step) > CFG_GetParam(CFG_PARAM_BAND_FMAX))
            MeasurementFreq = CFG_GetParam(CFG_PARAM_BAND_FMAX);
        else
            MeasurementFreq = MeasurementFreq + step;
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged = 1;
    }
    Sleep(500);
}

static void MEASUREMENT_FDecr_500k(void)
{
    FDecr(meas_maxstep);
}
static void MEASUREMENT_FDecr_100k(void)
{
    FDecr(100000);
}
static void MEASUREMENT_FDecr_10k(void)
{
    FDecr(10000);
}
static void MEASUREMENT_FIncr_10k(void)
{
    FIncr(10000);
}
static void MEASUREMENT_FIncr_100k(void)
{
    FIncr(100000);
}
static void MEASUREMENT_FIncr_500k(void)
{
    FIncr(meas_maxstep);
}
static void MEASUREMENT_SmithMatch(void)
{
    isMatch = !isMatch;
    redrawWindow=1;
    while(TOUCH_IsPressed());// WK
    Sleep(10);
}

static uint32_t fx = 14000000ul; //Scan range start frequency, in Hz
static uint32_t fxkHz;//Scan range start frequency, in kHz
static BANDSPAN pBs1;

static void MEASUREMENT_SetFreq(void)
{
//    while(TOUCH_IsPressed()); WK
    fx=CFG_GetParam(CFG_PARAM_MEAS_F);
    fxkHz=fx/1000;
    if (PanFreqWindow(&fxkHz, &pBs1))
        {
            //Span or frequency has been changed
            CFG_SetParam(CFG_PARAM_MEAS_F, fxkHz*1000);
        }
   // uint32_t val = NumKeypad(CFG_GetParam(CFG_PARAM_MEAS_F)/1000, BAND_FMIN/1000, CFG_GetParam(CFG_PARAM_BAND_FMAX)/1000, "Set measurement frequency, kHz");
    CFG_Flush();
    fChanged=1;
    Sleep(200);
}

static void MEASUREMENT_Screenshot(void)
{
    char* fname = 0;
    fname = SCREENSHOT_SelectFileName();
    SCREENSHOT_DeleteOldest();
    if (CFG_GetParam(CFG_PARAM_SCREENSHOT_FORMAT))
        SCREENSHOT_SavePNG(fname);
    else
        SCREENSHOT_Save(fname);
}

static const struct HitRect hitArr[] =// Set frequency...
{
    //        x0,  y0, width, height, callback
    HITRECT(   0, 236,  80,  35, MEASUREMENT_Exit),
    HITRECT(  90, 236,  82,  35, MEASUREMENT_SetFreq),
    HITRECT( 180, 236,  82,  35, MEASUREMENT_Screenshot),
    HITRECT(   0,   0,  82, 37, MEASUREMENT_FDecr_500k),
    HITRECT(  82,   0,  82, 37, MEASUREMENT_FDecr_100k),
    HITRECT( 164,   0,  72, 37, MEASUREMENT_FDecr_10k),
    HITRECT( 244,   0,  72, 37, MEASUREMENT_FIncr_10k),
    HITRECT( 316,   0,  82, 37, MEASUREMENT_FIncr_100k),
    HITRECT( 398,   0,  82, 37, MEASUREMENT_FIncr_500k),
    HITRECT( 278, 90, 201, 181, MEASUREMENT_SmithMatch),
    HITRECT( 278, 58, 100, 34, MEASUREMENT_SmithMatch),
    HITEND
};

void ShowIncDec(void){
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,5, 2, "-0.5M");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,87, 2, "-0.1M");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,169, 2, "-10k");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,249, 2, "+10k");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,320, 2, "+0.1M");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor,402, 2, "+0.5M");
}

//Measurement mode window. To change it to VSWR tap the lower part of display.
//To change frequency, in steps of +/- 500, 100 and 10 kHz, tap top part of the display,
//the step depends on how far you tap from the center.

void MEASUREMENT_Proc(void)
{
 float delta;
 uint32_t fbkup;
 uint32_t activeLayer;
 int i,j=0,k;
 DSP_RX rx, rx0;
 int Cycles=0;
    DrawFine=0;
    //Load saved middle frequency value from BKUP registers 2, 3
    //to MeasurementFreq
    while(TOUCH_IsPressed());
    fbkup = CFG_GetParam(CFG_PARAM_MEAS_F);
    if (!(fbkup >= BAND_FMIN &&
          fbkup <= CFG_GetParam(CFG_PARAM_BAND_FMAX) &&
          (fbkup % 1000) == 0))
    {
        CFG_SetParam(CFG_PARAM_MEAS_F, 14000000ul);
        CFG_Flush();
    }
    SetColours();
    rqExit = 0;
    fChanged = 0;
    isMatch = 0;
    redrawWindow = 1;
    i=0;


    FONT_Write(FONT_FRANBIG, LCD_WHITE, BackGrColor, 120, 60, "Measurement mode");
    if (-1 == OSL_GetSelected())
    {
        FONT_Write(FONT_FRANBIG, LCD_RED, BackGrColor, 80, 120, "No calibration file selected!");
        Sleep(200);
    }
    BSP_LCD_SelectLayer(0);
    LCD_FillAll(BackGrColor);
    BSP_LCD_SelectLayer(1);
    LCD_FillAll(BackGrColor);
    LCD_ShowActiveLayerOnly();
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 6, 236, " Exit");
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 92, 250, "Set frequency");
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 182, 250, "Save snapshot ");

    Sleep(1000);
//    while(TOUCH_IsPressed()); WK

    LCD_FillRect(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y-9), LCD_MakePoint(SCAN_ORIGIN_X + 200, SCAN_ORIGIN_Y + 21), BackGrColor); // Graph rectangle
    LCD_Rectangle(LCD_MakePoint(SCAN_ORIGIN_X - 1, SCAN_ORIGIN_Y-10), LCD_MakePoint(SCAN_ORIGIN_X + 201, SCAN_ORIGIN_Y + 22), LCD_BLUE);

    InitScan500();
MEASUREMENT_REDRAW:
     i=k=0;
        LCD_FillRect(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y-9), LCD_MakePoint(SCAN_ORIGIN_X + 200, SCAN_ORIGIN_Y + 21), BackGrColor); // Graph rectangle
        LCD_Rectangle(LCD_MakePoint(SCAN_ORIGIN_X - 1, SCAN_ORIGIN_Y-10), LCD_MakePoint(SCAN_ORIGIN_X + 201, SCAN_ORIGIN_Y + 22), LCD_BLUE);
        FONT_Write(FONT_FRAN, TextColor, BackGrColor, SCAN_ORIGIN_X - 20, SCAN_ORIGIN_Y - 35, "VSWR (1.0 ... 12.0), F +/- 500 KHz, step 50:");
        ShowF();

        if (-1 == OSL_GetSelected())
        {
            FONT_Write(FONT_FRAN, LCD_RED, BackGrColor, 380, 54, "No OSL");// WK
        }
        else
        {
            OSL_CorrectZ(BAND_FMIN, 0+0*I); //To force lazy loading OSL file if it has not been loaded yet
            if (OSL_IsSelectedValid())
            {
                FONT_Print(FONT_FRAN, Color1, BackGrColor, 380, 54, "OSL: %s, OK", OSL_GetSelectedName());// WK
            }
            else
            {
                FONT_Print(FONT_FRAN, LCD_YELLOW, BackGrColor, 380, 54, "OSL: %s, BAD", OSL_GetSelectedName());// WK
            }
        }

        if (OSL_IsErrCorrLoaded())
        {
            FONT_Write(FONT_FRAN, Color1, BackGrColor, 380, 70, "HW cal: OK");// WK
        }
        else
        {
            FONT_Write(FONT_FRAN, LCD_RED, BackGrColor, 380, 70, "HW cal: NO");// WK
        }

        DSP_Measure(CFG_GetParam(CFG_PARAM_MEAS_F), 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        rx = DSP_MeasuredZ();
        MeasurementModeDraw(rx);
        ShowHitRect(hitArr);
        ShowIncDec();

    //Main window cycle
    for(;;)
    {
        Scan500(i++,k);
        if(i>=20) {
            i=0;
            k=(k+2)%5;//0 2 4 1 3 0...
            MeasurementModeDraw(rx);
            if(fabsf(crealf(rx)) < 999.5f){
                BSP_LCD_SelectLayer(!activeLayer);
                DrawSmallSmith(380, 180, 80, rx);
                BSP_LCD_SelectLayer(!activeLayer);// WK
                LCD_ShowActiveLayerOnly();
            }
            MeasurementModeGraph(rx);
            ShowHitRect(hitArr);
        }

        DSP_Measure(CFG_GetParam(CFG_PARAM_MEAS_F), 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        rx0 = DSP_MeasuredZ();
        delta = fabsf(crealf(rx0)-crealf(rx));
        delta+=fabsf(cimagf(rx0)-cimagf(rx));
        delta=delta/crealf(rx0);
        if(delta>0.06){
            redrawWindow = 1;
            InitScan500();
            rx=rx0;
        }

        if (DSP_MeasuredMagVmv() < 1.)
        {
            FONT_Write(FONT_FRAN, LCD_RED, BackGrColor, 380, 38, "No signal  ");// WK
        }
        else
        {
            FONT_Write(FONT_FRAN, Color1, BackGrColor, 380, 38, "Signal OK  ");// WK
        }

        if(!isMatch)
            FONT_Write(FONT_FRAN, TextColor, BackGrColor, 292, 66, "LC match      ");
        else
            FONT_Write(FONT_FRAN, TextColor, BackGrColor, 292, 66, "Smith diagram ");
        uint32_t speedcnt = 0;
        meas_maxstep = 500000;
        LCDPoint pt;
        while (TOUCH_Poll(&pt))
        {
            HitTest(hitArr, pt.x, pt.y);
            if (rqExit)
            {
                GEN_SetMeasurementFreq(0);
               // while(TOUCH_IsPressed());
                return;
            }
            if (redrawWindow)
            {
                redrawWindow = 0;
                goto MEASUREMENT_REDRAW;
            }
            if (fChanged)
            {
                redrawWindow = 1;
                ShowF();
            }
            speedcnt++;
            if (speedcnt < 20)
                Sleep(100);
            else
                Sleep(30);
            if (speedcnt > 50)
                meas_maxstep = 2000000;
        }
        speedcnt = 0;
        meas_maxstep = 500000;

        if (fChanged)
        {
            redrawWindow = 1;
            InitScan500();
            CFG_Flush();
            fChanged = 0;
        }
        if (redrawWindow)
            {
                redrawWindow = 0;
                goto MEASUREMENT_REDRAW;
            }
        Sleep(5);
    }
}
