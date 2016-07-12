/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef DSP_H_INCLUDED
#define DSP_H_INCLUDED

#include <stdint.h>
#include <complex.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef float complex DSP_RX;

extern int g_favor_precision;

void DSP_Init(void);

void DSP_Measure(uint32_t freqHz, int applyErrCorr, int applyOSL, int nMeasurements);

DSP_RX DSP_MeasuredZ(void);
float DSP_MeasuredPhase(void);
float DSP_MeasuredDiff(void);
float DSP_MeasuredDiffdB(void);
float DSP_MeasuredPhaseDeg(void);
float DSP_MeasuredPhase(void);
float DSP_MeasuredMagVmv(void);
float DSP_MeasuredMagImv(void);
float complex DSP_MeasuredMagPhaseV(void);
float complex DSP_MeasuredMagPhaseI(void);

float DSP_CalcVSWR(DSP_RX Z);
uint32_t DSP_GetIF(void);

#ifdef __cplusplus
}
#endif

#endif //DSP_H_INCLUDED
