/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "LCD.h"
#include "dsp.h"
#include "font.h"
#include "touch.h"
#include "bkup.h"
#include "gen.h"
#include "osl.h"

void Sleep(uint32_t ms);

static uint32_t MeasurementFreq = 14000000;
static float vswr500[21];

static void ShowF()
{
    char str[50];
    sprintf(str, "F: %u kHz        ", (unsigned int)(MeasurementFreq / 1000));
    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 2, str);
}

//Scan VSWR in +/- 500 kHz range around measurement frequency with 100 kHz step, to draw a small graph below the measurement
static int Scan500(void)
{
    static int i = 0;
    int fq = (int)MeasurementFreq + (i - 10) * 50000;
    if (fq > 0)
    {
        GEN_SetMeasurementFreq(fq);
        Sleep(10);
        DSP_Measure(fq, 1, 1, 5);
        vswr500[i] = DSP_CalcVSWR(DSP_MeasuredZ());
    }
    else
    {
        vswr500[i] = 9999.0;
    }
    LCD_Line(LCD_MakePoint(60, 209), LCD_MakePoint(60 + i * 10, 209), LCD_GREEN);
    i++;
    if (i == 21)
    {
        i = 0;
        LCD_Line(LCD_MakePoint(59, 209), LCD_MakePoint(261, 209), LCD_BLUE);
        return 1;
    }
    return 0;
}

//Display measured data
static void MeasurementModeDraw(DSP_RX rx)
{
    char str[50] = "";
    FONT_ClearLine(FONT_FRAN, LCD_BLACK, 36);
    sprintf(str, "Magnitude diff %.2f dB", DSP_MeasuredDiffdB());
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLACK, 0, 38, str);

    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 62);
    sprintf(str, "R: %.1f  X: %.1f", crealf(rx), cimagf(rx));
    FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 0, 62, str);

    float VSWR = DSP_CalcVSWR(rx);
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 92);
    sprintf(str, "VSWR: %.1f", VSWR);
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
    float ga = cabsf(OSL_GFromZ(rx)); //G amplitude
    if (ga > 0.01)
    {
        float cl = -10. * log10f(ga);
        if (cl < 0.001f)
            cl = 0.f;
        sprintf(str, "MCL: %.2f dB", cl);
        FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 0, 158, str);
    }

}

//Draw a small (100x20 pixels) VSWR graph for data collected by Scan500()
static void MeasurementModeGraph(DSP_RX in)
{
    int idx = 0;
    float prev = 3.0;
    LCD_FillRect(LCD_MakePoint(60, 210), LCD_MakePoint(260, 230), LCD_RGB(0, 0, 48)); // Graph rectangle
    LCD_Line(LCD_MakePoint(160, 210), LCD_MakePoint(160, 230), LCD_RGB(48, 48, 48));  // Measurement frequency line
    LCD_Line(LCD_MakePoint(110, 210), LCD_MakePoint(110, 230), LCD_RGB(48, 48, 48));
    LCD_Line(LCD_MakePoint(210, 210), LCD_MakePoint(210, 230), LCD_RGB(48, 48, 48));
    LCD_Line(LCD_MakePoint(60, 220), LCD_MakePoint(260, 220), LCD_RGB(48, 48, 48));   // VSWR 2.0 line
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
            LCD_Line( LCD_MakePoint(60 + (idx - 1) * 10, 230 - ((int)(prev * 10) - 10)),
                      LCD_MakePoint(60 + idx * 10, 230 - ((int)(vswr * 10) - 10)),
                      LCD_YELLOW);
        }
        prev = vswr;
    }
}

//Handle touch screen in measurement mode
static int Touch(void)
{
    LCDPoint pt;
    uint32_t step = 100000;
    if(TOUCH_Poll(&pt))
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
            step = 10000;
        }
        uint32_t currF = MeasurementFreq;
        if(currF > step && currF % step != 0)
        {
            currF -= (currF % step);
        }
        if(currF < BAND_FMIN)
        {
            currF = BAND_FMIN;
        }
        if(pt.x > 160)
        {
            if(currF <= BAND_FMAX)
            {
                if ((currF + step) > BAND_FMAX)
                    MeasurementFreq = BAND_FMAX;
                else
                    MeasurementFreq = currF + step;
                GEN_SetMeasurementFreq(MeasurementFreq);
                BKUP_SaveFMeas(MeasurementFreq);
            }
            else
            {
                GEN_SetMeasurementFreq(currF);
            }
        }
        else
        {
            if(currF > BAND_FMIN)
            {
                if(currF > step && (currF - step) >= BAND_FMIN)
                {
                    MeasurementFreq = currF - step;
                    GEN_SetMeasurementFreq(MeasurementFreq);
                    BKUP_SaveFMeas(MeasurementFreq);
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
    }
    return 0; //no request to change window
}


//Measurement mode window. To change it to VSWR tap the lower part of display.
//To change frequency, in steps of +/- 500, 100 and 10 kHz, tap top part of the display,
//the step depends on how far you tap from the center.

void MEASUREMENT_Proc(void)
{
    //Load saved middle frequency value from BKUP registers 2, 3
    //to MeasurementFreq
    {
        uint32_t fbkup = BKUP_LoadFMeas();
        if (fbkup >= BAND_FMIN && fbkup <= BAND_FMAX && (fbkup % 10000) == 0)
        {
            MeasurementFreq = fbkup;
        }
        else
        {
            MeasurementFreq = 14000000ul;
            BKUP_SaveFMeas(MeasurementFreq);
        }
    }

    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 40, 60, "Measurement mode");
    Sleep(250);
    while(TOUCH_IsPressed());

    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRAN, LCD_BLUE, LCD_BLACK, 40, 184, "VSWR (1.0 ... 3.0), F +/- 500 KHz, step 50:");
    LCD_FillRect(LCD_MakePoint(60, 210), LCD_MakePoint(260, 230), LCD_RGB(0, 0, 48)); // Graph rectangle
    LCD_Rectangle(LCD_MakePoint(59, 209), LCD_MakePoint(261, 231), LCD_BLUE);

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

    DSP_RX rx;
    for(;;)
    {
        ShowF();
        int scanres = Scan500();
        GEN_SetMeasurementFreq(MeasurementFreq);
        Sleep(10);
#ifdef FAVOR_PRECISION
        DSP_Measure(MeasurementFreq, 1, 1, 13);
#else
        DSP_Measure(MeasurementFreq, 1, 1, 7);
#endif
        rx = DSP_MeasuredZ();
        MeasurementModeDraw(rx);
        if (scanres)
            MeasurementModeGraph(rx);

        if (DSP_MeasuredMagImv() < 100. && DSP_MeasuredMagQmv() < 100.)
        {
            FONT_Write(FONT_FRAN, LCD_BLACK, LCD_RED, 270, 2, "No signal  ");
        }
        else
        {
            FONT_Write(FONT_FRAN, LCD_GREEN, LCD_BLACK, 270, 2, "Signal OK");
        }

        if(Touch())
        {
            return; //change window
        }
        else
        {
            while(TOUCH_IsPressed())
            {
                Sleep(50);
                if(Touch())
                    return; //change window
            }
        }
        Sleep(50);
    }
};

