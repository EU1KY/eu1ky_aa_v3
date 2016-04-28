#include "config.h"
#include "ff.h"
#include "crash.h"
#include <string.h>

static uint32_t g_cfg_array[CFG_NUM_PARAMS] = { 0 };
const char *g_aa_dir = "/aa";
static const char *g_cfg_dir = "/aa/config/";
static const char *g_cfg_fpath = "/aa/config/config.bin";
const char *g_cfg_osldir = "/aa/osl";

typedef enum
{
    CFG_PARAM_T_U8,  //8-bit unsigned
    CFG_PARAM_T_U16, //16-bit unsigned
    CFG_PARAM_T_U32, //32-bit unsigned
    CFG_PARAM_T_S8,  //8-bit signed
    CFG_PARAM_T_S16, //16-bit signed
    CFG_PARAM_T_S32, //32-bit signed
    CFG_PARAM_T_F32, //32-bit float
    CFG_PARAM_T_CH,  //char**[]
} CFG_PARAM_TYPE_t;

#pragma pack(push,1)
typedef struct
{
    CFG_PARAM_t id;
    const char *idstring;
    uint32_t nvalues;
    const int32_t *values;
    const char **strvalues;
    uint32_t type;
    const char *dstring;
} CFG_CHANGEABLE_PARAM_DESCR_t;
#pragma pack(pop)

//Integer array macro
#define CFG_IARR(...) (const int32_t[]){__VA_ARGS__}
//Character array macro
#define CFG_SARR(...) (const char*[]){__VA_ARGS__}

static const CFG_CHANGEABLE_PARAM_DESCR_t cfg_ch_descr_table[] =
{
    {
        .id = CFG_PARAM_SYNTH_TYPE,
        .idstring = "SYNTH_TYPE",
        .nvalues = 1,
        .values = CFG_IARR(   0),
        .strvalues = CFG_SARR("Si5351A"),
        .type = CFG_PARAM_T_U32,
        .dstring = "Frequency synthesizer type used"
    },
    {
        .id = CFG_PARAM_SI5351_XTAL_FREQ,
        .idstring = "SI5351_XTAL_FREQ",
        .nvalues = 2,
        .values = CFG_IARR(25000000ul, 27000000ul),
        .strvalues = 0,
        .type = CFG_PARAM_T_U32,
        .dstring = "Si5351 XTAL frequency, Hz"
    },
    {
        .id = CFG_PARAM_SI5351_BUS_BASE_ADDR,
        .idstring = "SI5351_BUS_BASE_ADDR",
        .nvalues = 2,
        .values = CFG_IARR(   0xC0,   0xCE),
        .strvalues = CFG_SARR("0xC0", "0xCE"),
        .type = CFG_PARAM_T_U8,
        .dstring = "Si5351 i2c bus base address (default 0xC0)"
    },
    {
        .id = CFG_PARAM_SI5351_CORR,
        .idstring = "SI5351_CORR",
        .type = CFG_PARAM_T_S16,
        .dstring = "Si5351 Xtal frequency correction, Hz"
    },
    {
        .id = CFG_PARAM_OSL_SELECTED,
        .idstring = "OSL_SELECTED",
        .nvalues = 17,
        .values = CFG_IARR(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, -1),
        .strvalues = CFG_SARR("A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","None"),
        .type = CFG_PARAM_T_S32,
        .dstring = "Selected OSL file"
    },
    {
        .id = CFG_PARAM_R0,
        .idstring = "Z0",
        .nvalues = 6,
        .values = CFG_IARR(28, 50, 75, 100, 150, 300),
        .type = CFG_PARAM_T_U32,
        .dstring = "Selected base impedance (Z0) for Smith Chart and VSWR"
    },
    {
        .id = CFG_PARAM_OSL_RLOAD,
        .idstring = "OSL_RLOAD",
        .nvalues = 4,
        .values = CFG_IARR(50, 75, 100, 150),
        .type = CFG_PARAM_T_U32,
        .dstring = "LOAD R for OSL calibration, Ohm"
    },
    {
        .id = CFG_PARAM_OSL_RSHORT,
        .idstring = "OSL_RSHORT",
        .nvalues = 3,
        .values = CFG_IARR(0, 5, 10),
        .type = CFG_PARAM_T_U32,
        .dstring = "SHORT R for OSL calibration, Ohm"
    },
    {
        .id = CFG_PARAM_OSL_ROPEN,
        .idstring = "OSL_ROPEN",
        .nvalues = 6,
        .values = CFG_IARR(    300,   333,   500,   750,   1000,   999999),
        .strvalues = CFG_SARR("300", "333", "500", "750", "1000", "Open"),
        .type = CFG_PARAM_T_U32,
        .dstring = "OPEN R for OSL calibration, Ohm"
    },
    {
        .id = CFG_PARAM_OSL_NSCANS,
        .idstring = "OSL_NSCANS",
        .nvalues = 7,
        .values = CFG_IARR(1, 3, 5, 7, 9, 11, 15),
        .type = CFG_PARAM_T_U32,
        .dstring = "Number of scans to average during OSL calibration at each F"
    },
    {
        .id = CFG_PARAM_MEAS_NSCANS,
        .idstring = "MEAS_NSCANS",
        .nvalues = 7,
        .values = CFG_IARR(1, 3, 5, 7, 9, 11, 15),
        .type = CFG_PARAM_T_U32,
        .dstring = "Number of scans to average in measurement window"
    },
    {
        .id = CFG_PARAM_PAN_NSCANS,
        .idstring = "PAN_NSCANS",
        .nvalues = 7,
        .values = CFG_IARR(1, 3, 5, 7, 9, 11, 15),
        .type = CFG_PARAM_T_U32,
        .dstring = "Number of scans to average in panoramic window"
    },
    {
        .id = CFG_PARAM_LIN_ATTENUATION,
        .idstring = "LIN_ATTENUATION",
        .nvalues = 11,
        .values = CFG_IARR(0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30),
        .type = CFG_PARAM_T_U8,
        .dstring = "Linar audio inputs attenuation, dB."
    },
};

static const uint32_t cfg_ch_descr_table_num = sizeof(cfg_ch_descr_table) / sizeof(CFG_CHANGEABLE_PARAM_DESCR_t);

//CFG module initialization
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
    CFG_SetParam(CFG_PARAM_MEAS_NSCANS, 5);
    CFG_SetParam(CFG_PARAM_PAN_NSCANS, 5);
    CFG_SetParam(CFG_PARAM_LIN_ATTENUATION, 15);

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
            //Replace version to current
            CFG_SetParam(CFG_PARAM_VERSION, *(uint32_t*)AAVERSION);
            //And write extended file
            CFG_Flush();
        }
        else
        {
            UINT br;
            f_read(&fo, g_cfg_array, sizeof(g_cfg_array), &br);
            f_close(&fo);
            //Replace version to current
            CFG_SetParam(CFG_PARAM_VERSION, *(uint32_t*)AAVERSION);
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

static uint32_t CFG_GetNextValue(uint32_t param_idx, uint32_t param_value)
{
    if (param_idx >= cfg_ch_descr_table_num)
        return param_value + 1;

    const CFG_CHANGEABLE_PARAM_DESCR_t *pd = &cfg_ch_descr_table[param_idx];
    if (0 == pd->nvalues)
    {//If no default values specified:
        return param_value + 1;
    }

    uint32_t i;
    for (i = 0; i < pd->nvalues; i++)
    {
        if (param_value == pd->values[i])
        {//Value is among the defaults
            if (i == pd->nvalues - 1)
                return pd->values[0]; //Wrap around the last one
            return pd->values[i + 1];
        }
    }
    //Oops, if we get here then the value is not among default ones
    //Just return the last default value
    return pd->values[pd->nvalues - 1];
}

static uint32_t CFG_GetPrevValue(uint32_t param_idx, uint32_t param_value)
{
    if (param_idx >= cfg_ch_descr_table_num)
        return param_value - 1;

    const CFG_CHANGEABLE_PARAM_DESCR_t *pd = &cfg_ch_descr_table[param_idx];
    if (0 == pd->nvalues)
    {//If no default values specified:
        return param_value - 1;
    }

    uint32_t i;
    for (i = 0; i < pd->nvalues; i++)
    {
        if (param_value == pd->values[i])
        {//Value is among the defaults
            if (i == 0)
                return pd->values[pd->nvalues - 1]; //Wrap around 0
            return pd->values[i - 1];
        }
    }
    //Oops, if we get here then the value is not among default ones
    //Just return the first default value
    return pd->values[0];
}

//Get string representation of parameter
const char * CFG_GetStringValue(uint32_t param_idx)
{
    if (param_idx >= cfg_ch_descr_table_num)
        return "";

    const CFG_CHANGEABLE_PARAM_DESCR_t *pd = &cfg_ch_descr_table[param_idx];

    uint32_t uval = CFG_GetParam(pd->id); //Current parameter value
    uint32_t i;

    if (0 != pd->strvalues)
    {
        for (i = 0; i < pd->nvalues; i++)
        {
            if (uval == pd->values[i])
                return pd->strvalues[i]; //Found string for default value
        }
    }

    //Print value to the static buffer and return it
    static char tstr[32];

    switch (pd->type)
    {
        case CFG_PARAM_T_U8:
            sprintf(tstr, "%u", (unsigned int)((uint8_t)uval));
            break;
        case CFG_PARAM_T_U16:
            sprintf(tstr, "%u", (unsigned int)((uint16_t)uval));
            break;
        case CFG_PARAM_T_U32:
            sprintf(tstr, "%u", (unsigned int)((uint32_t)uval));
            break;
        case CFG_PARAM_T_S8:
            sprintf(tstr, "%d", (int)((int8_t)uval));
            break;
        case CFG_PARAM_T_S16:
            sprintf(tstr, "%d", (int)((int16_t)uval));
            break;
        case CFG_PARAM_T_S32:
            sprintf(tstr, "%d", (int)((int32_t)uval));
            break;
        case CFG_PARAM_T_F32:
            sprintf(tstr, "%f", *(float*)&uval);
            break;
        case CFG_PARAM_T_CH:
            memcpy(tstr, &uval, 4);
            tstr[5] = '\0';
            break;
        default:
            return "";
    }
    return tstr;
}

const char * CFG_GetStringDescr(uint32_t param_idx)
{
    if (param_idx >= cfg_ch_descr_table_num)
        return "";
    return cfg_ch_descr_table[param_idx].dstring;
}

const char * CFG_GetStringName(uint32_t param_idx)
{
    if (param_idx >= cfg_ch_descr_table_num)
        return "";
    return cfg_ch_descr_table[param_idx].idstring;
}

//===============================================================================
// Configuration parameters setting window
//===============================================================================
#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "textbox.h"
extern void Sleep(uint32_t);

static uint32_t rqExit = 0;
static uint32_t selected_param = 0;
static TEXTBOX_CTX_t *pctx = 0;
static uint32_t hbNameIdx = 0;
static uint32_t hbDescrIdx = 0;
static uint32_t hbValIdx = 0;

static void _hit_prev(void)
{
    if (--selected_param >= cfg_ch_descr_table_num)
        selected_param = cfg_ch_descr_table_num - 1;

    TEXTBOX_SetText(pctx, hbNameIdx, CFG_GetStringName(selected_param));
    TEXTBOX_SetText(pctx, hbValIdx, CFG_GetStringValue(selected_param));
    TEXTBOX_SetText(pctx, hbDescrIdx, CFG_GetStringDescr(selected_param));
}

static void _hit_next(void)
{
    selected_param++;
    if (selected_param >= cfg_ch_descr_table_num)
        selected_param = 0;
    TEXTBOX_SetText(pctx, hbNameIdx, CFG_GetStringName(selected_param));
    TEXTBOX_SetText(pctx, hbValIdx, CFG_GetStringValue(selected_param));
    TEXTBOX_SetText(pctx, hbDescrIdx, CFG_GetStringDescr(selected_param));
}

static void _hit_save(void)
{
    CFG_Flush();
    rqExit = 1;
}

static void _hit_ex(void)
{
    rqExit = 1;
}

static void _hit_prev_value(void)
{
    uint32_t currentValue = CFG_GetParam(cfg_ch_descr_table[selected_param].id);
    uint32_t prevValue = CFG_GetPrevValue(selected_param, currentValue);
    CFG_SetParam(cfg_ch_descr_table[selected_param].id, prevValue);
    TEXTBOX_SetText(pctx, hbValIdx, CFG_GetStringValue(selected_param));
}

static void _hit_next_value(void)
{
    uint32_t currentValue = CFG_GetParam(cfg_ch_descr_table[selected_param].id);
    uint32_t nextValue = CFG_GetNextValue(selected_param, currentValue);
    CFG_SetParam(cfg_ch_descr_table[selected_param].id, nextValue);
    TEXTBOX_SetText(pctx, hbValIdx, CFG_GetStringValue(selected_param));
}

//Changeable parameters setting window
//TODO!!!
void CFG_ParamWnd(void)
{
    rqExit = 0;
    selected_param = 0;

    LCD_FillAll(LCD_BLACK);

    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 100, 0, "Configuration editor");

    TEXTBOX_t hbPrevParam = {.x0 = 10, .y0 = 34, .text = " < Prev param ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLACK, .bgcolor = LCD_RGB(128,128,128), .cb = _hit_prev };
    TEXTBOX_t hbNextParam = {.x0 = 300, .y0 = 34, .text = " Next param > ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLACK, .bgcolor = LCD_RGB(128,128,128), .cb = _hit_next };
    TEXTBOX_t hbEx = {.x0 = 10, .y0 = 220, .text = " Cancel and exit ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLUE, .bgcolor = LCD_YELLOW, .cb = _hit_ex };
    TEXTBOX_t hbSave = {.x0 = 300, .y0 = 220, .text = " Save and exit ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLUE, .bgcolor = LCD_GREEN, .cb = _hit_save };

    TEXTBOX_t hbParamName = {.x0 = 10, .y0 = 70, .text = "    ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_GREEN, .bgcolor = LCD_BLACK, .cb = 0, .nowait = 1 };
    TEXTBOX_t hbParamDescr = {.x0 = 10, .y0 = 110, .text = "    ", .font = FONT_FRAN,
                            .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK, .cb = 0, .nowait = 1 };
    TEXTBOX_t hbValue = {.x0 = 80, .y0 = 130, .text = "    ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLUE, .bgcolor = LCD_BLACK, .cb = 0, .nowait = 1 };
    TEXTBOX_t hbPrevValue = {.x0 = 10, .y0 = 130, .text = "  <  ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLACK, .bgcolor = LCD_RGB(128,128,128), .cb = _hit_prev_value };
    TEXTBOX_t hbNextValue = {.x0 = 400, .y0 = 130, .text = "  >  ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLACK, .bgcolor = LCD_RGB(128,128,128), .cb = _hit_next_value };

    TEXTBOX_CTX_t ctx = {0};
    pctx = &ctx;

    TEXTBOX_Append(pctx, &hbPrevParam);
    TEXTBOX_Append(pctx, &hbNextParam);
    TEXTBOX_Append(pctx, &hbEx);
    TEXTBOX_Append(pctx, &hbSave);
    TEXTBOX_Append(pctx, &hbPrevValue);
    TEXTBOX_Append(pctx, &hbNextValue);
    hbNameIdx = TEXTBOX_Append(pctx, &hbParamName);
    hbDescrIdx = TEXTBOX_Append(pctx, &hbParamDescr);
    hbValIdx = TEXTBOX_Append(pctx, &hbValue);
    TEXTBOX_DrawContext(&ctx);

    TEXTBOX_SetText(pctx, hbNameIdx, CFG_GetStringName(selected_param));
    TEXTBOX_SetText(pctx, hbValIdx, CFG_GetStringValue(selected_param));
    TEXTBOX_SetText(pctx, hbDescrIdx, CFG_GetStringDescr(selected_param));

    for(;;)
    {
        if (TEXTBOX_HitTest(&ctx))
        {
            if (rqExit)
            {
                CFG_Init();
                return;
            }
            Sleep(50);
        }
    }
}
