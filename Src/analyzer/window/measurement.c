/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <stdbool.h>
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

extern void Sleep(uint32_t ms);

//==============================================================================
static float vswr500[21];
static uint32_t rqExit = 0;
static uint32_t redrawWindow = 0;
static uint32_t fChanged = 0;
static uint32_t isMatch = 0;
static uint32_t meas_maxstep = 500000;
static float complex z500[21] = { 0 };
#define SCAN_ORIGIN_X 20
#define SCAN_ORIGIN_Y 209

static void ShowF()
{
    char str[50];
    sprintf(str, "F: %u kHz        ", (unsigned int)(CFG_GetParam(CFG_PARAM_MEAS_F) / 1000));
    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 2, str);
}

static void DrawSmallSmith(int X0, int Y0, int R, float complex rx)
{
    LCD_FillRect(LCD_MakePoint(X0 - R - 20, Y0 - R - 2), LCD_MakePoint(LCD_GetWidth()-1, LCD_GetHeight()-1), LCD_BLACK);
    if (isMatch)
    { //Draw LC match
        X0 = X0 - R - 20;
        Y0 -= R;
        MATCH_S matches[4];
        uint32_t nMatches = MATCH_Calc(rx, matches);
        if (0 == nMatches)
        {
            FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLACK, X0, Y0, "No LC match for this load");
        }
        else
        {
            uint32_t i;
            uint32_t fHz = CFG_GetParam(CFG_PARAM_MEAS_F);
            FONT_Print(FONT_FRAN, LCD_PURPLE, LCD_BLACK, X0, Y0, "LC match for SRC Z0 = %d", CFG_GetParam(CFG_PARAM_R0));
            Y0 += FONT_GetHeight(FONT_FRAN) + 4;
            FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLACK, X0, Y0, "SRCpar   Ser   LoadPar");
            Y0 += FONT_GetHeight(FONT_FRAN) + 4;
            for (i = 0; i < nMatches; i++)
            {
                char strxpl[32];
                char strxps[32];
                char strxs[32];
                MATCH_XtoStr(fHz, matches[i].XPL, strxpl);
                MATCH_XtoStr(fHz, matches[i].XPS, strxps);
                MATCH_XtoStr(fHz, matches[i].XS, strxs);
                FONT_Print(FONT_FRAN, LCD_WHITE, LCD_BLACK, X0, Y0, "%s, %s, %s", strxps, strxs, strxpl);
                Y0 += FONT_GetHeight(FONT_FRAN) + 4;
            }
        }
        return;
    }
    float complex G = OSL_GFromZ(rx, CFG_GetParam(CFG_PARAM_R0));
    LCDColor sc = LCD_RGB(96, 96, 96);
    SMITH_DrawGrid(X0, Y0, R, sc, 0, SMITH_R50 | SMITH_Y50 | SMITH_R25 | SMITH_R10 | SMITH_R100 | SMITH_R200 |
                                     SMITH_J50 | SMITH_J100 | SMITH_J200 | SMITH_J25 | SMITH_J10);
    SMITH_DrawLabels(LCD_RGB(128, 128, 0), 0, SMITH_R50 | SMITH_R25 | SMITH_R10 | SMITH_R100 | SMITH_R200 |
                     SMITH_J50 | SMITH_J100 | SMITH_J200 | SMITH_J25 | SMITH_J10);

    //Draw mini-scan points
    SMITH_ResetStartPoint();
    uint32_t i;
    int x, y;
    for (i = 0; i < 21; i++)
    {
        if (i == 10)
            continue;
        float complex Gx = OSL_GFromZ(z500[i], CFG_GetParam(CFG_PARAM_R0));
        SMITH_DrawG(Gx, LCD_RGB(0, 0, 192));
        SMITH_ResetStartPoint();
    }

    x = (int)(crealf(G) * R) + X0;
    y = Y0 - (int)(cimagf(G) * R);
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
        z500[i] = DSP_MeasuredZ();
        vswr500[i] = DSP_CalcVSWR(z500[i]);
    }
    else
    {
        vswr500[i] = 9999.0;
    }
    BSP_LCD_SelectLayer(!BSP_LCD_GetActiveLayer());
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y), LCD_MakePoint(SCAN_ORIGIN_X + i * 10, SCAN_ORIGIN_Y), LCD_GREEN);
    BSP_LCD_SelectLayer(!BSP_LCD_GetActiveLayer());
    LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X, SCAN_ORIGIN_Y), LCD_MakePoint(SCAN_ORIGIN_X + i * 10, SCAN_ORIGIN_Y), LCD_GREEN);
    i++;
    if (i == 21)
    {
        i = 0;
        BSP_LCD_SelectLayer(!BSP_LCD_GetActiveLayer());
        LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X - 1, SCAN_ORIGIN_Y), LCD_MakePoint(SCAN_ORIGIN_X + 201, SCAN_ORIGIN_Y), LCD_BLUE);
        BSP_LCD_SelectLayer(!BSP_LCD_GetActiveLayer());
        LCD_Line(LCD_MakePoint(SCAN_ORIGIN_X - 1, SCAN_ORIGIN_Y), LCD_MakePoint(SCAN_ORIGIN_X + 201, SCAN_ORIGIN_Y), LCD_BLUE);
        return 1;
    }
    return 0;
}

//Display measured data
static void MeasurementModeDraw(DSP_RX rx)
{
    char str[100] = "";
    sprintf(str, "Magnitude diff %.2f dB     ", DSP_MeasuredDiffdB());
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLACK, 0, 38, str);

    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 62);
    if (fabsf(crealf(rx)) >= 999.5f)
        sprintf(str, "R: %.2fk  ", crealf(rx) / 1000.f);
    else if (fabsf(crealf(rx)) >= 199.5f)
        sprintf(str, "R: %.0f  ", crealf(rx));
    else
        sprintf(str, "R: %.1f  ", crealf(rx));
    if (fabsf(cimagf(rx)) > 999.5f)
        sprintf(&str[strlen(str)], "X: %.2fk", cimagf(rx) / 1000.f);
    else if (fabsf(cimagf(rx)) >= 199.5f)
        sprintf(&str[strlen(str)], "X: %.0f", cimagf(rx));
    else
        sprintf(&str[strlen(str)], "X: %.1f", cimagf(rx));
    FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 0, 62, str);

    float VSWR = DSP_CalcVSWR(rx);
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 92);
    sprintf(str, "VSWR: %.1f (Z0 %lu)", VSWR, CFG_GetParam(CFG_PARAM_R0));
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
    float complex g = OSL_GFromZ(rx, CFG_GetParam(CFG_PARAM_R0));
    float ga = cabsf(g); //G amplitude
    if (ga > 0.01)
    {
        float cl = -10. * log10f(ga);
        if (cl < 0.001f)
            cl = 0.f;
        sprintf(str, "MCL: %.2f dB, |Z|: %.1f", cl, cabsf(rx));

        // Calculate open line phase length, in degrees
        if (ga > 0.9f) // it is reasonable for short enough, low loss line only
        {
            float complex fi = clogf(g) / (-2. * I);
            float fi_degrees = crealf(fi) * 180.f / M_PI;
            if (fi_degrees < 0.f)
            {
                fi_degrees += 180.f;
            }
            sprintf(&str[strlen(str)], ", \xd4: %.1f\xb0", fi_degrees);
        }

        FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 0, 158, str);
    }

    DrawSmallSmith(380, 180, 80, rx);
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
    while(TOUCH_IsPressed());
    Sleep(50);
}

static void MEASUREMENT_SetFreq(void)
{
    while(TOUCH_IsPressed());
    uint32_t val = NumKeypad(CFG_GetParam(CFG_PARAM_MEAS_F)/1000, BAND_FMIN/1000, CFG_GetParam(CFG_PARAM_BAND_FMAX)/1000, "Set measurement frequency, kHz");
    CFG_SetParam(CFG_PARAM_MEAS_F, val * 1000);
    CFG_Flush();
    redrawWindow = 1;
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

static const struct HitRect hitArr[] =
{
    //        x0,  y0, width, height, callback
    HITRECT(   0, 200,  60,  72, MEASUREMENT_Exit),
    HITRECT(  75, 200,  80,  72, MEASUREMENT_SetFreq),
    HITRECT( 190, 200,  80,  72, MEASUREMENT_Screenshot),
    HITRECT(   0,   0,  80, 100, MEASUREMENT_FDecr_500k),
    HITRECT(  80,   0,  80, 100, MEASUREMENT_FDecr_100k),
    HITRECT( 160,   0,  70, 100, MEASUREMENT_FDecr_10k),
    HITRECT( 250,   0,  70, 100, MEASUREMENT_FIncr_10k),
    HITRECT( 320,   0,  80, 100, MEASUREMENT_FIncr_100k),
    HITRECT( 400,   0,  80, 100, MEASUREMENT_FIncr_500k),
    HITRECT( 300, 110, 180, 110, MEASUREMENT_SmithMatch),
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
    if (!(fbkup >= BAND_FMIN &&
          fbkup <= CFG_GetParam(CFG_PARAM_BAND_FMAX) &&
          (fbkup % 1000) == 0))
    {
        CFG_SetParam(CFG_PARAM_MEAS_F, 14000000ul);
        CFG_Flush();
    }

    rqExit = 0;
    fChanged = 0;
    isMatch = 0;
    redrawWindow = 0;

    BSP_LCD_SelectLayer(0);
    LCD_FillAll(LCD_BLACK);
    BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_BLACK);
    LCD_ShowActiveLayerOnly();

    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 120, 60, "Measurement mode");
    if (-1 == OSL_GetSelected())
    {
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 80, 120, "No calibration file selected!");
        Sleep(200);
    }
    Sleep(300);
    while(TOUCH_IsPressed());

    uint32_t activeLayer;

MEASUREMENT_REDRAW:
    for (uint32_t layer = 0; layer < 2; layer++)
    {
        BSP_LCD_SelectLayer(layer);
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

        FONT_Write(FONT_FRAN, LCD_WHITE, LCD_DYELLOW, 0, 250, "    Exit    ");
        FONT_Write(FONT_FRAN, LCD_WHITE, LCD_DBLUE, 72, 250, "  Set frequency...  ");
        FONT_Write(FONT_FRAN, LCD_WHITE, LCD_DBLUE, 184, 250, "  Save snapshot  ");

        ShowF();

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
    }
    LCD_ShowActiveLayerOnly();

    //Main window cycle
    for(;;)
    {
        int scanres = Scan500();
        GEN_SetMeasurementFreq(CFG_GetParam(CFG_PARAM_MEAS_F));
        Sleep(10);
        DSP_Measure(CFG_GetParam(CFG_PARAM_MEAS_F), 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        DSP_RX rx = DSP_MeasuredZ();

        activeLayer = BSP_LCD_GetActiveLayer();
        BSP_LCD_SelectLayer(!activeLayer);

        ShowF();
        MeasurementModeDraw(rx);
        if (scanres)
        {
            BSP_LCD_SelectLayer(activeLayer);
            MeasurementModeGraph(rx);
            BSP_LCD_SelectLayer(!activeLayer);
            MeasurementModeGraph(rx);
        }

        if (DSP_MeasuredMagVmv() < 1.)
        {
            FONT_Write(FONT_FRAN, LCD_BLACK, LCD_RED, 380, 2, "No signal  ");
        }
        else
        {
            FONT_Write(FONT_FRAN, LCD_GREEN, LCD_BLACK, 380, 2, "Signal OK  ");
        }

        LCD_ShowActiveLayerOnly();

        uint32_t speedcnt = 0;
        meas_maxstep = 500000;
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
            if (redrawWindow)
            {
                redrawWindow = 0;
                goto MEASUREMENT_REDRAW;
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
                meas_maxstep = 2000000;
        }
        speedcnt = 0;
        meas_maxstep = 500000;

        if (fChanged)
        {
            CFG_Flush();
            fChanged = 0;
        }
        Sleep(50);
    }
}

static bool Z0_IsLossy(float complex rx)
{
    float complex gamma;
    gamma = OSL_GFromZ(rx, CFG_GetParam(CFG_PARAM_R0));
    if (cabsf(gamma) < 0.7f)
    {
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Load is not a line, too lossy");
        while(!TOUCH_IsPressed())
        {
            Sleep(50);
        }
        return true;
    }
    return false;
}

static bool Z0_FLimit(uint32_t freq)
{
    if ((freq > 150000000u) || (freq < BAND_FMIN))
    {
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Frequency limit, line is too short?");
        while(!TOUCH_IsPressed())
        {
            Sleep(50);
        }
        return true;
    }
    return false;
}

void Z0_Proc(void)
{
    rqExit = 0;

    BSP_LCD_SelectLayer(0);
    LCD_FillAll(LCD_BLACK);
    BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_BLACK);
    LCD_ShowActiveLayerOnly();

    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 120, 60, "Finding Z0 of open line");

    while(TOUCH_IsPressed())
    {
        Sleep(50);
    }

    uint32_t freq = 2 * BAND_FMIN;
    float complex rx;

    // Initial check
    DSP_Measure(freq, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
    rx = DSP_MeasuredZ();
    if (Z0_IsLossy(rx))
        goto EXIT;

    if (cimagf(rx) > 0.f)
    {
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Load is not an open line");
        while(!TOUCH_IsPressed())
        {
            Sleep(50);
        }
        goto EXIT;
    }

    // Find where X becomes > 0, step 500 kHz
    while (true)
    {
        if (Z0_FLimit(freq))
            goto EXIT;
        DSP_Measure(freq, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        rx = DSP_MeasuredZ();
        if (rqExit || Z0_IsLossy(rx))
            goto EXIT;
        if (cimagf(rx) > 0.f)
            break;
        freq += 500000u;
    }

    freq -= 100000u;
    // Find where X becomes < 0, step 100 kHz
    while (true)
    {
        if (Z0_FLimit(freq))
            goto EXIT;
        DSP_Measure(freq, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        rx = DSP_MeasuredZ();
        if (rqExit || Z0_IsLossy(rx))
            goto EXIT;
        if (cimagf(rx) < 0.f)
            break;
        freq -= 100000u;
    }

    freq += 10000u;
    // Find where X becomes > 0, step 10 kHz
    while (true)
    {
        if (Z0_FLimit(freq))
            goto EXIT;
        DSP_Measure(freq, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        rx = DSP_MeasuredZ();
        if (rqExit || Z0_IsLossy(rx))
            goto EXIT;
        if (cimagf(rx) > 0.f)
            break;
        freq += 10000u;
    }

    freq -= 1000u;
    // Find where X becomes < 0, step 1 kHz
    while (true)
    {
        if (Z0_FLimit(freq))
            goto EXIT;
        DSP_Measure(freq, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
        rx = DSP_MeasuredZ();
        if (rqExit || Z0_IsLossy(rx))
            goto EXIT;
        if (cimagf(rx) < 0.f)
            break;
        freq -= 1000u;
    }

    // -X measured at half the frequency corresponding to line length lambda/4 - is actually the line Z0
    DSP_Measure(freq / 2, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
    rx = DSP_MeasuredZ();

    FONT_Print(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 0, 100, "Measured Z0: %.1f", -cimagf(rx));
    FONT_Print(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 0, 140, "L/4 freq %u kHz", freq / 1000);
    if (Z0_IsLossy(rx))
    {
        FONT_Print(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 0, 180, "Line is lossy, Q = %.1f", cimagf(rx)/crealf(rx));
    }
    Sleep(500);

    while (!TOUCH_IsPressed())
    {
        Sleep(50);
    }

EXIT:
    GEN_SetMeasurementFreq(0);
    while(TOUCH_IsPressed());
}
