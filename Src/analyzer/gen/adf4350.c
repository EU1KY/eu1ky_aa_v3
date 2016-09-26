#include "adf4350.h"
#include "si5351.h"
#include "custom_spi2.h"
#include "rational.h"

//Input clocks to 2x ADF4350's are taken from CLK2 output of Si5351a
//ADF4350 interface CLK pins are connected to SPI2 SCK
//ADF4350 interface DATA pins are connected to SPI2 MOSI
//ADF4350 LE signals are connected to SPI2_SLAVE_0 (F0 generator) and SPI2_SLAVE_1 (F1 generator)
//ADF4350 CS signals are connected to logic 1

#define ADF4350_REF_CLK 27000000ul

//Send 32-bit data to ADF4350
static void adf4350_SendDW(uint32_t dw, SPI2_Slave_t slave)
{
    uint8_t bytes[4];
    //Convert endianness (4 bytes must go out as bit 32 .. bit 0)
    bytes[0] = (uint8_t)((dw >> 24) & 0xFF);
    bytes[1] = (uint8_t)((dw >> 16) & 0xFF);
    bytes[2] = (uint8_t)((dw >> 8) & 0xFF);
    bytes[3] = (uint8_t)(dw & 0xFF);

    SPI2_SelectSlave(slave);
    SPI2_Transmit(bytes, 4);
    SPI2_DeselectSlave();
}

/**
    @brief Set ADF4350 R4 value
    @param RF_Out_en     0 to disable RF output, 1 to enable
    @param RBS           8-bit band select clock divider value
    @param DBB           3-bit RF divider select
    @param slave         SPI2 slave identificator
*/
static void adf4350_SendR4(uint32_t RF_Out_en, uint32_t RBS, uint32_t DBB, SPI2_Slave_t slave)
{
    uint32_t dw = 0;
    dw |=  (0x03 << 3); //DB4:3 - output power
    dw |=  ((RF_Out_en & 0x01) << 5);    //DB5 - RF output enable
    dw |=  (0x03 << 6); //DB6:7 - AUX output power
    dw |=  (0 << 8);    //DB8 - AUX output enable
    dw |=  (0 << 9);    //DB9 - AUX output select
    dw |=  (1 << 10);   //DB10 - Mute till lock detect
    dw |=  (0 << 11);   //DB11 - VCO powered down (0 to power up)
    dw |=  ((RBS & 0xFF) << 12);   //DB19:12 - 8-bit band select clock divider (R) value
    dw |=  ((DBB & 0x07) << 20);   //DB22:20 - 3-bit RF divider select
    adf4350_SendDW(dw | 0x04, slave); //Write register 4
}


/**
    @brief Set ADF4350 R3 value
    @param clkdiv        12-bit clock divider value
    @param clkdivmode    2-bit clock divider mode
    @param csr           1-bit cycle slip reduction
    @param slave         SPI2 slave identificator
*/
static void adf4350_SendR3(uint32_t clkdiv, uint32_t clkdivmode, uint32_t csr, SPI2_Slave_t slave)
{
    uint32_t dw = 0;
    dw |=  ((clkdiv & 0xFFF) << 3);
    dw |=  ((clkdivmode & 0x3) << 15);
    dw |=  ((csr & 0x1) << 18);
    adf4350_SendDW(dw | 0x03, slave);
}

/**
    @brief Set ADF4350 R2 value
    @param pwrdown       1 to power down device, 0 to power up
    @param rcounter      10-bit R counter value (reference divider)
    @param slave         SPI2 slave identificator
*/
static void adf4350_SendR2(uint32_t pwrdown, uint32_t rcounter, SPI2_Slave_t slave)
{
    uint32_t dw = 0;
    dw |=  ((pwrdown & 0x1) << 5);
    dw |=  (1 << 6);                // Phase detector polarity bit
    dw |=  (0x08 << 9);             // Charge pump current setting
    dw |=  ((rcounter & 0x3FF) << 14);
    adf4350_SendDW(dw | 0x02, slave);
}


/**
    @brief Set ADF4350 R1 value
    @param mod           12-bit modulus (MOD) value
    @param presc         1-bit prescaler value (0 - 4/5, 1 - 8/9)
    @param slave         SPI2 slave identificator
*/
static void adf4350_SendR1(uint32_t mod, uint32_t presc, SPI2_Slave_t slave)
{
    uint32_t dw = 0;
    dw |= ((mod & 0xFFF) << 3);
    //Phase value (DBR) is ignored, always setting it to recommended 1 in this application
    dw |= (1 << 15);
    dw |= ((presc & 0x01) << 27);
    adf4350_SendDW(dw | 0x01, slave);
}


/**
    @brief Set ADF4350 R0 value.
    @param integ         16-bit integer value (INT). Must be in the allowed range of
                         23..65535 (with prescaler 4/5) or 75..65535 (with prescaler 8/9).
    @param frac          12-bit fractional value (FRAC)
    @param slave         SPI2 slave identificator
*/
static void adf4350_SendR0(uint32_t integ, uint32_t frac, SPI2_Slave_t slave)
{
    uint32_t dw = 0;
    dw |= ((frac & 0xFFF) << 3);
    dw |= ((integ & 0xFFFF) << 15);
    adf4350_SendDW(dw, slave);
}


//-----------------------------------------------------------------------------------------
// Public API
//-----------------------------------------------------------------------------------------

/**
    @brief Initialize ADF4350 devices
*/
void adf4350_Init(void)
{
    uint32_t tmp;
    uint32_t dw;

    //Si5351 CLK2 is used as 27 MHz clock for both ADF4350 synthesizers
    //Note that Si5351 frequency precision should be calibrated separately
    si5351_Init();
    si5351_SetF2(ADF4350_REF_CLK);

    //Initialize ADF4350 chips here
    adf4350_SendDW(0x00400000ul | 0x05, SPI2_SLAVE_0); //Register 5 : LD pin mode = {0 1}
    adf4350_SendR4(0, 0, 0, SPI2_SLAVE_0);
    adf4350_SendR3(0, 0, 0, SPI2_SLAVE_0);
    adf4350_SendR2(1, 1, SPI2_SLAVE_0);
    adf4350_SendR1(0, 0, SPI2_SLAVE_0);
    adf4350_SendR0(75, 0, SPI2_SLAVE_0);

    adf4350_SendDW(0x00400000ul | 0x05, SPI2_SLAVE_1); //Register 5 : LD pin mode = {0 1}
    adf4350_SendR4(0, 0, 0, SPI2_SLAVE_1);
    adf4350_SendR3(0, 0, 0, SPI2_SLAVE_1);
    adf4350_SendR2(1, 1, SPI2_SLAVE_1);
    adf4350_SendR1(0, 0, SPI2_SLAVE_1);
    adf4350_SendR0(75, 0, SPI2_SLAVE_1);
}

/**
    @brief Turn off RF outputs and turn off clock for both ADF4350 devices
*/
void adf4350_Off(void)
{
    adf4350_SendR2(1, 1, SPI2_SLAVE_0); //Power down F0 generator
    adf4350_SendR2(1, 1, SPI2_SLAVE_1); //Power down LO generator
}

/**
    @brief Set given frequency at the output of ADF4350 generating F0
    @param fhz     Desired frequency in Hz
*/
void adf4350_SetF0(uint32_t fhz)
{
    //TODO : implement setting frequency in F0 generator
    //adf4350_SendR4(0, 0, 0, SPI2_SLAVE_0);
    //adf4350_SendR3(0, 0, 0, SPI2_SLAVE_0);
    //adf4350_SendR2(1, 1, SPI2_SLAVE_0);
    //adf4350_SendR1(0, 0, SPI2_SLAVE_0);
    //adf4350_SendR0(75, 0, SPI2_SLAVE_0);
}


/**
    @brief Set given frequency at the output of ADF4350 generating LO
    @param fhz     Desired frequency in Hz
*/
void adf4350_SetLO(uint32_t fhz)
{
    //TODO : implement setting frequency in LO generator
    //adf4350_SendR4(0, 0, 0, SPI2_SLAVE_1);
    //adf4350_SendR3(0, 0, 0, SPI2_SLAVE_1);
    //adf4350_SendR2(1, 1, SPI2_SLAVE_1);
    //adf4350_SendR1(0, 0, SPI2_SLAVE_1);
    //adf4350_SendR0(75, 0, SPI2_SLAVE_1);
}