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
#include "config.h"
#include "LCD.h"
#include "font.h"
#include "touch.h"
#include "hit.h"
#include "dsp.h"
#include "gen.h"
#include "stm32f746xx.h"
#include "oslfile.h"

extern void Sleep(uint32_t ms);

//==============================================================================
static float vswr500[21];
static uint32_t rqExit = 0;
static uint32_t fChanged = 0;

#define SCAN_ORIGIN_X 20
#define SCAN_ORIGIN_Y 209

static void ShowF()
{
    char str[50];
    sprintf(str, "F: %u kHz        ", (unsigned int)(CFG_GetParam(CFG_PARAM_MEAS_F) / 1000));
    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 2, str);
}

static void DrawSmallSmith(int X0, int Y0, int R, float complex G)
{
    LCDColor sc = LCD_RGB(96, 96, 96);
    LCD_FillCircle(LCD_MakePoint(X0, Y0), R + 2, LCD_BLACK);
    LCD_Circle(LCD_MakePoint(X0, Y0), R, sc);
    LCD_Circle(LCD_MakePoint(X0 - R / 2 , Y0), R / 2, sc);
    LCD_Circle(LCD_MakePoint(X0 + R / 2 , Y0), R / 2, sc);
    LCD_Line(LCD_MakePoint(X0 - R, Y0), LCD_MakePoint(X0 + R, Y0), sc);

    int x = (int)(crealf(G) * R) + X0;
    int y = Y0 - (int)(cimagf(G) * R);
    LCD_Line(LCD_MakePoint(x - 1, y), LCD_MakePoint(x + 1, y), LCD_GREEN);
    LCD_Line(LCD_MakePoint(x, y - 1), LCD_MakePoint(x, y + 1), LCD_GREEN);
}

//Scan VSWR in +/- 500 kHz range around measurement frequency with 100 kHz step, to draw a small graph below the measurement
static int Scan500(void)
{
    static int i = 0;
    int fq = (int)CFG_GetParam(CFG_PARAM_MEAS_F) + (i - 10) * 50000;
    if (fq > 0)
    {
        GEN_SetMeasurementFreq(fq);
        Sleep(10);
        DSP_Measure(fq, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        vswr500[i] = DSP_CalcVSWR(DSP_MeasuredZ());
    }
    else
    {
        vswr500[i] = 9999.0;
    }
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y), LCD_MakePoint(SCAN_ORIGIN_X + i * 10, SCAN_ORIGIN_Y), LCD_GREEN);
    i++;
    if (i == 21)
    {
        i = 0;
        LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X - 1, SCAN_ORIGIN_Y), LCD_MakePoint(SCAN_ORIGIN_X + 201, SCAN_ORIGIN_Y), LCD_BLUE);
        return 1;
    }
    return 0;
}

//Display measured data
static void MeasurementModeDraw(DSP_RX rx)
{
    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS)); //Wait for LCD output cycle finished to avoid flickering

    char str[50] = "";
    sprintf(str, "Magnitude diff %.2f dB     ", DSP_MeasuredDiffdB());
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLACK, 0, 38, str);

    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 62);
    sprintf(str, "R: %.1f  X: %.1f", crealf(rx), cimagf(rx));
    FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 0, 62, str);

    float VSWR = DSP_CalcVSWR(rx);
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 92);
    sprintf(str, "VSWR: %.1f (Z0 %d)", VSWR, CFG_GetParam(CFG_PARAM_R0));
    FONT_Write(FONT_FRANBIG, LCD_CYAN, LCD_BLACK, 0, 92, str);

    float XX = cimagf(rx);
    //calculate equivalent capacity and inductance (if XX is big enough)
    if(XX > 3.0 && XX < 1000.)
    {
        float Luh = 1e6 * fabs(XX) / (2.0 * 3.1415926 * GEN_GetLastFreq());
        sprintf(str, "%.2f uH           ", Luh);
        FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 0, 122, str);
    }
    else if (XX < -3.0 && XX > -1000.)
    {
        float Cpf = 1e12 / (2.0 * 3.1415926 * GEN_GetLastFreq() * fabs(XX));
        sprintf(str, "%.0f pF              ", Cpf);
        FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 0, 122, str);
    }
    else
    {
        FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 122);
    }

    //Calculated matched cable loss at this frequency
    FONT_ClearLine(FONT_FRAN, LCD_BLACK, 158);
    float ga = cabsf(OSL_GFromZ(rx, CFG_GetParam(CFG_PARAM_R0))); //G amplitude
    if (ga > 0.01)
    {
        float cl = -10. * log10f(ga);
        if (cl < 0.001f)
            cl = 0.f;
        sprintf(str, "MCL: %.2f dB", cl);
        FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 0, 158, str);
    }

    DrawSmallSmith(380, 180, 80, OSL_GFromZ(rx, CFG_GetParam(CFG_PARAM_R0)));
}

//Draw a small (100x20 pixels) VSWR graph for data collected by Scan500()
static void MeasurementModeGraph(DSP_RX in)
{
    int idx = 0;
    float prev = 3.0;
    LCD_FillRect(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y+1), LCD_MakePoint(SCAN_ORIGIN_X + 200, SCAN_ORIGIN_Y + 21), LCD_RGB(0, 0, 48)); // Graph rectangle
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X+100, SCAN_ORIGIN_Y+1), LCD_MakePoint(SCAN_ORIGIN_X+100, SCAN_ORIGIN_Y+21), LCD_RGB(48, 48, 48));  // Measurement frequency line
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X+50, SCAN_ORIGIN_Y+1), LCD_MakePoint(SCAN_ORIGIN_X+50, SCAN_ORIGIN_Y+21), LCD_RGB(48, 48, 48));
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X+150, SCAN_ORIGIN_Y+1), LCD_MakePoint(SCAN_ORIGIN_X+150, SCAN_ORIGIN_Y+21), LCD_RGB(48, 48, 48));
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y + 11), LCD_MakePoint(SCAN_ORIGIN_X+200, SCAN_ORIGIN_Y + 11), LCD_RGB(48, 48, 48));   // VSWR 2.0 line
    for (idx = 0; idx <= 20; idx++)
    {
        float vswr;
        if (idx == 10)
            vswr = DSP_CalcVSWR(in); //VSWR at measurement frequency
        else
            vswr = vswr500[idx];

        if (vswr > 3.0 || isnan(vswr) || isinf(vswr) || vswr < 1.0) //Graph limit is VSWR 3.0
            vswr = 3.0;                                             //Including uninitialized values

        if (idx > 0)
        {
            LCD_Line( LCD_MakePoint(SCAN_ORIGIN_X + (idx - 1) * 10, SCAN_ORIGIN_Y + 21 - ((int)(prev * 10) - 10)),
                      LCD_MakePoint(SCAN_ORIGIN_X + idx * 10, SCAN_ORIGIN_Y + 21 - ((int)(vswr * 10) - 10)),
                      LCD_YELLOW);
        }
        prev = vswr;
    }
}

static void MEASUREMENT_SwitchWindowing(void)
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
    if(MeasurementFreq < BAND_FMAX)
    {
        if ((MeasurementFreq + step) > BAND_FMAX)
            MeasurementFreq = BAND_FMAX;
        else
            MeasurementFreq = MeasurementFreq + step;
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged = 1;
    }
}

static void MEASUREMENT_FDecr_500k(void)
{
    FDecr(500000);
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
    FIncr(500000);
}

static const struct HitRect hitArr[] =
{
    //        x0,  y0, width, height, callback
    HITRECT(   0, 200,   100,     79, MEASUREMENT_SwitchWindowing),
    HITRECT(   0,   0,  80, 150, MEASUREMENT_FDecr_500k),
    HITRECT(  80,   0,  80, 150, MEASUREMENT_FDecr_100k),
    HITRECT( 160,   0,  70, 150, MEASUREMENT_FDecr_10k),
    HITRECT( 250,   0,  70, 150, MEASUREMENT_FIncr_10k),
    HITRECT( 320,   0,  80, 150, MEASUREMENT_FIncr_100k),
    HITRECT( 400,   0,  80, 150, MEASUREMENT_FIncr_500k),
    HITEND
};

//Measurement mode window. To change it to VSWR tap the lower part of display.
//To change frequency, in steps of +/- 500, 100 and 10 kHz, tap top part of the display,
//the step depends on how far you tap from the center.

void MEASUREMENT_Proc(void)
{
    //Load saved middle frequency value from BKUP registers 2, 3
    //to MeasurementFreq
    uint32_t fbkup = CFG_GetParam(CFG_PARAM_MEAS_F);
    if (!(fbkup >= BAND_FMIN && fbkup <= BAND_FMAX && (fbkup % 10000) == 0))
    {
        CFG_SetParam(CFG_PARAM_MEAS_F, 14000000ul);
        CFG_Flush();
    }

    rqExit = 0;
    fChanged = 0;

    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 120, 60, "Measurement mode");
    if (-1 == OSL_GetSelected())
    {
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 80, 120, "No calibration file selected!");
        Sleep(200);
    }
    Sleep(300);
    while(TOUCH_IsPressed());

    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRAN, LCD_BLUE, LCD_BLACK, SCAN_ORIGIN_X - 20, SCAN_ORIGIN_Y - 25, "VSWR (1.0 ... 3.0), F +/- 500 KHz, step 50:");
    LCD_FillRect(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y+1), LCD_MakePoint(SCAN_ORIGIN_X + 200, SCAN_ORIGIN_Y + 21), LCD_RGB(0, 0, 48)); // Graph rectangle
    LCD_Rectangle(LCD_MakePoint(SCAN_ORIGIN_X - 1, SCAN_ORIGIN_Y), LCD_MakePoint(SCAN_ORIGIN_X + 201, SCAN_ORIGIN_Y + 22), LCD_BLUE);

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
    ShowF();
    DSP_RX rx;

    if (-1 == OSL_GetSelected())
    {
        FONT_Write(FONT_FRAN, LCD_RED, LCD_BLACK, 380, 18, "No OSL");
    }
    else
    {
        OSL_CorrectZ(BAND_FMIN, 0+0*I); //To force lazy loading OSL file if it has not been loaded yet
        if (OSL_IsSelectedValid())
        {
            FONT_Print(FONT_FRAN, LCD_GREEN, LCD_BLACK, 380, 18, "OSL: %s, OK", OSL_GetSelectedName());
        }
        else
        {
            FONT_Print(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 380, 18, "OSL: %s, BAD", OSL_GetSelectedName());
        }
    }

    if (OSL_IsErrCorrLoaded())
    {
        FONT_Write(FONT_FRAN, LCD_GREEN, LCD_BLACK, 380, 36, "HW cal: OK");
    }
    else
    {
        FONT_Write(FONT_FRAN, LCD_RED, LCD_BLACK, 380, 36, "HW cal: NO");
    }

    for(;;)
    {
        int scanres = Scan500();
        GEN_SetMeasurementFreq(CFG_GetParam(CFG_PARAM_MEAS_F));
        Sleep(10);
        DSP_Measure(CFG_GetParam(CFG_PARAM_MEAS_F), 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));

        rx = DSP_MeasuredZ();
        MeasurementModeDraw(rx);
        if (scanres)
            MeasurementModeGraph(rx);

        if (DSP_MeasuredMagVmv() < 20.)
        {
            FONT_Write(FONT_FRAN, LCD_BLACK, LCD_RED, 380, 2, "No signal  ");
        }
        else
        {
            FONT_Write(FONT_FRAN, LCD_GREEN, LCD_BLACK, 380, 2, "Signal OK");
        }

        SCB_CleanDCache(); //Flush D-Cache contents to the RAM to avoid cache coherency

        LCDPoint pt;
        while (TOUCH_Poll(&pt))
        {
            HitTest(hitArr, pt.x, pt.y);
            if (rqExit)
            {
                GEN_SetMeasurementFreq(0);
                while(TOUCH_IsPressed());
                return;
            }
            if (fChanged)
            {
                ShowF();
                SCB_CleanDCache(); //Flush D-Cache contents to the RAM to avoid cache coherency
            }
            Sleep(50);
        }
        if (fChanged)
        {
            CFG_Flush();
            fChanged = 0;
        }
        Sleep(50);
    }
}
