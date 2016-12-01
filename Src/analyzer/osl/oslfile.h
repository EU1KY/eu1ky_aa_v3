#ifndef _OSLFILE_H_
#define _OSLFILE_H_

#include <complex.h>
#include <stdint.h>

float complex OSL_GFromZ(float complex Z, float Rbase);
float complex OSL_ZFromG(float complex Z, float Rbase);
float complex OSL_CorrectZ(uint32_t fhz, float complex zMeasured);
float complex OSL_GtoMA(float complex G);
float complex OSL_ParabolicInterpolation(float complex y1, float complex y2, float complex y3, //values for frequencies x1, x2, x3
                               float x1, float x2, float x3,       //frequencies of respective y values
                               float x); //Frequency between x2 and x3 where we want to interpolate result

int32_t OSL_GetSelected(void);
const char* OSL_GetSelectedName(void);
void OSL_Select(int32_t index);
int32_t OSL_IsSelectedValid(void);

void OSL_ScanOpen(void(*progresscb)(uint32_t));
void OSL_ScanShort(void(*progresscb)(uint32_t));
void OSL_ScanLoad(void(*progresscb)(uint32_t));
void OSL_Calculate(void);
void OSL_LoadErrCorr(void);
void OSL_ScanErrCorr(void(*progresscb)(uint32_t));
void OSL_CorrectErr(uint32_t fhz, float *magdif, float *phdif);
int32_t OSL_IsErrCorrLoaded(void);

#endif //_OSLFILE_H_
