#include <math.h>
#include <float.h>
#include <string.h>
#include "oslfile.h"
#include "ff.h"
#include "crash.h"
#include "config.h"
#include "font.h"
#include "LCD.h"
#include "gen.h"
#include "dsp.h"

#define MAX_OSLFILES 16
#define OSL_BASE_R0 50.0f // Note: all OSL calibration coefficients are calculated using G based on 50 Ohms, not on CFG_PARAM_R0 !!!

extern void Sleep(uint32_t);

static float _nonz(float f) __attribute__((unused));
static float complex _cnonz(float complex f) __attribute__((unused));

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

//Hardware error correction sctucture
typedef struct
{
    float mag0;    //Magnitude ratio correction coefficient
    float phase0;  //Phase correction value
} OSL_ERRCORR;

typedef enum
{
    OSL_FILE_EMPTY =  0x00,
    OSL_FILE_VALID =  0x01,
    OSL_FILE_SCANNED_SHORT = 0x02,
    OSL_FILE_SCANNED_LOAD = 0x04,
    OSL_FILE_SCANNED_OPEN = 0x08,
    OSL_FILE_SCANNED_ALL = OSL_FILE_SCANNED_SHORT | OSL_FILE_SCANNED_LOAD | OSL_FILE_SCANNED_OPEN
} OSL_FILE_STATUS;

#define OSL_SCAN_STEP           (100000)
#define OSL_TABLES_IN_SDRAM     (0)

#define OSL_ENTRIES ((MAX_BAND_FREQ - (BAND_FMIN)) / OSL_SCAN_STEP + 1)
#define OSL_NUM_VALID_ENTRIES ((CFG_GetParam(CFG_PARAM_BAND_FMAX) - (BAND_FMIN)) / OSL_SCAN_STEP + 1)

#if (1 == OSL_TABLES_IN_SDRAM)
#define MEMATTR_OSL __attribute__((section (".user_sdram")))
#else
#define MEMATTR_OSL
#endif

static OSL_FILE_STATUS osl_file_status = OSL_FILE_EMPTY;
static S_OSLDATA MEMATTR_OSL osl_data[OSL_ENTRIES] = { 0 };
static OSL_ERRCORR MEMATTR_OSL osl_errCorr[OSL_ENTRIES] = { 0 };
static int32_t osl_file_loaded = -1;
static int32_t osl_err_loaded = 0;

static const COMPLEX cmplus1 = 1.0f + 0.0fi;
static const COMPLEX cmminus1 = -1.0f + 0.0fi;

static int32_t OSL_LoadFromFile(void);

static uint32_t OSL_GetCalFreqByIdx(int32_t idx)
{
    if (idx < 0 || idx >= OSL_NUM_VALID_ENTRIES)
        return 0;
    return BAND_FMIN + idx * OSL_SCAN_STEP;
}

//Fix by OM0IM: now returns floor instead of round in order to linearly interpolate
//HW calibration in OSL_CorrectErr(). This improves precision on low frequencies.
static int GetIndexForFreq(uint32_t fhz)
{
    int idx = -1;
    if (fhz < BAND_FMIN)
        return idx;
    if (fhz <= CFG_GetParam(CFG_PARAM_BAND_FMAX))
    {
        //idx = (int)roundf((float)fhz / OSL_SCAN_STEP) - BAND_FMIN / OSL_SCAN_STEP;
        idx = (int)(fhz / OSL_SCAN_STEP) - BAND_FMIN / OSL_SCAN_STEP;
        return idx;
    }
    return idx;
}

int32_t OSL_IsErrCorrLoaded(void)
{
    return osl_err_loaded;
}

void OSL_LoadErrCorr(void)
{
    FRESULT res;
    FIL fp;
    TCHAR path[64];

    osl_err_loaded = 0;

    sprintf(path, "%s/errcorr.osl", g_cfg_osldir);
    res = f_open(&fp, path, FA_READ | FA_OPEN_EXISTING);
    if (FR_OK != res)
        return;
    UINT br =  0;
    res = f_read(&fp, osl_errCorr, sizeof(*osl_errCorr) * OSL_NUM_VALID_ENTRIES, &br);
    f_close(&fp);
    if (FR_OK != res || (sizeof(*osl_errCorr) * OSL_NUM_VALID_ENTRIES != br))
        return;
    osl_err_loaded = 1;
}

void OSL_ScanErrCorr(void(*progresscb)(uint32_t))
{
    uint32_t i;
    for (i = 0; i < OSL_NUM_VALID_ENTRIES; i++)
    {
        uint32_t freq = OSL_GetCalFreqByIdx(i);
        GEN_SetMeasurementFreq(freq);
        DSP_Measure(freq, 0, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS));
        if (DSP_MeasuredMagImv() < 1. || DSP_MeasuredMagVmv() < 1.)
        {
            CRASH("No signal");
        }
        osl_errCorr[i].mag0 = 1.0f / DSP_MeasuredDiff();
        osl_errCorr[i].phase0 = DSP_MeasuredPhase();
        if (progresscb)
            progresscb((i * 100) / OSL_NUM_VALID_ENTRIES);
    }
    GEN_SetMeasurementFreq(0);
    //Store to file
    FRESULT res;
    FIL fp;
    TCHAR path[64];
    sprintf(path, "%s/errcorr.osl", g_cfg_osldir);
    f_mkdir(g_cfg_osldir);
    res = f_open(&fp, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (FR_OK != res)
        CRASHF("Failed to open file %s for write: error %d", path, res);
    UINT bw;
    res = f_write(&fp, osl_errCorr, sizeof(*osl_errCorr) * OSL_NUM_VALID_ENTRIES, &bw);
    if (FR_OK != res || bw != sizeof(*osl_errCorr) * OSL_NUM_VALID_ENTRIES)
        CRASHF("Failed to write file %s: error %d", path, res);
    f_close(&fp);
    osl_err_loaded = 1;
}

//Linear interpolation added by OM0IM
void OSL_CorrectErr(uint32_t fhz, float *magdif, float *phdif)
{
    if (!osl_err_loaded)
        return;
    int idx = GetIndexForFreq(fhz);
    if (-1 == idx)
        return;
    float correct;
    if (fhz == CFG_GetParam(CFG_PARAM_BAND_FMAX))
        correct = 0.0f;
    else
    {
        correct = osl_errCorr[idx + 1].mag0;
        correct = correct - osl_errCorr[idx].mag0;
    }
    correct = osl_errCorr[idx].mag0 + (correct * (((float)fhz / OSL_SCAN_STEP) - (fhz / OSL_SCAN_STEP)));
    *magdif *= correct;

    if (fhz == CFG_GetParam(CFG_PARAM_BAND_FMAX))
        correct = 0.0f;
    else
    {
        correct = osl_errCorr[idx + 1].phase0;
        correct = correct - osl_errCorr[idx].phase0;
    }
    correct = osl_errCorr[idx].phase0 + (correct * (((float)fhz / OSL_SCAN_STEP) - (fhz / OSL_SCAN_STEP)));
    *phdif -= correct;
}

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
    div = _cnonz(div);
    pResult[0] = Determinant3(b1, a12, a13, b2, a22, a23, b3, a32, a33) / div;
    pResult[1] = Determinant3(a11, b1, a13, a21, b2, a23, a31, b3, a33) / div;
    pResult[2] = Determinant3(a11, a12, b1, a21, a22, b2, a31, a32, b3) / div;
}

//Ensure f is nonzero (to be safely used as denominator)
static float _nonz(float f)
{
    if (0.f == f || -0.f == f)
        return 1e-30; //Small, but much larger than __FLT_MIN__ to avoid INF result
    return f;
}

static float complex _cnonz(float complex f)
{
    if (0.f == cabsf(f))
        return 1e-30 + 0.fi; //Small, but much larger than __FLT_MIN__ to avoid INF result
    return f;
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
float complex OSL_ParabolicInterpolation(float complex y1, float complex y2, float complex y3, //values for frequencies x1, x2, x3
                               float x1, float x2, float x3,       //frequencies of respective y values
                               float x) //Frequency between x2 and x3 where we want to interpolate result
{
    float complex a = ((y3-y2)/(x3-x2)-(y2-y1)/(x2-x1))/(x3-x1);
    float complex b = ((y3-y2)/(x3-x2)*(x2-x1)+(y2-y1)/(x2-x1)*(x3-x2))/(x3-x1);
    float complex res = a * powf(x - x2, 2.) + b * (x - x2) + y2;
    return res;
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
    static char fn[2];
    if (OSL_GetSelected() < 0 || OSL_GetSelected() >= MAX_OSLFILES)
        return "None";
    fn[0] = (char)(OSL_GetSelected() + (int32_t)'A');
    fn[1] = '\0';
    return fn;
}

void OSL_Select(int32_t index)
{
    if (index >= 0 && index < MAX_OSLFILES)
        CFG_SetParam(CFG_PARAM_OSL_SELECTED, index);
    else
        CFG_SetParam(CFG_PARAM_OSL_SELECTED, ~0);
    CFG_Flush();
    OSL_LoadFromFile();
}

void OSL_ScanShort(void(*progresscb)(uint32_t))
{
    if (OSL_GetSelected() < 0)
    {
        CRASH("OSL_ScanShort called without OSL file selected");
    }

    osl_file_status &= ~OSL_FILE_VALID;

    int i;
    for (i = 0; i < OSL_NUM_VALID_ENTRIES; i++)
    {
        uint32_t oslCalFreqHz = OSL_GetCalFreqByIdx(i);
        if (oslCalFreqHz == 0)
            break;
        if (i == 0)
            DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS)); //First run is fake to let the filter stabilize

        DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS));

        COMPLEX rx = DSP_MeasuredZ();
        COMPLEX gamma = OSL_GFromZ(rx, OSL_BASE_R0);

        osl_data[i].gshort = gamma;
        if (progresscb)
            progresscb((i * 100) / OSL_NUM_VALID_ENTRIES);
    }
    GEN_SetMeasurementFreq(0);
    osl_file_status |= OSL_FILE_SCANNED_SHORT;
}

void OSL_ScanLoad(void(*progresscb)(uint32_t))
{
    if (OSL_GetSelected() < 0)
    {
        CRASH("OSL_ScanLoad called without OSL file selected");
    }

    osl_file_status &= ~OSL_FILE_VALID;

    int i;
    for (i = 0; i < OSL_NUM_VALID_ENTRIES; i++)
    {
        uint32_t oslCalFreqHz = OSL_GetCalFreqByIdx(i);
        if (oslCalFreqHz == 0)
            break;
        if (i == 0)
            DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS)); //First run is fake to let the filter stabilize

        DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS));

        COMPLEX rx = DSP_MeasuredZ();
        COMPLEX gamma = OSL_GFromZ(rx, OSL_BASE_R0);

        osl_data[i].gload = gamma;
        if (progresscb)
            progresscb((i * 100) / OSL_NUM_VALID_ENTRIES);
    }
    GEN_SetMeasurementFreq(0);
    osl_file_status |= OSL_FILE_SCANNED_LOAD;
}

void OSL_ScanOpen(void(*progresscb)(uint32_t))
{
    if (OSL_GetSelected() < 0)
    {
        CRASH("OSL_ScanOpen called without OSL file selected");
    }

    osl_file_status &= ~OSL_FILE_VALID;

    int i;
    for (i = 0; i < OSL_NUM_VALID_ENTRIES; i++)
    {
        uint32_t oslCalFreqHz = OSL_GetCalFreqByIdx(i);
        if (oslCalFreqHz == 0)
            break;
        if (i == 0)
            DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS)); //First run is fake to let the filter stabilize

        DSP_Measure(oslCalFreqHz, 1, 0, CFG_GetParam(CFG_PARAM_OSL_NSCANS));

        COMPLEX rx = DSP_MeasuredZ();
        COMPLEX gamma = OSL_GFromZ(rx, OSL_BASE_R0);

        osl_data[i].gopen = gamma;
        if (progresscb)
            progresscb((i * 100) / OSL_NUM_VALID_ENTRIES);
    }
    GEN_SetMeasurementFreq(0);
    osl_file_status |= OSL_FILE_SCANNED_OPEN;
}

static void OSL_StoreFile(void)
{
    FRESULT res;
    FIL fp;
    TCHAR path[64];

    if ((-1 == OSL_GetSelected()) || (osl_file_status != OSL_FILE_VALID))
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
        osl_file_loaded = -1;
        return -1;
    }

    osl_file_loaded = OSL_GetSelected(); //It's OK here, at least we tried to load it

    sprintf(path, "%s/%s.osl", g_cfg_osldir, OSL_GetSelectedName());
    res = f_open(&fp, path, FA_READ | FA_OPEN_EXISTING);
    if (FR_OK != res)
    {
        osl_file_status = OSL_FILE_EMPTY;
        return 1;
    }
    UINT br =  0;
    res = f_read(&fp, osl_data, sizeof(*osl_data) * OSL_NUM_VALID_ENTRIES, &br);
    f_close(&fp);
    if (FR_OK != res || (sizeof(*osl_data) * OSL_NUM_VALID_ENTRIES != br))
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
    for (i = 0; i < OSL_NUM_VALID_ENTRIES; i++)
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
    osl_file_status = OSL_FILE_VALID;
    osl_file_loaded = OSL_GetSelected();
    OSL_StoreFile();
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
    float dg = _nonz(powf((1.0f - crealf(G)), 2) + gi2);

    float r = Rbase * (1.0f - gr2 - gi2) / dg;
    if (r < 0.0f) //Sometimes it overshoots a little due to limited calculation accuracy
        r = 0.0f;
    float x = Rbase * 2.0f * cimagf(G) / dg;
    return r + x * I;
}

//Correct measured G (vs OSL_BASE_R0) using selected OSL calibration file
static float complex OSL_CorrectG(uint32_t fhz, float complex gMeasured)
{
    if (fhz < BAND_FMIN || fhz > CFG_GetParam(CFG_PARAM_BAND_FMAX)) //We can't do anything with frequencies beyond the range
    {
        return gMeasured;
    }

    if (OSL_GetSelected() >= 0 && osl_file_loaded != OSL_GetSelected()) //Reload OSL file if needed
        OSL_LoadFromFile();

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

        oslData.e00 = OSL_ParabolicInterpolation(osl_data[i-1].e00, osl_data[i].e00, osl_data[i+1].e00,
                                             f1, f2, f3, (float)(fhz));
        oslData.e11 = OSL_ParabolicInterpolation(osl_data[i-1].e11, osl_data[i].e11, osl_data[i+1].e11,
                                             f1, f2, f3, (float)(fhz));
        oslData.de = OSL_ParabolicInterpolation(osl_data[i-1].de, osl_data[i].de, osl_data[i+1].de,
                                             f1, f2, f3, (float)(fhz));

    }
    //At this point oslData contains correction structure for given frequency fhz

    COMPLEX gResult = gMeasured * oslData.e11 - oslData.de; //Denominator
    gResult = (gMeasured - oslData.e00) / _cnonz(gResult);
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

//Convert G to magnitude (-1 .. 1) and angle in degrees (-180 .. 180)
//returning complex just for convenience, with magnitude in its real part, and angle in imaginary
float complex OSL_GtoMA(float complex G)
{
    float mag = cabsf(G);
    if (0.f == fabs(mag)) //Handle special case where atan2 is undefined
        return 0.f + 0.fi;
    float phaseDegrees = cargf(G) * 180. / M_PI;
    return mag + phaseDegrees * I;
}
