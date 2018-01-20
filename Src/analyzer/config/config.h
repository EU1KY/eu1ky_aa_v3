#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <stdbool.h>
#include "LCD.h"
#define AAVERSION "3.0d" //Must be 4 characters

#define BAND_FMIN 500000ul    //BAND_FMIN must be multiple 100000
#define MAX_BAND_FREQ  450000000ul

#if (BAND_FMIN % 100000) != 0
    #error "Incorrect band limit settings"
#endif

typedef enum
{
    CFG_SYNTH_SI5351  = 0,
    CFG_SYNTH_ADF4350 = 1,
    CFG_SYNTH_ADF4351 = 2,
    CFG_SYNTH_SI5338A = 3,
} CFG_SYNTH_TYPE_t;

typedef enum
{
    CFG_S1P_TYPE_S_MA = 0,
    CFG_S1P_TYPE_S_RI  = 1
} CFG_S1P_TYPE_t;

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
    CFG_PARAM_3RD_HARMONIC_ENABLED,  //Enable setting frequency on 3rd harmonic (1) above BAND_FMAX, or disabe (0)
    CFG_PARAM_S11_SHOW,              //Show S11 graph in the panoramic window
    CFG_PARAM_S1P_TYPE,              //Type of Touchstone S1P file saved with panoramic screenshot
    CFG_PARAM_SHOW_HIDDEN,           //Show hidden options in configuration menu
    CFG_PARAM_SCREENSHOT_FORMAT,     //If 0, use BMP format for screenshots, otherwise use PNG
    CFG_PARAM_BAND_FMAX,             //Maximum frequency of the device's working band, Hz
    CFG_PARAM_SI5351_MAX_FREQ,       //Maximum frequency that Si5351 can output, Hz (160 MHz by default, but some samples can reliably provide 200 MHz)
    CFG_PARAM_SI5351_CAPS,           //Si5351a crystal capacitors setting
    CFG_PARAM_TDR_VF,                //Velocity factor for TDR, % (1..100)

    //---------------------
    CFG_NUM_PARAMS
} CFG_PARAM_t;

const char *g_cfg_osldir;
const char *g_aa_dir;

extern  uint8_t ColourSelection;
extern  bool FatLines;
extern  uint32_t BackGrColor;
extern  uint32_t CurvColor;
extern  uint32_t TextColor;
extern  uint32_t Color1;
extern  uint32_t Color2;
extern  uint32_t Color3;
extern  uint32_t Color4;
extern void SetColours();

void CFG_Init(void);
uint32_t CFG_GetParam(CFG_PARAM_t param);
void CFG_SetParam(CFG_PARAM_t param, uint32_t value);
void CFG_Flush(void);
void CFG_ParamWnd(void);

#endif // _CONFIG_H_
