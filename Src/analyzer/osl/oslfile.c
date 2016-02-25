#include <math.h>
#include <string.h>
#include "oslfile.h"
#include "ff.h"
#include "crash.h"

#define MAX_OSLFILES 16
#define R0 50.0f
#define Z0 R0+0.0fi
#define OSL_SIGNATURE_VALID    0x564C534F  //OSLV
#define OSL_SIGNATURE_PREPARED 0x504C534F  //OSLP


static int32_t osl_selected = -1;

//OSL calibration file header structure
typedef struct
{
    uint32_t signgature;
    uint32_t f_lo;
    uint32_t step_khz;
    uint32_t num_entries;
    float    rshort;
    float    r0;
    float    ropen;
    uint32_t reserved[9];
} OSL_FILE_HDR;

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

#define OSL_DATA_STEP sizeof(S_OSL_DATA);

int32_t OSL_GetSelected(void)
{
    return osl_selected;
}

const char* OSL_GetSelectedName(void)
{
    static char* fn = " ";
    if (osl_selected < 0 || osl_selected >= MAX_OSLFILES)
        return "None";
    fn[0] = (char)(osl_selected + (int)"A");
    fn[1] = '\0';
    return fn;
}

void OSL_Select(int32_t index)
{
    if (index >= 0 && index < MAX_OSLFILES)
        osl_selected = index;
    else
        osl_selected = -1;
}

int32_t OSL_IsSelectedValid(void)
{
    return 0;
}

int32_t OSL_ScanPrepare(float rshort, float rload, float ropen)
{
    return 0;
}

int32_t OSL_ScanShort(void)
{
    return 0;
}

int32_t OSL_ScanOpen(void)
{
    return 0;
}

int32_t OSL_ScanLoad(void)
{
    return 0;
}

int32_t OSL_ScanFinalize(void)
{
    return 0;
}

float complex OSL_GFromZ(float complex Z)
{
    float complex G = (Z - Z0) / (Z + Z0);
    if (isnan(crealf(G)) || isnan(cimagf(G)))
    {
        //DBGPRINT("OSL_GFromZ NaN\n");
        return 0.99999999f+0.0fi;
    }
    return G;
}

float complex OSL_ZFromG(float complex G)
{
    float gr2  = powf(crealf(G), 2);
    float gi2  = powf(cimagf(G), 2);
    float dg = powf((1.0f - crealf(G)), 2) + gi2;
    float r = R0 * (1.0f - gr2 - gi2) / dg;
    if (r < 0.0f) //Sometimes it overshoots a little due to limited calculation accuracy
        r = 0.0f;
    float x = R0 * 2.0f * cimagf(G) / dg;
    return r + x * I;
}
