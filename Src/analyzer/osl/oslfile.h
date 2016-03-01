#ifndef _OSLFILE_H_
#define _OSLFILE_H_

#include <complex.h>
#include <stdint.h>

float complex OSL_GFromZ(float complex Z, float Rbase);
float complex OSL_GFromZ(float complex Z, float Rbase);
float complex OSL_CorrectZ(uint32_t fhz, float complex zMeasured);

int32_t OSL_GetSelected(void);
const char* OSL_GetSelectedName(void);
void OSL_Select(int32_t index);
int32_t OSL_IsSelectedValid(void);

void OSL_ScanOpen(void);
void OSL_ScanShort(void);
void OSL_ScanLoad(void);
void OSL_Calculate(void);
void OSL_RecalcLoads(void);


#endif //_OSLFILE_H_
