#include <math.h>
#include <string.h>
#include "oslfile.h"
#include "ff.h"
#include "crash.h"
#include "config.h"
#include "font.h"
#include "LCD.h"

#define MAX_OSLFILES 16
#define OSL_BASE_R0 50.0f // Note: all OSL calibration coefficients are calculated using G based on 50 Ohms, not on OSL_BASE_R0 !!!

extern void Sleep(uint32_t);

typedef float complex COMPLEX;

//OSL calibration data structure
typedef union
{
    struct
    {
        float complex e00;    //e00 correction coefficient
        float complex e11;    //e11 correction coefficient
        float complex de;     //delta-e correction coefficient
    };
    struct
    {
        float complex gshort; //measured gamma for short load
        float complex gload;  //measured gamma for 50 Ohm load
        float complex gopen;  //measured gamma for open load
    };
} S_OSLDATA;

typedef enum
{
    OSL_FILE_EMPTY =  0x00,
    OSL_FILE_VALID =  0x01,
    OSL_FILE_SCANNED_SHORT = 0x02,
    OSL_FILE_SCANNED_LOAD = 0x04,
    OSL_FILE_SCANNED_OPEN = 0x08,
    OSL_FILE_SCANNED_ALL = OSL_FILE_SCANNED_SHORT | OSL_FILE_SCANNED_LOAD | OSL_FILE_SCANNED_OPEN
} OSL_FILE_STATUS;

#define OSL_SCAN_STEP 100000

#define OSL_NUM_FILE_ENTRIES (((BAND_FMAX) - (BAND_FMIN)) / OSL_SCAN_STEP + 1)


//==============================================================================
//Temporary stubs

static void DSP_Measure(uint32_t freqHz, int applyErrCorrection, int applyOSL, int nMeasurements)
{
    Sleep(2);
}

static void GEN_SetMeasurementFreq(uint32_t f)
{
}

static COMPLEX DSP_MeasuredZ(void)
{
    return 50.0 + 0.0fi;
}

//==============================================================================





static OSL_FILE_STATUS osl_file_status = OSL_FILE_EMPTY;
static S_OSLDATA osl_data[OSL_NUM_FILE_ENTRIES]; //36 kilobytes

static const COMPLEX cmplus1 = 1.0f + 0.0fi;
static const COMPLEX cmminus1 = -1.0f + 0.0fi;


static int32_t OSL_LoadFromFile(void);

// Function to calculate determinant of 3x3 matrix
// Input: 3x3 matrix [[a, b, c],
//                    [m, n, k],
//                    [u, v, w]]
static COMPLEX Determinant3(const COMPLEX a, const COMPLEX b, const COMPLEX c,
                            const COMPLEX m, const COMPLEX n, const COMPLEX k,
                            const COMPLEX u, const COMPLEX v, const COMPLEX w)
{
    return a * n * w + b * k * u + m * v * c - c * n * u - b * m * w - a * k * v;
}

//Cramer's rule implementation (see Wikipedia article)
//Solves three equations with three unknowns
static void CramersRule(const COMPLEX a11, const COMPLEX a12, const COMPLEX a13, const COMPLEX b1,
                        const COMPLEX a21, const COMPLEX a22, const COMPLEX a23, const COMPLEX b2,
                        const COMPLEX a31, const COMPLEX a32, const COMPLEX a33, const COMPLEX b3,
                        COMPLEX* pResult)
{
    COMPLEX div = Determinant3(a11, a12, a13, a21, a22, a23, a31, a32, a33);
    pResult[0] = Determinant3(b1, a12, a13, b2, a22, a23, b3, a32, a33) / div;
    pResult[1] = Determinant3(a11, b1, a13, a21, b2, a23, a31, b3, a33) / div;
    pResult[2] = Determinant3(a11, a12, b1, a21, a22, b2, a31, a32, b3) / div;
}

//Parabolic interpolation
// Let (x1,y1), (x2,y2), and (x3,y3) be the three "nearest" points and (x,y)
// be the "concerned" point, with x2 < x < x3. If (x,y) is to lie on the
// parabola through the three points, you can express y as a quadratic function
// of x in the form:
//    y = a*(x-x2)^2 + b*(x-x2) + y2
// where a and b are:
//    a = ((y3-y2)/(x3-x2)-(y2-y1)/(x2-x1))/(x3-x1)
//    b = ((y3-y2)/(x3-x2)*(x2-x1)+(y2-y1)/(x2-x1)*(x3-x2))/(x3-x1)
static COMPLEX ParabolicInterpolation(COMPLEX y1, COMPLEX y2, COMPLEX y3, //values for frequencies x1, x2, x3
                               float x1, float x2, float x3,       //frequencies of respective y values
                               float x) //Frequency between x2 and x3 where we want to interpolate result
{
    COMPLEX a = ((y3-y2)/(x3-x2)-(y2-y1)/(x2-x1))/(x3-x1);
    COMPLEX b = ((y3-y2)/(x3-x2)*(x2-x1)+(y2-y1)/(x2-x1)*(x3-x2))/(x3-x1);
    COMPLEX res = a * powf(x - x2, 2.) + b * (x - x2) + y2;
    return res;
}

static uint32_t GetCalFreqByIdx(int32_t idx)
{
    if (idx < 0 || idx >= OSL_NUM_FILE_ENTRIES)
        return 0;
    return BAND_FMIN + idx * OSL_SCAN_STEP;
}

int32_t OSL_GetSelected(void)
{
    return (int32_t)CFG_GetParam(CFG_PARAM_OSL_SELECTED);
}

int32_t OSL_IsSelectedValid(void)
{
    if (-1 == OSL_GetSelected() || osl_file_status != OSL_FILE_VALID)
        return 0;
    return 1;
}

const char* OSL_GetSelectedName(void)
{
    static char* fn = " ";
    if (OSL_GetSelected() < 0 || OSL_GetSelected() >= MAX_OSLFILES)
        return "None";
    fn[0] = (char)(OSL_GetSelected() + (int32_t)"A");
    fn[1] = '\0';
    return fn;
}

void OSL_Select(int32_t index)
{
    if (index != OSL_GetSelected())
    {
        if (index >= 0 && index < MAX_OSLFILES)
            CFG_SetParam(CFG_PARAM_OSL_SELECTED, index);
        else
            CFG_SetParam(CFG_PARAM_OSL_SELECTED, ~0);
        CFG_Flush();
        OSL_LoadFromFile();
    }
}

void OSL_ScanShort(void)
{
    if (OSL_GetSelected() < 0)
    {
        CRASH("OSL_ScanShort called without OSL file selected");
    }

    osl_file_status &= ~OSL_FILE_VALID;

    int i;
    for (i = 0; i < OSL_NUM_FILE_ENTRIES; i++)
    {
        uint32_t oslCalFreqHz = GetCalFreqByIdx(i);
        if (oslCalFreqHz == 0)
            break;
        if (i == 0)
            DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS)); //First run is fake to let the filter stabilize

        FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
        FONT_Print(FONT_FRANBIG, LCD_PURPLE, LCD_BLACK, 0, 100, "%d kHz", oslCalFreqHz/1000);

        DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS));

        COMPLEX rx = DSP_MeasuredZ();
        COMPLEX gamma = OSL_GFromZ(rx, OSL_BASE_R0);

        osl_data[i].gshort = gamma;
    }
    GEN_SetMeasurementFreq(0);
    osl_file_status |= OSL_FILE_SCANNED_SHORT;
}

void OSL_ScanLoad(void)
{
    if (OSL_GetSelected() < 0)
    {
        CRASH("OSL_ScanLoad called without OSL file selected");
    }

    osl_file_status &= ~OSL_FILE_VALID;

    int i;
    for (i = 0; i < OSL_NUM_FILE_ENTRIES; i++)
    {
        uint32_t oslCalFreqHz = GetCalFreqByIdx(i);
        if (oslCalFreqHz == 0)
            break;
        if (i == 0)
            DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS)); //First run is fake to let the filter stabilize

        FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
        FONT_Print(FONT_FRANBIG, LCD_PURPLE, LCD_BLACK, 0, 100, "%d kHz", oslCalFreqHz / 1000);

        DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS));

        COMPLEX rx = DSP_MeasuredZ();
        COMPLEX gamma = OSL_GFromZ(rx, OSL_BASE_R0);

        osl_data[i].gload = gamma;
    }
    GEN_SetMeasurementFreq(0);
    osl_file_status |= OSL_FILE_SCANNED_LOAD;
}

void OSL_ScanOpen(void)
{
    if (OSL_GetSelected() < 0)
    {
        CRASH("OSL_ScanOpen called without OSL file selected");
    }

    osl_file_status &= ~OSL_FILE_VALID;

    int i;
    for (i = 0; i < OSL_NUM_FILE_ENTRIES; i++)
    {
        uint32_t oslCalFreqHz = GetCalFreqByIdx(i);
        if (oslCalFreqHz == 0)
            break;
        if (i == 0)
            DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS)); //First run is fake to let the filter stabilize

        FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 100);
        FONT_Print(FONT_FRANBIG, LCD_PURPLE, LCD_BLACK, 0, 100, "%d kHz", oslCalFreqHz / 1000);

        DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS));

        COMPLEX rx = DSP_MeasuredZ();
        COMPLEX gamma = OSL_GFromZ(rx, OSL_BASE_R0);

        osl_data[i].gopen = gamma;
    }
    GEN_SetMeasurementFreq(0);
    osl_file_status |= OSL_FILE_SCANNED_OPEN;
}

static void OSL_StoreFile(void)
{
    FRESULT res;
    FIL fp;
    TCHAR path[64];

    if (-1 == OSL_GetSelected())
        return;
    sprintf(path, "%s/%s.osl", g_cfg_osldir, OSL_GetSelectedName());
    f_mkdir(g_cfg_osldir);
    res = f_open(&fp, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (FR_OK != res)
        CRASHF("Failed to open file %s for write: error %d", path, res);
    UINT bw;
    res = f_write(&fp, osl_data, sizeof(osl_data), &bw);
    if (FR_OK != res || bw != sizeof(osl_data))
        CRASHF("Failed to write file %s: error %d", path, res);
    f_close(&fp);
}

static int32_t OSL_LoadFromFile(void)
{
    FRESULT res;
    FIL fp;
    TCHAR path[64];
    if (-1 == OSL_GetSelected())
    {
        osl_file_status = OSL_FILE_EMPTY;
        return -1;
    }
    sprintf(path, "%s/%s.osl", g_cfg_osldir, OSL_GetSelectedName());
    res = f_open(&fp, path, FA_READ | FA_OPEN_EXISTING);
    if (FR_OK != res)
    {
        osl_file_status = OSL_FILE_EMPTY;
        return 1;
    }
    UINT br =  0;
    res = f_read(&fp, osl_data, sizeof(osl_data), &br);
    f_close(&fp);
    if (FR_OK != res || sizeof(osl_data) != br)
    {
        osl_file_status = OSL_FILE_EMPTY;
        return 2;
    }
    osl_file_status = OSL_FILE_VALID;
    return 0;
}

//Calculate OSL correction coefficients and write to flash in place of original values
void OSL_Calculate(void)
{
    if (OSL_GetSelected() < 0)
    {
        CRASH("OSL_Calculate called without OSL file selected");
    }

    if ((osl_file_status & OSL_FILE_SCANNED_ALL) != OSL_FILE_SCANNED_ALL)
    {
        CRASH("OSL_Calculate called without scanning all loads");
    }

    float r = (float)CFG_GetParam(CFG_PARAM_OSL_RLOAD);
    COMPLEX gammaLoad = (r - OSL_BASE_R0) / (r + OSL_BASE_R0) + 0.0fi;
    r = CFG_GetParam(CFG_PARAM_OSL_RSHORT);
    COMPLEX gammaShort = (r - OSL_BASE_R0) / (r + OSL_BASE_R0) + 0.0fi;
    r = CFG_GetParam(CFG_PARAM_OSL_ROPEN);
    COMPLEX gammaOpen = (r - OSL_BASE_R0) / (r + OSL_BASE_R0) + 0.0fi;

    //Calculate calibration coefficients from measured reflection coefficients
    int i;
    for (i = 0; i < OSL_NUM_FILE_ENTRIES; i++)
    {
        S_OSLDATA* pd = &osl_data[i];
        COMPLEX result[3]; //[e00, e11, de]
        COMPLEX a12 = gammaShort * pd->gshort;
        COMPLEX a22 = gammaLoad * pd->gload;
        COMPLEX a32 = gammaOpen * pd->gopen;
        COMPLEX a13 = cmminus1 * gammaShort;
        COMPLEX a23 = cmminus1 * gammaLoad;
        COMPLEX a33 = cmminus1 * gammaOpen;
        CramersRule( cmplus1, a12, a13, pd->gshort,
                     cmplus1, a22, a23, pd->gload,
                     cmplus1, a32, a33, pd->gopen,
                     result);
        pd->e00 = result[0];
        pd->e11 = result[1];
        pd->de = result[2];
    }

    //Now store calculated data to file
    OSL_StoreFile();
    osl_file_status = OSL_FILE_VALID;
}

float complex OSL_GFromZ(float complex Z, float Rbase)
{
    float complex Z0 = Rbase + 0.0 * I;
    float complex G = (Z - Z0) / (Z + Z0);
    if (isnan(crealf(G)) || isnan(cimagf(G)))
    {
        return 0.99999999f+0.0fi;
    }
    return G;
}

float complex OSL_ZFromG(float complex G, float Rbase)
{
    float gr2  = powf(crealf(G), 2);
    float gi2  = powf(cimagf(G), 2);
    float dg = powf((1.0f - crealf(G)), 2) + gi2;
    float r = Rbase * (1.0f - gr2 - gi2) / dg;
    if (r < 0.0f) //Sometimes it overshoots a little due to limited calculation accuracy
        r = 0.0f;
    float x = Rbase * 2.0f * cimagf(G) / dg;
    return r + x * I;
}

//Correct measured G (vs OSL_BASE_R0) using selected OSL calibration file
static float complex OSL_CorrectG(uint32_t fhz, float complex gMeasured)
{
    if (fhz < BAND_FMIN || fhz > BAND_FMAX) //We can't do anything with frequencies beyond the range
    {
        return gMeasured;
    }
    if (OSL_GetSelected() < 0 || !OSL_IsSelectedValid())
    {
        return gMeasured;
    }

    int i;
    S_OSLDATA oslData;
    i = (fhz - BAND_FMIN) / OSL_SCAN_STEP; //Nearest lower OSL file record index
    if (0 == (fhz % OSL_SCAN_STEP)) //We already have exact value for this frequency
        oslData = osl_data[i];
    else if (i == 0)
    {//Corner case. Linearly interpolate two OSL factors for two nearby records
     //(there is no third point for this interval)
        float prop = ((float)(fhz - BAND_FMIN)) / (float)(OSL_SCAN_STEP); //proportion
        oslData.e00 = (osl_data[1].e00 - osl_data[0].e00) * prop + osl_data[0].e00;
        oslData.e11 = (osl_data[1].e11 - osl_data[0].e11) * prop + osl_data[0].e11;
        oslData.de = (osl_data[1].de - osl_data[0].de) * prop + osl_data[0].de;
    }
    else
    {//We have three OSL points near fhz, thus using parabolic interpolation
        float f1, f2, f3;
        f1 = (i - 1) * (float)OSL_SCAN_STEP + (float)(BAND_FMIN);
        f2 = i * (float)OSL_SCAN_STEP + (float)(BAND_FMIN);
        f3 = (i + 1) * (float)OSL_SCAN_STEP + (float)(BAND_FMIN);

        oslData.e00 = ParabolicInterpolation(osl_data[i-1].e00, osl_data[i].e00, osl_data[i+1].e00,
                                             f1, f2, f3, (float)(fhz));
        oslData.e11 = ParabolicInterpolation(osl_data[i-1].e11, osl_data[i].e11, osl_data[i+1].e11,
                                             f1, f2, f3, (float)(fhz));
        oslData.de = ParabolicInterpolation(osl_data[i-1].de, osl_data[i].de, osl_data[i+1].de,
                                             f1, f2, f3, (float)(fhz));

    }
    //At this point oslData contains correction structure for given frequency fhz
    COMPLEX gResult = (gMeasured - oslData.e00) / (gMeasured * oslData.e11 - oslData.de);
    return gResult;
}

float complex OSL_CorrectZ(uint32_t fhz, float complex zMeasured)
{
    COMPLEX g = OSL_GFromZ(zMeasured, OSL_BASE_R0);
    g = OSL_CorrectG(fhz, g);
    if (crealf(g) > 1.0f)
        g = 1.0f + cimagf(g)*I;
    else if (crealf(g) < -1.0f)
        g = -1.0f + cimagf(g)*I;
    if (cimagf(g) > 1.0f)
        g = crealf(g) + 1.0f * I;
    else if (cimagf(g) < -1.0f)
        g = crealf(g) - 1.0f * I;
    g = OSL_ZFromG(g, OSL_BASE_R0);
    return g;
}
