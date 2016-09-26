#include "adf4350.h"
#include "si5351.h"
#include "custom_spi2.h"

//Input clocks to 2x ADF4350's are taken from CLK2 output of Si5351a
//ADF4350 interface CLK pins are connected to SPI2 SCK
//ADF4350 interface DATA pins are connected to SPI2 MOSI
//ADF4350 LE signals are connected to SPI2_SLAVE_0 (F0 generator) and SPI2_SLAVE_1 (F1 generator)
//ADF4350 CS signals are connected to logic 1

void adf4350_Init(void)
{
    //Si5351 CLK2 is used as 27 MHz clock for both ADF4350 synthesizers
    si5351_Init();
    si5351_SetF2(27000000ul);

    //TODO: initialize ADF4350 chips here

}

void adf4350_Off(void)
{
    //TODO : implement shutting down generator of both ADF4350 chips
}

void adf4350_SetF0(uint32_t fhz)
{
    //TODO : implement setting frequency in F0 generator
}


void adf4350_SetLO(uint32_t fhz)
{
    //TODO : implement setting frequency in LO generator
}
