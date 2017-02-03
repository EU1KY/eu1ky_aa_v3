/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */
#include "si5351.h"
#include "adf4350.h"
#include "adf4351.h"
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
    else if (CFG_SYNTH_ADF4350 == CFG_GetParam(CFG_PARAM_SYNTH_TYPE))
    {
        gen.Init = adf4350_Init;
        gen.Off = adf4350_Off;
        gen.SetF0 = adf4350_SetF0;
        gen.SetLO = adf4350_SetLO;
    }
    else if (CFG_SYNTH_ADF4351 == CFG_GetParam(CFG_PARAM_SYNTH_TYPE))
    {
        gen.Init = ADF4351_Init;
        gen.Off = ADF4351_Off;
        gen.SetF0 = ADF4351_SetF0;
        gen.SetLO = ADF4351_SetLO;
    }
    else if (CFG_SYNTH_SI5351_SS == CFG_GetParam(CFG_PARAM_SYNTH_TYPE))
    {
        gen.Init = si5351_ss_Init;
        gen.Off = si5351_ss_Off;
        gen.SetF0 = si5351_ss_SetF0;
        gen.SetLO = si5351_ss_SetLO;
    }
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

    if ( (CFG_SYNTH_SI5351 == CFG_GetParam(CFG_PARAM_SYNTH_TYPE) ||
          CFG_SYNTH_SI5351_SS == CFG_GetParam(CFG_PARAM_SYNTH_TYPE) )
         && fhz > 150000000ul )
    {
        gen.SetF0(fhz / 3);
        if (CFG_SYNTH_SI5351_SS == CFG_GetParam(CFG_PARAM_SYNTH_TYPE))
            gen.SetLO((fhz - IF) / 3);
        else
            gen.SetLO((fhz + IF) / 3);
    }
    else
    {
        gen.SetF0(fhz);
        if (CFG_SYNTH_SI5351_SS == CFG_GetParam(CFG_PARAM_SYNTH_TYPE))
            gen.SetLO(fhz - IF); //In order to enable device operation up to 150 MHz: SI5351_SS is unable to work above this freq
        else
            gen.SetLO(fhz + IF);
    }

    lastSetFreq = fhz;
    Sleep(2);
}

uint32_t GEN_GetLastFreq()
{
    return lastSetFreq;
}
