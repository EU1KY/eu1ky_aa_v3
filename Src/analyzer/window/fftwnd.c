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

#define NSAMPLES 512
#define NDUMMY 32
#define FSAMPLE I2S_AUDIOFREQ_48K
#define step 1

static int16_t audioBuf[(NSAMPLES + NDUMMY) * 2] = {0};
static float rfft_input[NSAMPLES];
static float rfft_output[NSAMPLES];
static float rfft_mags[NSAMPLES/2];
static const float complex *prfft   = (float complex*)rfft_output;
static const float binwidth = ((float)(FSAMPLE)) / (NSAMPLES);
static float wnd[NSAMPLES];
static enum {W_NONE, W_SINUS, W_HANN, W_HAMMING, W_BLACKMAN, W_FLATTOP} wndtype = W_NONE;
static const char* wndstr[] = {"None", "Sinus", "Hann", "Hamming", "Blackman", "Flattop"};
static uint32_t oscilloscope = 1;
static uint32_t rqExit = 0;

static void prepare_windowing(void);

static void FFTWND_SwitchWindowing(void)
{
    wndtype++;
    prepare_windowing();
    FONT_SetAttributes(FONT_FRANBIG, LCD_WHITE, LCD_BLACK);
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 32);
    FONT_Printf(0, 32, "Windowing: %s", wndstr[wndtype]);
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
    HITRECT(0, 0, 479, 140, FFTWND_SwitchWindowing),
    HITRECT(0, 160, 50, 279, FFTWND_ExitWnd),
    HITRECT(0, 140, 479, 279, FFTWND_SwitchDispMode),
    HITEND
};


static void prepare_windowing(void)
{
    int32_t i;
    int ns = NSAMPLES - 1;
    for(i = 0; i < NSAMPLES; i++)
    {
        switch (wndtype)
        {
        default:
            wndtype = W_NONE;
            //Fall through
        case W_NONE:
            wnd[i] = 1.0f;
            break;
        case W_SINUS:
            wnd[i] = sinf((M_PI * i)/ns);
            break;
        case W_HANN:
            wnd[i] = 0.5 - 0.5 * cosf((2 * M_PI * i)/ns);
            break;
        case W_HAMMING:
            wnd[i] = 0.54 - 0.46 * cosf((2 * M_PI * i)/ns);
            break;
        case W_BLACKMAN:
            //Blackman window, >66 dB OOB rejection
            wnd[i] = 0.426591f - .496561f * cosf( (2 * M_PI * i) / ns) + .076848f * cosf((4 * M_PI * i) / ns);
            break;
        case W_FLATTOP:
            //Flattop window
            wnd[i] = (1.0 - 1.93*cosf((2 * M_PI * i) / ns) + 1.29 * cosf((4 * M_PI * i) / ns)
                      -0.388*cosf((6 * M_PI * i) / ns) +0.0322*cosf((8 * M_PI * i) / ns)) / 3.f;
            break;
        }
    }
}

static void do_fft_audiobuf(int ch)
{
    //Only one channel
    int i;
    arm_rfft_fast_instance_f32 S;

    int16_t* pBuf = &audioBuf[NDUMMY + (ch != 0)];
    for(i = 0; i < NSAMPLES; i++)
    {
        rfft_input[i] = (float)*pBuf;
        pBuf += 2;
        rfft_input[i] *= wnd[i];
    }

    arm_rfft_fast_init_f32(&S, NSAMPLES);
    arm_rfft_fast_f32(&S, rfft_input, rfft_output, 0);

    for (i = 0; i < NSAMPLES/2; i++)
    {
        float complex binf = prfft[i];
        rfft_mags[i] = cabsf(binf) / (NSAMPLES/2);
    }
}

void measure(void)
{
    extern SAI_HandleTypeDef haudio_in_sai;
    HAL_SAI_Receive(&haudio_in_sai, (uint8_t*)audioBuf, (NSAMPLES + NDUMMY) * 2, HAL_MAX_DELAY);
}

void FFTWND_Proc(void)
{
    uint32_t ctr = 0;
    int i;

    rqExit = 0;
    prepare_windowing();
    LCD_FillAll(LCD_BLACK);

    FONT_SetAttributes(FONT_FRANBIG, LCD_WHITE, LCD_BLACK);
    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 32);
    FONT_Printf(0, 32, "Windowing: %s", wndstr[wndtype]);

    while (1)
    {
        (HAL_GetTick() & 0x100 ? BSP_LED_On : BSP_LED_Off)(LED1);
        LCDPoint pt;
        if (TOUCH_Poll(&pt))
        {
            HitTest(hitArr, pt.x, pt.y);
            HAL_Delay(50);
            while(TOUCH_IsPressed());
            if (rqExit)
                return;
            continue;
        }
        else
        {
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 0);
        }

        uint32_t tmstart = HAL_GetTick();
        measure();
        tmstart = HAL_GetTick() - tmstart;

        if (oscilloscope)
        {
            uint16_t lasty_left = 210;
            uint16_t lasty_right = 210;
            int16_t* pData;
            int16_t minMag = 32767;
            int16_t maxMag = -32767;
            int32_t magnitude = 0;

            while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS)); //Wait for LCD output cycle finished to avoid flickering
            LCD_FillRect(LCD_MakePoint(0, 140), LCD_MakePoint(LCD_GetWidth()-1, LCD_GetHeight()-1), 0xFF000020);

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
                    lasty_left = 210 - (int)(*pData++ * wnd[i*step]) / 500;
                    if (lasty_left > LCD_GetHeight()-1)
                        lasty_left = LCD_GetHeight()-1;
                    if (lasty_left < 0)
                        lasty_left = 0;
                    LCD_SetPixel(LCD_MakePoint(i, lasty_left), LCD_COLOR_GREEN);
                    lasty_right = 210 - (int)(*pData++ * wnd[i*step]) / 500;
                    if (lasty_right > LCD_GetHeight()-1)
                        lasty_right = LCD_GetHeight()-1;
                    if (lasty_right < 140)
                        lasty_right = 140;
                    LCD_SetPixel(LCD_MakePoint(i, lasty_right), LCD_COLOR_RED);
                }
                else
                {
                    uint16_t y_left = 210 - (int)(*pData++ * wnd[i*step]) / 500;
                    uint16_t y_right = 210 - (int)(*pData++ * wnd[i*step]) / 500;
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
            while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS)); //Wait for LCD output cycle finished to avoid flickering
            //Draw spectrum
            LCD_FillRect(LCD_MakePoint(0, 140), LCD_MakePoint(LCD_GetWidth()-1, LCD_GetHeight()-1), 0xFF000020);

            //Draw horizontal grid lines
            for (i = LCD_GetHeight()-1; i > 140; i-=10)
            {
                LCD_Line(LCD_MakePoint(0, i), LCD_MakePoint(LCD_GetWidth()-1, i), 0xFF303070);
            }

            for (int ch = 0; ch < 2; ch++)
            {
                uint32_t tstart = HAL_GetTick();
                do_fft_audiobuf(ch);
                tstart = HAL_GetTick() - tstart;

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

                if (0 == ch)
                {
                    FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 64);
                    FONT_SetAttributes(FONT_FRANBIG, LCD_BLUE, LCD_BLACK);
                    FONT_Printf(0, 64, "RFFT: %d ms Mag %.0f @ %.0f Hz", tstart, maxmag, binwidth * idxmax);
                }

                //Draw spectrum
                for (int x = 0; x < 240; x++)
                {
                    int y = (int)(LCD_GetHeight()- 1 - 20 * log10f(rfft_mags[x]));
                    if (y <= LCD_GetHeight()-1)
                    {
                        uint32_t clr = ch ? LCD_GREEN : LCD_RED;
                        LCD_Line(LCD_MakePoint(x + ch*240, LCD_GetHeight()-1), LCD_MakePoint(x + ch*240, y), clr);
                    }
                    if (x >= LCD_GetWidth()-1)
                        break;
                }
            }
        }
        HAL_Delay(100);
    }
}
