#ifndef _CUSTOM_SPI2_H_
#define _CUSTOM_SPI2_H_

#include <stdint.h>

typedef enum
{
    SPI2_SLAVE_0,
    SPI2_SLAVE_1
} SPI2_Slave_t;

void SPI2_Init(void);

void SPI2_SelectSlave(SPI2_Slave_t slave);

void SPI2_DeselectSlave(void);

void SPI2_Transmit(uint8_t *pDataTx, uint32_t nBytes);

#endif //_CUSTOM_SPI2_H_
