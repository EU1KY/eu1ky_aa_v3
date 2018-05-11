#ifndef _ADF4351_H_
#define _ADF4351_H_

#include <stdint.h>

//ADF4351 driver API
#ifdef __cplusplus
extern "C" {
#endif
void ADF4351_Init(void);
void ADF4351_Off(void);
void ADF4351_SetF0(uint32_t fhz);
void ADF4351_SetLO(uint32_t fhz);

#ifdef __cplusplus
}
#endif

#endif
