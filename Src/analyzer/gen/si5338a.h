#ifndef _SI5338A_H_
#define _SI5338A_H_

#include <stdint.h>

//ADF4351 driver API
#ifdef __cplusplus
extern "C" {
#endif
void SI5338A_Init(void);
void SI5338A_Off(void);
void SI5338A_SetF0(uint32_t fhz);
void SI5338A_SetLO(uint32_t fhz);

#ifdef __cplusplus
}
#endif

#endif
