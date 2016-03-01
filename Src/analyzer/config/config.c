#include "config.h"
#include "ff.h"
#include "crash.h"

static uint32_t g_cfg_array[CFG_NUM_PARAMS] = { 0 };
const char *g_aa_dir = "/aa";
static const char *g_cfg_dir = "/aa/config/";
static const char *g_cfg_fpath = "/aa/config/config.bin";
const char *g_cfg_osldir = "/aa/osl";

void CFG_Init(void)
{
    //Set defaults for parameters
    CFG_SetParam(CFG_PARAM_VERSION, *(uint32_t*)AAVERSION);
    CFG_SetParam(CFG_PARAM_PAN_F1, 14000000ul);
    CFG_SetParam(CFG_PARAM_PAN_SPAN, 1ul);
    CFG_SetParam(CFG_PARAM_MEAS_F, 14000000ul);
    CFG_SetParam(CFG_PARAM_SYNTH_TYPE, 0);
    CFG_SetParam(CFG_PARAM_SI5351_XTAL_FREQ, 27000000ul);
    CFG_SetParam(CFG_PARAM_SI5351_BUS_BASE_ADDR, 0xC0);
    CFG_SetParam(CFG_PARAM_SI5351_CORR, 0);
    CFG_SetParam(CFG_PARAM_OSL_SELECTED, ~0ul);
    CFG_SetParam(CFG_PARAM_R0, 50);
    CFG_SetParam(CFG_PARAM_OSL_RLOAD, 50);
    CFG_SetParam(CFG_PARAM_OSL_RSHORT, 5);
    CFG_SetParam(CFG_PARAM_OSL_ROPEN, 500);
    CFG_SetParam(CFG_PARAM_OSL_NSCANS, 5);

    //Load params from SD
    FRESULT res;
    FIL fo = { 0 };

    FILINFO finfo;
    res = f_stat(g_cfg_fpath, &finfo);
    if (FR_NOT_ENABLED == res || FR_NOT_READY == res)
        CRASH("No SD card");
    if (FR_OK == res)
    {
        if (0 == finfo.fsize)
        {
            CFG_Flush();
        }
        res = f_open(&fo, g_cfg_fpath, FA_READ);
        if (finfo.fsize < sizeof(g_cfg_array))
        {
            //Read partially
            UINT br;
            f_read(&fo, g_cfg_array, finfo.fsize, &br);
            f_close(&fo);
            //And write extended file
            CFG_Flush();
        }
        else
        {
            UINT br;
            f_read(&fo, g_cfg_array, sizeof(g_cfg_array), &br);
            f_close(&fo);
        }

    }
    else
    {
        res = f_mkdir(g_aa_dir);
        res = f_mkdir(g_cfg_dir);
        CFG_Flush();
        return;
    }
}

uint32_t CFG_GetParam(CFG_PARAM_t param)
{
    assert_param(param >= 0 && param < CFG_NUM_PARAMS);
    return g_cfg_array[param];
}

void CFG_SetParam(CFG_PARAM_t param, uint32_t value)
{
    assert_param(param >= 0 && param < CFG_NUM_PARAMS);
    g_cfg_array[param] = value;
}

void CFG_Flush(void)
{
    FRESULT res;
    FIL fo = { 0 };
    res = f_open(&fo, g_cfg_fpath, FA_OPEN_ALWAYS | FA_WRITE);
    if (FR_OK == res)
    {
        UINT bw;
        CFG_SetParam(CFG_PARAM_VERSION, *(uint32_t*)AAVERSION);
        res = f_write(&fo, g_cfg_array, sizeof(g_cfg_array), &bw);
        res = f_close(&fo);
    }
    else
    {
        CRASHF("CFG_Flush: Failed to open config file. Err = %d", res);
    }
}
