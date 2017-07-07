/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <math.h>
#include <complex.h>
#include "arm_math.h"

#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "config.h"
#include "crash.h"
#include "dsp.h"
#include "gen.h"
#include "oslfile.h"
#include "stm32746g_discovery_lcd.h"
#include "screenshot.h"
#include "tdr.h"

//Reusing buffers from dsp.c to save memory
extern float rfft_input[NSAMPLES];
extern float rfft_output[NSAMPLES];
extern const float complex *prfft;

#define NUMTDRSAMPLES 256

//Scan load from 500 KHz up to 127.5 MHz step 500KHz
static void TDR_Scan(void)
{
    DSP_Measure(BAND_FMIN, 1, 1, 1); //Fake initial run to let the circuit stabilize

    rfft_output[0] = 0.f+0.fi; //DC bin

    uint32_t i;
    for (i = 1; i < NUMTDRSAMPLES; i++)
    {
        uint32_t freqHz = i * 500000;
        DSP_Measure(freqHz, 1, 1, CFG_GetParam(CFG_PARAM_PAN_NSCANS));
        float complex G = OSL_GFromZ(DSP_MeasuredZ(), (float)CFG_GetParam(CFG_PARAM_R0));
        rfft_output[i] = G;
    }

    // Do IRFFT
    arm_rfft_fast_instance_f32 S;
    arm_rfft_fast_init_f32(&S, NUMTDRSAMPLES*2);
    arm_rfft_fast_f32(&S, rfft_output, rfft_input, 1);
}

void TDR_Proc(void)
{
}
