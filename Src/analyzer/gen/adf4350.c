#if defined (DEBUG)
#    include <math.h>
#endif
#include "adf4350.h"
#include "si5351.h"
#include "custom_spi2.h"
#include "rational.h"
#include "stm32746g_discovery.h"

//Input clocks to 2x ADF4350's are taken from CLK2 output of Si5351a
//ADF4350 interface CLK pins are connected to SPI2 SCK
//ADF4350 interface DATA pins are connected to SPI2 MOSI
//ADF4350 LE signals are connected to SPI2_SLAVE_0 (F0 generator) and SPI2_SLAVE_1 (F1 generator)
//ADF4350 CS signals are connected to logic 1

#define ADF4350_MIN_OUT_FREQ 137500000ul  //137.5 MHz
#define ADF4350_MAX_OUT_FREQ 4294000000ul //4294 MHz (to fit into uint32_t)

#define ADF4350_REF_CLK 27000000ul
#define ADF4350_FPFD 100000ul
#define ADF4350_R_VALUE (ADF4350_REF_CLK / ADF4350_FPFD)
#define ADF4350_FVCO_MIN 2200000000ull //2.2 GHz
#define ADF4350_FVCO_MAX 4400000000ull //4.4 GHz

extern void Sleep(uint32_t);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"

static void adf4350_calc_div(uint32_t fhz, uint32_t* rfdiv_out, uint32_t* div_int, uint32_t* div_frac, uint32_t* div_mod)
{
    //Check fhz valid value (140 MHz ... 4200 MHz)
    assert_param(fhz >= ADF4350_MIN_OUT_FREQ && fhz <= ADF4350_MAX_OUT_FREQ);
    //Determine RF divider value
    uint32_t rfd = (uint32_t)(ADF4350_FVCO_MAX / (uint64_t)fhz);
    uint32_t mask = 16;
    *rfdiv_out = 4;
    while (mask)
    {
        if (rfd & mask) //Find the most significant 1 bit in the rfd
        {
            rfd = mask;
            break;
        }
        mask >>= 1;
        *rfdiv_out--;
    }

    // rfd is 16, 8, 4, 2 or 1 at this point
    // *rfdiv_out is 4, 3, 2, 1 or 0 respectively at this point - the value to be placed to ADF4350 register R4

    uint32_t numerator;
    uint32_t denominator;
    rational_best_approximation((uint64_t)fhz * rfd, ADF4350_FPFD, 0xFFFFFFFF, 4095, &numerator, &denominator);

    *div_int = numerator / denominator;
    *div_frac = numerator % denominator;
    *div_mod = denominator;

    #if defined (DEBUG)
    //Verify result:
    uint32_t actual_f = (uint32_t)(round(((double)ADF4350_FPFD * ((double)*div_int + (double)*div_frac / (double)*div_mod)) / (double)rfd));
    uint32_t diff = (actual_f > fhz) ? (actual_f - fhz) : (fhz - actual_f);
    assert_param(diff < 30);
    #endif
}

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
    Sleep(0);
}


/**
    @brief Set ADF4350 R4 value
    @param RF_Out_en     0 to disable RF output, 1 to enable
    @param DBB           3-bit RF divider select
    @param slave         SPI2 slave identificator
*/
static void adf4350_SendR4(uint32_t RF_Out_en, int32_t DBB, SPI2_Slave_t slave)
{
    uint32_t dw = 0;
    dw |=  (0x03 << 3); //DB4:3 - output power
    dw |=  ((RF_Out_en & 0x01) << 5);    //DB5 - RF output enable
    dw |=  (0x03 << 6); //DB6:7 - AUX output power
    dw |=  (0 << 8);    //DB8 - AUX output enable
    dw |=  (0 << 9);    //DB9 - AUX output select
    dw |=  (1 << 10);   //DB10 - Mute till lock detect
    dw |=  (0 << 11);   //DB11 - VCO powered down (0 to power up)
    dw |=  (1 << 12);   //DB19:12 - 8-bit band select clock divider (R) value - using hardcoded 1 for Fpfd <= 125 kHz
    dw |=  ((DBB & 0x07) << 20);   //DB22:20 - 3-bit RF divider select
    adf4350_SendDW(dw | 0x04, slave); //Write register 4
}


/**
    @brief Set ADF4350 R3 valueband select clock divider
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
    @param slave         SPI2 slave identificator
*/
static void adf4350_SendR1(uint32_t mod, SPI2_Slave_t slave)
{
    uint32_t dw = 0;
    dw |= ((mod & 0xFFF) << 3);
    //Phase value (DBR) is ignored, always setting it to recommended 1 in this application
    dw |= (1 << 15);
    dw |= (1 << 27); // 8/9 prescaler is in use. It limits lowest INT value to 75
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
    assert_param(integ >= 75 && integ <= 65535);
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
    //Si5351 CLK2 is used as 27 MHz clock for both ADF4350 synthesizers
    //Note that Si5351 frequency precision should be calibrated separately
    si5351_Init();
    si5351_SetF2(ADF4350_REF_CLK);

    //Initialize ADF4350 chips here
    adf4350_SendDW(0x00400000ul | 0x05, SPI2_SLAVE_0); //Register 5 : LD pin mode = {0 1}
    adf4350_SendR4(0, 0, SPI2_SLAVE_0);
    adf4350_SendR3(0, 0, 0, SPI2_SLAVE_0);
    adf4350_SendR2(1, 1, SPI2_SLAVE_0);
    adf4350_SendR1(0, SPI2_SLAVE_0);
    adf4350_SendR0(75, 0, SPI2_SLAVE_0);

    adf4350_SendDW(0x00400000ul | 0x05, SPI2_SLAVE_1); //Register 5 : LD pin mode = {0 1}
    adf4350_SendR4(0, 0, SPI2_SLAVE_1);
    adf4350_SendR3(0, 0, 0, SPI2_SLAVE_1);
    adf4350_SendR2(1, 1, SPI2_SLAVE_1);
    adf4350_SendR1(0, SPI2_SLAVE_1);
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
    uint32_t rf_div, div_int, div_frac, div_mod;
    adf4350_calc_div(fhz, &rf_div, &div_int, &div_frac, &div_mod);
    adf4350_SendR4(1, rf_div, SPI2_SLAVE_0);
    adf4350_SendR2(0, ADF4350_R_VALUE, SPI2_SLAVE_0);
    adf4350_SendR1(div_mod, SPI2_SLAVE_0);
    adf4350_SendR0(div_int, div_frac, SPI2_SLAVE_0);
    Sleep(2);
}


/**
    @brief Set given frequency at the output of ADF4350 generating LO
    @param fhz     Desired frequency in Hz
*/
void adf4350_SetLO(uint32_t fhz)
{
    uint32_t rf_div, div_int, div_frac, div_mod;
    adf4350_calc_div(fhz, &rf_div, &div_int, &div_frac, &div_mod);
    adf4350_SendR4(1, rf_div, SPI2_SLAVE_1);
    adf4350_SendR2(0, ADF4350_R_VALUE, SPI2_SLAVE_1);
    adf4350_SendR1(div_mod, SPI2_SLAVE_1);
    adf4350_SendR0(div_int, div_frac, SPI2_SLAVE_1);
    Sleep(2);
}
