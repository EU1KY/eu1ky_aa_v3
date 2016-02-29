#include "config.h"
#include "ff.h"
#include "crash.h"

static uint32_t g_cfg_array[CFG_NUM_PARAMS] = { 0 };
static const TCHAR *g_aa_dir = "aa";
static const TCHAR *g_cfg_dir = "aa/config/";
static const TCHAR *g_cfg_fpath = "/aa/config/config.bin";
const TCHAR *g_cfg_osldir = "/aa/osl";

void CFG_Init(void)
{
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
        }
        else
        {
            UINT br;
            f_read(&fo, g_cfg_array, sizeof(g_cfg_array), &br);
        }
        f_close(&fo);
    }
    else
    {
        res = f_mkdir("/aa");
        res = f_mkdir("/aa/config");
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
    res = f_open(&fo, g_cfg_fpath, FA_OPEN_EXISTING | FA_WRITE);
    if (FR_OK == res)
    {
        UINT bw;
        res = f_write(&fo, g_cfg_array, sizeof(g_cfg_array), &bw);
        res = f_close(&fo);
    }
    else
    {
        CRASHF("CFG_Flush: Failed to open config file. Err = %d", res);
    }
}
