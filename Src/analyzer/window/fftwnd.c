#include <complex.h>
#include <stdint.h>
#include <math.h>
#include "arm_math.h"
#include <string.h>

#include "stm32f7xx_hal.h"
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_audio.h"
#include "stm32746g_discovery_lcd.h"

#include "fftwnd.h"
#include "LCD.h"
#include "font.h"
#include "touch.h"
#include "hit.h"
#include "crash.h"
#include "gen.h"
#include "dsp.h"
#include "ff.h"
#include "config.h"
#include "si5351.h"

#define step 1

extern void Sleep(uint32_t nms);

static uint32_t oscilloscope = 0;
static uint32_t rqExit = 0;
static uint32_t fChanged1 = 0;
static float rfft_mags[NSAMPLES/2];

extern int16_t audioBuf[(NSAMPLES + NDUMMY) * 2];
extern float rfft_input[NSAMPLES];
extern float rfft_output[NSAMPLES];
extern const float complex *prfft;
extern float windowfunc[NSAMPLES];

static void ShowF(void)
{
    char str[50];
    sprintf(str, "F: %u kHz        ", (unsigned int)(CFG_GetParam(CFG_PARAM_MEAS_F) / 1000));
    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 0, 2, str);
}

static void FDecr(uint32_t step1)
{
    uint32_t MeasurementFreq = CFG_GetParam(CFG_PARAM_MEAS_F);
    if(MeasurementFreq > step1 && MeasurementFreq % step1 != 0)
    {
        MeasurementFreq -= (MeasurementFreq % step1);
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged1 = 1;
    }
    if(MeasurementFreq < BAND_FMIN)
    {
        MeasurementFreq = BAND_FMIN;
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged1 = 1;
    }
    if(MeasurementFreq > BAND_FMIN)
    {
        if(MeasurementFreq > step1 && (MeasurementFreq - step1) >= BAND_FMIN)
        {
            MeasurementFreq = MeasurementFreq - step1;
            CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
            fChanged1 = 1;
        }
    }
}

static void FIncr(uint32_t step1)
{
    uint32_t MeasurementFreq = CFG_GetParam(CFG_PARAM_MEAS_F);
    if(MeasurementFreq > step1 && MeasurementFreq % step1 != 0)
    {
        MeasurementFreq -= (MeasurementFreq % step1);
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged1 = 1;
    }
    if(MeasurementFreq < BAND_FMIN)
    {
        MeasurementFreq = BAND_FMIN;
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged1 = 1;
    }
    if(MeasurementFreq <  CFG_GetParam(CFG_PARAM_BAND_FMAX))
    {
        if ((MeasurementFreq + step1) >  CFG_GetParam(CFG_PARAM_BAND_FMAX))
            MeasurementFreq =  CFG_GetParam(CFG_PARAM_BAND_FMAX);
        else
            MeasurementFreq = MeasurementFreq + step1;
        CFG_SetParam(CFG_PARAM_MEAS_F, MeasurementFreq);
        fChanged1 = 1;
    }
}

static void FFTWND_FDecr_10k(void)
{
    FDecr(500000);
}
static void FFTWND_FDecr_5k(void)
{
    FDecr(100000);
}
static void FFTWND_FDecr_1k(void)
{
    FDecr(5000);
}
static void FFTWND_FIncr_1k(void)
{
    FIncr(5000);
}
static void FFTWND_FIncr_5k(void)
{
    FIncr(100000);
}
static void FFTWND_FIncr_10k(void)
{
    FIncr(500000);
}

static void FFTWND_ExitWnd(void)
{
    rqExit = 1;
}

static void FFTWND_SwitchDispMode(void)
{
    oscilloscope = !oscilloscope;
}

static const struct HitRect hitArr[] =
{
    //        x0,   y0, width,  height, callback
    HITRECT(   0, 200, 100,  80, FFTWND_ExitWnd),
    HITRECT(   0, 140, 480, 140, FFTWND_SwitchDispMode),
    HITRECT(   0,   0,  80, 100, FFTWND_FDecr_10k),
    HITRECT(  80,   0,  80, 100, FFTWND_FDecr_5k),
    HITRECT( 160,   0,  70, 100, FFTWND_FDecr_1k),
    HITRECT( 250,   0,  70, 100, FFTWND_FIncr_1k),
    HITRECT( 320,   0,  80, 100, FFTWND_FIncr_5k),
    HITRECT( 400,   0,  80, 100, FFTWND_FIncr_10k),
    HITEND
};


static void do_fft_audiobuf(int ch)
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

void FFTWND_Proc(void)
{
    int i;

    rqExit = 0;
    oscilloscope = 0;
    BSP_LCD_SelectLayer(0);
    LCD_FillAll(LCD_BLACK);
    BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_BLACK);
    LCD_ShowActiveLayerOnly();

    FONT_SetAttributes(FONT_FRANBIG, LCD_WHITE, LCD_BLACK);

    GEN_SetMeasurementFreq(CFG_GetParam(CFG_PARAM_MEAS_F));

#ifdef SI5351_ENABLE_DUMP_REGS
    if (CFG_SYNTH_SI5351 == CFG_GetParam(CFG_PARAM_SYNTH_TYPE))
        si5351_dump_regs();
#endif

    while(TOUCH_IsPressed())
    {
        Sleep(10);
    }

    uint32_t activeLayer;
    while (1)
    {
        LCDPoint pt;
        if (TOUCH_Poll(&pt))
        {
            HitTest(hitArr, pt.x, pt.y);
            if (fChanged1)
            {
                ShowF();
                Sleep(70);
                continue;
            }
            Sleep(50);
            while(TOUCH_IsPressed());
            if (rqExit)
            {
                GEN_SetMeasurementFreq(0);
                return;
            }
            continue;
        }
        else
        {
            if (fChanged1)
            {
                GEN_SetMeasurementFreq(CFG_GetParam(CFG_PARAM_MEAS_F));
                CFG_Flush();
                fChanged1 = 0;
            }
            activeLayer = BSP_LCD_GetActiveLayer();
            BSP_LCD_SelectLayer(!activeLayer);
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 0);

            //Draw freq change areas bar
            uint16_t y;
            for (y = 0; y < 2; y++)
            {
                LCD_Line(LCD_MakePoint(0,y), LCD_MakePoint(79,y), LCD_RGB(15,15,63));
                LCD_Line(LCD_MakePoint(80,y), LCD_MakePoint(159,y), LCD_RGB(31,31,127));
                LCD_Line(LCD_MakePoint(160,y), LCD_MakePoint(229,y),  LCD_RGB(64,64,255));
                LCD_Line(LCD_MakePoint(250,y), LCD_MakePoint(319,y), LCD_RGB(64,64,255));
                LCD_Line(LCD_MakePoint(320,y), LCD_MakePoint(399,y), LCD_RGB(31,31,127));
                LCD_Line(LCD_MakePoint(400,y), LCD_MakePoint(479,y), LCD_RGB(15,15,63));
            }
            ShowF();
        }

        uint32_t tmstart = HAL_GetTick();
        DSP_Sample();
        tmstart = HAL_GetTick() - tmstart;

        if (oscilloscope)
        {
            uint16_t lasty_left = 210;
            uint16_t lasty_right = 210;
            int16_t* pData;
            int16_t minMag = 32767;
            int16_t maxMag = -32767;
            int32_t magnitude = 0;

            LCD_WaitForRedraw();
            LCD_FillRect(LCD_MakePoint(0, 140), LCD_MakePoint(LCD_GetWidth()-1, LCD_GetHeight()-1), 0xFF000020);
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);

            //Calc magnitude for one channel only
            for (i = NDUMMY * 2; i < (NSAMPLES + NDUMMY) * 2; i += 2)
            {
                if (minMag > audioBuf[i])
                    minMag = audioBuf[i];
                if (maxMag < audioBuf[i])
                    maxMag = audioBuf[i];
            }
            magnitude = (maxMag - minMag) / 2;

            FONT_SetAttributes(FONT_FRANBIG, LCD_BLUE, LCD_BLACK);
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 64);
            FONT_Printf(0, 64, "Sampling %d ms, Magnitude: %d", tmstart, magnitude);

            pData = &audioBuf[NDUMMY];
            for (i = 0; i < NSAMPLES; i ++)
            {
                if (i == 0)
                {
                    lasty_left = 210 - (int)(*pData++ * windowfunc[i*step]) / 500;
                    if (lasty_left > LCD_GetHeight()-1)
                        lasty_left = LCD_GetHeight()-1;
                    if (lasty_left < 0)
                        lasty_left = 0;
                    LCD_SetPixel(LCD_MakePoint(i, lasty_left), LCD_COLOR_GREEN);
                    lasty_right = 210 - (int)(*pData++ * windowfunc[i*step]) / 500;
                    if (lasty_right > LCD_GetHeight()-1)
                        lasty_right = LCD_GetHeight()-1;
                    if (lasty_right < 140)
                        lasty_right = 140;
                    LCD_SetPixel(LCD_MakePoint(i, lasty_right), LCD_COLOR_RED);
                }
                else
                {
                    uint16_t y_left = 210 - (int)(*pData++ * windowfunc[i*step]) / 200; //500;
                    uint16_t y_right = 210 - (int)(*pData++ * windowfunc[i*step]) / 200; //500;
                    if (y_left > LCD_GetHeight()-1)
                        y_left = LCD_GetHeight()-1;
                    if (y_left < 140)
                        y_left = 140;
                    if (y_right > LCD_GetHeight()-1)
                        y_right = LCD_GetHeight()-1;
                    if (y_right < 140)
                        y_right = 140;
                    LCD_Line(LCD_MakePoint(i-1, lasty_left), LCD_MakePoint(i, y_left), LCD_RED);
                    LCD_Line(LCD_MakePoint(i-1, lasty_right), LCD_MakePoint(i, y_right), LCD_GREEN);
                    lasty_left = y_left;
                    lasty_right = y_right;
                }
                pData += (step * 2 - 2);
                if (i >= LCD_GetWidth()-1)
                    break;
            }
        }
        else //Spectrum
        {
            //Draw spectrum
            LCD_WaitForRedraw();
            LCD_FillRect(LCD_MakePoint(0, 140), LCD_MakePoint(LCD_GetWidth()-1, LCD_GetHeight()-1), 0xFF000020);

            //Draw horizontal grid lines
            for (i = LCD_GetHeight()-1; i > 140; i-=10)
            {
                LCD_Line(LCD_MakePoint(0, i), LCD_MakePoint(LCD_GetWidth()-1, i), 0xFF303070);
            }
            LCD_Line(LCD_MakePoint(240, 140), LCD_MakePoint(240, LCD_GetHeight()-1), 0xFF303070);

            for (int ch = 0; ch < 2; ch++)
            {
                do_fft_audiobuf(ch);

                //Calculate max magnitude bin
                float maxmag = 0.;
                int idxmax = -1;
                for (i = 5; i < NSAMPLES/2 - 5; i++)
                {
                    if (rfft_mags[i] > maxmag)
                    {
                        maxmag = rfft_mags[i];
                        idxmax = i;
                    }
                }

                //Calculate magnitude value considering +/-2 bins from maximum
                float P = 0.f;
                for (i = idxmax - 2; i <= idxmax + 2; i++)
                    P += powf(rfft_mags[i], 2);
                maxmag = sqrtf(P);

                //Draw spectrum
                for (int x = 0; x < 240; x++)
                {
                    if (x >= NSAMPLES/2)
                        break;
                    int y = (int)(LCD_GetHeight()- 1 - 20 * log10f(rfft_mags[x]));
                    if (y <= LCD_GetHeight()-1)
                    {
                        uint32_t clr = ch ? LCD_GREEN : LCD_RED;
                        LCD_Line(LCD_MakePoint(x + ch*240, LCD_GetHeight()-1), LCD_MakePoint(x + ch*240, y), clr);
                    }
                    if (x >= LCD_GetWidth()-1)
                        break;
                }

                float binwidth = ((float)(FSAMPLE)) / (NSAMPLES);
                if (0 == ch)
                {
                    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 64);
                    FONT_SetAttributes(FONT_FRANBIG, LCD_RED, LCD_BLACK);
                    FONT_Printf(0, 64, "Mag %.0f @ bin %d (%.0f) Hz", maxmag, idxmax, binwidth * idxmax);
                }
                else
                {
                    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
                    FONT_SetAttributes(FONT_FRANBIG, LCD_GREEN, LCD_BLACK);
                    FONT_Printf(0, 100, "Mag %.0f @ bin %d (%.0f) Hz", maxmag, idxmax, binwidth * idxmax);
                }
            }
        }
        LCD_ShowActiveLayerOnly();
        Sleep(100);
    }
}
