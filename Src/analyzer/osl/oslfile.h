#ifndef _OSLFILE_H_
#define _OSLFILE_H_

#include <complex.h>
#include <stdint.h>

const float OSL_RLOAD;
float OSL_RSHORT;   //can be changed via configuration menu
float OSL_ROPEN;  //can be changed via configuration menu

float complex OSL_ZFromG(float complex G);
float complex OSL_GFromZ(float complex Z);
float complex OSL_CorrectZ(uint32_t fhz, float complex zMeasured);
float complex OSL_CorrectG(uint32_t fhz, float complex gMeasured);

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
