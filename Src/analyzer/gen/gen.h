#ifndef GEN_H_
#define GEN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void GEN_Init(void);
void GEN_SetMeasurementFreq(uint32_t fhz);
uint32_t GEN_GetLastFreq(void);

#ifdef __cplusplus
}
#endif

#endif
