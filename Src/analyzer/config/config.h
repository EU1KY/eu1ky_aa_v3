#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

#define AAVERSION "3.0d"

//If quadrature mixer is used (RF2713), define F_LO_DIVIDED_BY_TWO
//If two mixers are used without quadrature (e.g. two of AD8342), comment it out
#define F_LO_DIVIDED_BY_TWO

//Custom XTAL frequency (in Hz) of Si5351a can be set here. Uncomment and edit if
//you are using not a 27 MHz crystal with it. Enter your crystal frequency in Hz.
//#define SI5351_XTAL_FREQ                    27000000

//Uncomment if your Si5351a has alternative I2C address (there was a batch of
//defected chips sold through Mouser, they have this address instead of documented)
//#define SI5351_BUS_BASE_ADDR 0xCE

//Frequency range of the analyzer
#ifdef F_LO_DIVIDED_BY_TWO
    #define BAND_FMAX 75000000ul  //BAND_FMAX must be multiple of 100000
#else
    #define BAND_FMAX 150000000ul //BAND_FMAX must be multiple of 100000
#endif

#define BAND_FMIN 100000ul   //BAND_FMIN must be 100000

#if (BAND_FMAX % 100000) != 0 || BAND_FMAX < BAND_FMIN || BAND_FMIN != 100000
    #error "Incorrect band limit settings"
#endif

typedef enum
{
    CFG_PARAM_PAN_F1,   //Initial frequency for panoramic window
    CFG_PARAM_PAN_SPAN, //Span for panoramic window
    CFG_PARAM_MEAS_F,   //Measurement window frequency

    //---------------------
    CFG_NUM_PARAMS
} CFG_PARAM_t;

void CFG_Init(void);
uint32_t CFG_GetParam(CFG_PARAM_t param);
void CFG_SetParam(CFG_PARAM_t param, uint32_t value);
void CFG_Flush(void);

#endif // _CONFIG_H_
