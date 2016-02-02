/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "bkup.h"
#include "osl.h"
#include "gen.h"
#include "configwnd.h"
#include "hit.h"
#include "si5351.h"
#include "dsp.h"

void Sleep(uint32_t);
static void SiCalProc(void);
static void OSLCalProc(void);
static void HwCalProc(void);
static void OSLFileHitCB(void);
static void ExitRqCB(void);

static const char* const fnames[] = {"None", "A", "B", "C"};

static const struct HitRect hitArr1[] =
{
    HITRECT(160, 0, 130, 32, OSLFileHitCB),
    HITRECT(0, 50, 320, 32, SiCalProc),
    HITRECT(0, 100, 320, 32, OSLCalProc),
    HITRECT(0, 150, 320, 32, HwCalProc),
    HITRECT(0, 210, 320, 30, ExitRqCB),
    HITEND
};

static int rqExit = 0;
static LCDPoint pt;

static void DrawCfgWnd(void)
{
    LCD_FillAll(LCD_BLACK);
    FONT_Print(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 0, 0, "Use calibration file: < %s >", fnames[OSL_GetSelected() + 1]);

    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 0, 50, "Clock calibration...");

    if (OSL_GetSelected() != -1)
        FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 0, 100, "OSL calibration...");
    else
        FONT_Write(FONT_FRANBIG, LCD_GRAY, LCD_BLACK, 0, 100, "OSL calibration...");
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 0, 150, "HW calibration...");
    FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 130, 200, "Exit");
}

static void OSLFileHitCB(void)
{
    OSL_Select(OSL_GetSelected() + 1);
    Sleep(100);
}

static void ExitRqCB(void)
{
    rqExit = 1;
}

void CONFIG_Proc(void)
{
    DrawCfgWnd();
    for(;;)
    {
        DrawCfgWnd();
        while(TOUCH_IsPressed());
        while(!TOUCH_Poll(&pt));
        HitTest(hitArr1, pt.x, pt.y);
        if (rqExit)
        {
            rqExit = 0;
            while(TOUCH_IsPressed());
            return;
        }
    }
}

//==================================================================================================================
// Si5351a clock calibration window
//==================================================================================================================
static uint32_t GenFreq = 0;
static int GenCorr = 0;
static int GenPressedCtr = 0;

static void ShowSiF()
{
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 0);
    FONT_Print(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 20, 0, "<< < F: %u kHz > >>", (unsigned int)(GenFreq / 1000));
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 70);
    FONT_Print(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 40, 70, "<< < %+d Hz > >>", GenCorr);
}

static void GenMinus1000()
{
    if (GenFreq > 1000000)
        GenFreq = ((GenFreq - 1000000) / 1000000) * 1000000;
    else
        GenFreq = 1000000;
    if (GenFreq < BAND_FMIN)
        GenFreq = BAND_FMIN;
    else if (GenFreq > BAND_FMAX)
        GenFreq = BAND_FMAX;
    ShowSiF();
    GEN_SetMeasurementFreq(GenFreq);
    BKUP_SaveFGen(GenFreq);
}

static void GenMinus100()
{
    GenFreq = ((GenFreq - 100000) / 100000) * 100000;
    if (GenFreq < BAND_FMIN)
        GenFreq = BAND_FMIN;
    else if (GenFreq > BAND_FMAX)
        GenFreq = BAND_FMAX;
    ShowSiF();
    GEN_SetMeasurementFreq(GenFreq);
    BKUP_SaveFGen(GenFreq);
}

static void GenMinus5()
{
    GenFreq = ((GenFreq - 5000) / 5000) * 5000;
    if (GenFreq < BAND_FMIN)
        GenFreq = BAND_FMIN;
    else if (GenFreq > BAND_FMAX)
        GenFreq = BAND_FMAX;
    ShowSiF();
    GEN_SetMeasurementFreq(GenFreq);
    BKUP_SaveFGen(GenFreq);
}

static void GenPlus5()
{
    GenFreq = ((GenFreq + 5000) / 5000) * 5000;
    if (GenFreq < BAND_FMIN)
        GenFreq = BAND_FMIN;
    else if (GenFreq > BAND_FMAX)
        GenFreq = BAND_FMAX;
    ShowSiF();
    GEN_SetMeasurementFreq(GenFreq);
    BKUP_SaveFGen(GenFreq);
}

static void GenPlus100()
{
    GenFreq = ((GenFreq + 100000) / 100000) * 100000;
    if (GenFreq < BAND_FMIN)
        GenFreq = BAND_FMIN;
    else if (GenFreq > BAND_FMAX)
        GenFreq = BAND_FMAX;
    ShowSiF();
    GEN_SetMeasurementFreq(GenFreq);
    BKUP_SaveFGen(GenFreq);
}

static void GenPlus1000()
{
    GenFreq = ((GenFreq + 1000000) / 1000000) * 1000000;
    if (GenFreq < BAND_FMIN)
        GenFreq = BAND_FMIN;
    else if (GenFreq > BAND_FMAX)
        GenFreq = BAND_FMAX;
    ShowSiF();
    GEN_SetMeasurementFreq(GenFreq);
    BKUP_SaveFGen(GenFreq);
}

static void GenCalMinus()
{
    if (GenPressedCtr > 10)
        GenCorr -= 20;
    else
        --GenCorr;
    if (GenCorr < -7000)
        GenCorr = -7000;
    ShowSiF();
    BKUP_SaveXtalCorr(GenCorr);
    si5351_init();
    GEN_SetMeasurementFreq(GenFreq);
}

static void GenCalPlus()
{
    if (GenPressedCtr > 10)
        GenCorr += 20;
    else
        ++GenCorr;
    if (GenCorr > 7000)
        GenCorr = 7000;
    ShowSiF();
    BKUP_SaveXtalCorr(GenCorr);
    si5351_init();
    GEN_SetMeasurementFreq(GenFreq);
}

static const struct HitRect hitGenCal[] =
{
    HITRECT(0, 0, 53, 32, GenMinus1000),
    HITRECT(53, 0, 53, 32, GenMinus100),
    HITRECT(106, 0, 53, 32, GenMinus5),
    HITRECT(159, 0, 53, 32, GenPlus5),
    HITRECT(212, 0, 53, 32, GenPlus100),
    HITRECT(265, 0, 55, 32, GenPlus1000),
    HITRECT(0, 70, 140, 32, GenCalMinus),
    HITRECT(180, 70, 140, 32, GenCalPlus),
    HITRECT(0, 210, 320, 30, ExitRqCB),
    HITEND
};

static void SiCalProc(void)
{//calibrate generator clock offset
    LCD_FillAll(LCD_BLACK);
    Sleep(50);
    while(TOUCH_IsPressed());

    {
        uint32_t fbkup = BKUP_LoadFGen();
        if (fbkup >= BAND_FMIN && fbkup <= BAND_FMAX && (fbkup % 5000) == 0)
            GenFreq = fbkup;
        else
            GenFreq = 14000000ul;
    }

    GenCorr = BKUP_LoadXtalCorr();
    GEN_SetMeasurementFreq(GenFreq);
    ShowSiF();
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLACK, 50, 120, "Set Si5351a Xtal clock correction");
    FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 10, 200, "Exit");

    for(;;)
    {
        while(!TOUCH_Poll(&pt))
            GenPressedCtr = 0;
        GenPressedCtr++;
        HitTest(hitGenCal, pt.x, pt.y);
        Sleep(50);
        if (rqExit)
        {
            rqExit = 0;
            while(TOUCH_IsPressed());
            return;
        }
    }
}

//=================================================================================================
static void OSLCalProc(void)
{
    if (OSL_GetSelected() == -1)
        return;
    LCD_FillAll(LCD_BLACK);
    Sleep(50);
    while(TOUCH_IsPressed());

    OSL_RecalcLoads();

    FONT_Print(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 10, 0, "OSL calibration, file %s", fnames[OSL_GetSelected() + 1]);
    FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_BLUE, 130, 200, " Exit ");

    //Now select OSl short and open loads, if needed
    for(;;)
    {
        FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 50);
        FONT_Print(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 0, 50, "Short load: %.0f Ohm", OSL_RSHORT);
        FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
        FONT_Print(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 0, 100, "Open load: %.0f Ohm", OSL_ROPEN);
        FONT_Write(FONT_FRANBIG, LCD_BLACK, LCD_RED, 130, 150, "  GO!  ");
        Sleep(200);
        while(TOUCH_IsPressed());
        while(!TOUCH_Poll(&pt));
        if (pt.y > 200)
        {
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 50);
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 150);
            goto ExitLabel; //Exit
        }
        if (pt.y > 150 && pt.y <182)
        {//GO!
            Sleep(200);
            while(TOUCH_IsPressed());
            break;
        }
        else if (pt.y > 50 && pt.y < 82)
        {//short load selection
            int r = (int)OSL_RSHORT;
            switch (r)
            {
            case 0:
                OSL_RSHORT = 5.0f;
                break;
            case 5:
                OSL_RSHORT = 10.0f;
                break;
            default:
                OSL_RSHORT = 0.0f;
                break;
            }
            OSL_RecalcLoads();
        }
        else if (pt.y > 100 && pt.y < 132)
        {//open load selection
            int r = (int)OSL_ROPEN;
            switch (r)
            {
            case 300:
                OSL_ROPEN = 333.0f;
                break;
            case 333:
                OSL_ROPEN = 500.0f;
                break;
            case 500:
                OSL_ROPEN = 99999.0f;
                break;
            default:
                OSL_ROPEN = 300.0f;
                break;
            }
            OSL_RecalcLoads();
        }
    }

    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 50);
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 150);

    FONT_Print(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Load %.1f Ohm and tap", OSL_RSHORT);
    for(;;)
    {
        while(!TOUCH_Poll(&pt));
        if (pt.y > 150)
            goto ExitLabel; //Exit
        if (pt.y > 90)
            break;
    }

    //Clear OSL file
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Clearing file");
    Sleep(200);
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    if (!OSL_CleanOSL())
    {
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "FAILED to clear OSL file!");
        goto ExitLabel;
    }
    else
    {
        FONT_Print(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 0, 100, "OSL file %s cleared!", fnames[OSL_GetSelected() + 1]);
        Sleep(200);
    }

    g_favor_precision = 1;
    //Scan SHORT
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    if (!OSL_ScanShort())
    {
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Scanning FAILED!");
        goto ExitLabel;
    }

    while(TOUCH_IsPressed());
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    FONT_Print(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Load %.1f Ohm and tap", OSL_RLOAD);
    while(!TOUCH_Poll(&pt));
    for(;;)
    {
        while(!TOUCH_Poll(&pt));
        if (pt.y > 150)
        {
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
            FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "OSL calibration interrupted");
            goto ExitLabel; //Exit
        }
        if (pt.y > 90)
            break;
    }

    //Scan LOAD
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    if (!OSL_ScanLoad())
    {
        FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Scanning FAILED!");
        goto ExitLabel;
    }

    while(TOUCH_IsPressed());
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    FONT_Print(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Load %.1f Ohm and tap", OSL_ROPEN);
    while(!TOUCH_Poll(&pt));
    for(;;)
    {
        while(!TOUCH_Poll(&pt));
        if (pt.y > 150)
        {
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
            FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "OSL calibration interrupted");
            goto ExitLabel; //Exit
        }
        if (pt.y > 90)
            break;
    }

    //Scan OPEN
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    if (!OSL_ScanOpen())
    {
        FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "Scanning FAILED!");
        goto ExitLabel;
    }

    //Calculate
    while(TOUCH_IsPressed());
    Sleep(100);
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    FONT_Write(FONT_FRANBIG, LCD_PURPLE, LCD_BLACK, 0, 100, "Processing data...");
    if (!OSL_Calculate())
    {
        FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
        FONT_Print(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 100, "FAILED to process data!");
        goto ExitLabel;
    }

    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    FONT_Print(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 30, 100, "Calibration success");

ExitLabel:
    g_favor_precision = 0;
    while(!TOUCH_IsPressed());
    Sleep(50);
    while(TOUCH_IsPressed());
}

static void HwCalProc(void)
{
    LCD_FillAll(LCD_BLACK);
    Sleep(50);
    while(TOUCH_IsPressed());
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 80, 0, "HW calibration");
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 60, 70, "Set jumper to HW calibration position");
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 120, 90, "and tap here");
    FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 140, 200, "Exit");
    while(!TOUCH_Poll(&pt));
    if (pt.y > 150)
        return; //Exit

    FONT_ClearLine(FONT_FRAN, LCD_BLACK, 70);
    FONT_ClearLine(FONT_FRAN, LCD_BLACK, 90);

    g_favor_precision = 1;
    int res = OSL_ScanADCCorrection();
    g_favor_precision = 0;
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
    if (!res)
        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 110, 100, "FAILED");
    else
        FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 110, 100, "Success");

    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 90, 70, "Set jumper to normal position");

    while(!TOUCH_IsPressed());
    Sleep(50);
    while(TOUCH_IsPressed());
}
