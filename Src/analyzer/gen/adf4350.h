#ifndef _ADF_4350_H_
#define _ADF_4350_H_

#include <stdint.h>

void adf4350_Init(void);
void adf4350_Off(void);
void adf4350_SetF0(uint32_t fhz);
void adf4350_SetLO(uint32_t fhz);

#endif // _ADF_4350_H_
