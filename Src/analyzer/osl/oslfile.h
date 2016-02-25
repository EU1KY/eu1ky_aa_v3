#ifndef _OSLFILE_H_
#define _OSLFILE_H_

#include <complex.h>
#include <stdint.h>

float complex OSL_ZFromG(float complex G);
float complex OSL_GFromZ(float complex Z);

int32_t OSL_GetSelected(void);
const char* OSL_GetSelectedName(void);
void OSL_Select(int32_t index);
int32_t OSL_IsSelectedValid(void);

int32_t OSL_ScanPrepare(float rshort, float rload, float ropen);
int32_t OSL_ScanOpen(void);
int32_t OSL_ScanShort(void);
int32_t OSL_ScanLoad(void);
int32_t OSL_ScanFinalize(void);

#endif //_OSLFILE_H_
