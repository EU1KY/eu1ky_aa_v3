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
#include "crash.h"

static uint32_t lastSetFreq = 14000000ul;
extern void Sleep(uint32_t);

//Generator driver descriptor
typedef struct
{
    void (*Init)(void);         //Initialize generator
    void (*Off)(void);          //Shutdown generator
    void (*SetF0)(uint32_t);    //Set F0 frequency (i.e. measurement frequency)
    void (*SetLO)(uint32_t);    //Set LO frequency (i.e. F0 + IF)
} GenDrv_t;

static GenDrv_t gen = { 0 }; //Generator driver descriptor

void GEN_Init(void)
{
    //Set pointers to generator driver functions
    //Todo: add other variants using CFG_PARAM_SYNTH_TYPE
    if (CFG_SYNTH_SI5351 == CFG_GetParam(CFG_PARAM_SYNTH_TYPE))
    {
        gen.Init = si5351_Init;
        gen.Off = si5351_Off;
        gen.SetF0 = si5351_SetF0;
        gen.SetLO = si5351_SetLO;
    }
    //Todo: add other generator variants using CFG_PARAM_SYNTH_TYPE
    else
    {
        CRASH("Unexpected frequency synthesizer type in configuration");
    }

    //Initialize generator
    gen.Init();
}

void GEN_SetMeasurementFreq(uint32_t fhz)
{
    uint32_t IF = DSP_GetIF(); //Required IF frequency
    if (fhz == 0)
    {
        gen.Off();
        return;
    }

    if (CFG_GetParam(CFG_PARAM_F_LO_DIV_BY_TWO))
    { //Quadrature mixer is used
        if (fhz > (BAND_FMAX/2)) //Measurement is performed on 3rd harmonics of LO (-10 dB signals)
        {
            gen.SetF0(fhz);
            gen.SetLO(((fhz + IF) * 2) / 3);
            if (CFG_GetParam(CFG_PARAM_LIN_ATTENUATION) > 9)
            {
                BSP_AUDIO_IN_SetVolume(109 - CFG_GetParam(CFG_PARAM_LIN_ATTENUATION)); //Add +10 dB gain against normal
            }
        }
        else
        {
            gen.SetF0(fhz);
            gen.SetLO(((fhz + IF) * 2));
            BSP_AUDIO_IN_SetVolume(100 - CFG_GetParam(CFG_PARAM_LIN_ATTENUATION)); //Set nominal gain
        }
    }
    else //No need to use 3rd harmonic in this case
    {
        gen.SetF0(fhz);
        gen.SetLO(fhz + IF);
    }

    lastSetFreq = fhz;
    Sleep(2);
}

uint32_t GEN_GetLastFreq()
{
    return lastSetFreq;
}
