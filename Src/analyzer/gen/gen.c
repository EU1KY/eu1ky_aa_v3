/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */
#include "si5351.h"
#include "gen.h"
#include "dsp.h"
#include "config.h"
#include "stm32746g_discovery_audio.h"

static uint32_t lastSetFreq = 14000000ul;
extern void Sleep(uint32_t);

void GEN_SetMeasurementFreq(uint32_t fhz)
{
    uint32_t IF = DSP_GetIF(); //Required IF frequency
    if (fhz == 0)
    {
        si5351_clock_enable(SI5351_CLK0, 0);
        si5351_clock_enable(SI5351_CLK1, 0);
        return;
    }

    if (CFG_GetParam(CFG_PARAM_F_LO_DIV_BY_TWO))
    { //Quadrature mixer is used
        if (fhz > (BAND_FMAX/2)) //Measurement is performed on 3rd harmonics of LO (-10 dB signals)
        {
            si5351_set_freq(fhz, SI5351_CLK0);
            si5351_set_freq(((fhz + IF) * 2) / 3, SI5351_CLK1);
            BSP_AUDIO_IN_SetVolume(109 - CFG_GetParam(CFG_PARAM_LIN_ATTENUATION)); //Add +10 dB gain against normal
        }
        else
        {
            si5351_set_freq(fhz, SI5351_CLK0);
            si5351_set_freq(((fhz + IF) * 2), SI5351_CLK1);
            BSP_AUDIO_IN_SetVolume(100 - CFG_GetParam(CFG_PARAM_LIN_ATTENUATION)); //Set nominal gain
        }
    }
    else //No need to use 3rd harmonic in this case
    {
        si5351_set_freq(fhz, SI5351_CLK0);
        si5351_set_freq(fhz + IF, SI5351_CLK1);
    }
    si5351_clock_enable(SI5351_CLK0, 1);
    si5351_clock_enable(SI5351_CLK1, 1);

    lastSetFreq = fhz;
    Sleep(2);
}

uint32_t GEN_GetLastFreq()
{
    return lastSetFreq;
}
