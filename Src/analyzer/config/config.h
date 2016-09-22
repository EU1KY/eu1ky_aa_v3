#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

#define AAVERSION "3.0d" //Must be 4 characters

//If quadrature mixer is used (RF2713), define F_LO_DIVIDED_BY_TWO
//If two mixers are used without quadrature (e.g. two of AD8342), comment it out
//#define F_LO_DIVIDED_BY_TWO

//Custom XTAL frequency (in Hz) of Si5351a can be set here. Uncomment and edit if
//you are using not a 27 MHz crystal with it. Enter your crystal frequency in Hz.
//#define SI5351_XTAL_FREQ                    27000000

//Uncomment if your Si5351a has alternative I2C address (there was a batch of
//defected chips sold through Mouser, they have this address instead of documented)
//#define SI5351_BUS_BASE_ADDR 0xCE

//Frequency range of the analyzer
#define BAND_FMIN 500000ul    //BAND_FMIN must be multiple 100000
#define BAND_FMAX 150000000ul //BAND_FMAX must be multiple of 100000

#if (BAND_FMAX % 100000) != 0 || BAND_FMAX < BAND_FMIN || (BAND_FMIN % 100000) != 0
    #error "Incorrect band limit settings"
#endif

typedef enum
{
    CFG_PARAM_VERSION,               //4 characters of version string
    CFG_PARAM_PAN_F1,                //Initial frequency for panoramic window
    CFG_PARAM_PAN_SPAN,              //Span for panoramic window
    CFG_PARAM_MEAS_F,                //Measurement window frequency
    CFG_PARAM_SYNTH_TYPE,            //Synthesizer type used: 0 - Si5351a
    CFG_PARAM_SI5351_XTAL_FREQ,      //Si5351a Xtal frequency, Hz
    CFG_PARAM_SI5351_BUS_BASE_ADDR,  //Si5351a I2C bus base address
    CFG_PARAM_SI5351_CORR,           //Si5351a Xtal correction (signed, int16_t)
    CFG_PARAM_OSL_SELECTED,          //Selected OSL file
    CFG_PARAM_R0,                    //Base R0 for G measurements
    CFG_PARAM_OSL_RLOAD,             //RLOAD for OSL calibration
    CFG_PARAM_OSL_RSHORT,            //RSHORT for OSL calibration
    CFG_PARAM_OSL_ROPEN,             //ROPEN for OSL calibration
    CFG_PARAM_OSL_NSCANS,            //Number of scans to average during OSL
    CFG_PARAM_MEAS_NSCANS,           //Number of scans to average in measurement window
    CFG_PARAM_PAN_NSCANS,            //Number of scans to average in panoramic window
    CFG_PARAM_LIN_ATTENUATION,       //Linear audio input attenuation, dB
    CFG_PARAM_F_LO_DIV_BY_TWO,       //LO frequency is divided by two in quadrature mixer
    CFG_PARAM_GEN_F,                 //Frequency for generator window, Hz
    CFG_PARAM_PAN_CENTER_F,          //Way of setting panoramic window. 0: F0+bandspan, 1: Fcenter +/- Bandspan/2
    CFG_PARAM_BRIDGE_RM,             //Value of measurement resistor in bridge, float32
    CFG_PARAM_BRIDGE_RADD,           //Value of series resistor in bridge, float32
    CFG_PARAM_BRIDGE_RLOAD,          //Value of load resistor in bridge, float32
    CFG_PARAM_COM_PORT,              //Serial (COM) port to be used: COM1 or COM2
    CFG_PARAM_COM_SPEED,             //Serial (COM) port speed, bps
    CFG_PARAM_LOWPWR_TIME,           //Time in milliseconds after which to lower power consumption mode (0 - disabled)

    //---------------------
    CFG_NUM_PARAMS
} CFG_PARAM_t;

const char *g_cfg_osldir;
const char *g_aa_dir;

void CFG_Init(void);
uint32_t CFG_GetParam(CFG_PARAM_t param);
void CFG_SetParam(CFG_PARAM_t param, uint32_t value);
void CFG_Flush(void);
void CFG_ParamWnd(void);

#endif // _CONFIG_H_
