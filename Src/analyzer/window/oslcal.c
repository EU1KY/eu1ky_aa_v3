//OSL calibration window

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "LCD.h"
#include "font.h"
#include "touch.h"
#include "hit.h"
#include "textbox.h"
#include "oslfile.h"

extern void Sleep(uint32_t);

static uint32_t rqExit = 0;
static uint32_t shortScanned = 0;
static uint32_t loadScanned = 0;
static uint32_t openScanned = 0;
static char progresstxt[16];
static int progressval;
static TEXTBOX_t hbScanShort;
static TEXTBOX_t hbScanOpen;
static TEXTBOX_t hbScanLoad;
static TEXTBOX_t hbScanProgress;
static uint32_t hbScanProgressId;
static TEXTBOX_t hbSave;
static TEXTBOX_CTX_t osl_ctx = {0};
static int percents = 0;

static void _hit_ex(void)
{
    if (shortScanned || openScanned || loadScanned)
        OSL_Select(OSL_GetSelected()); //Reload OSL file
    rqExit = 1;
}

static void progress_cb(uint32_t new_percent)
{
    if (new_percent == progressval || new_percent > 100)
        return;
    progressval = new_percent;
    sprintf(progresstxt, "%u%%", (unsigned int)progressval);
    TEXTBOX_SetText(&osl_ctx, hbScanProgressId, progresstxt);
}

static void _hb_scan_short(void)
{
    progressval = 100;
    progress_cb(0);

    hbScanShort.bgcolor = LCD_RGB(128, 128, 0);
    TEXTBOX_DrawContext(&osl_ctx);

    OSL_ScanShort(progress_cb);
    shortScanned = 1;
    hbScanShort.bgcolor = LCD_RGB(0, 128, 0);
    if (shortScanned && openScanned && loadScanned)
        hbSave.bgcolor = LCD_RGB(0, 128, 0);
    progresstxt[0] = '\0';
    TEXTBOX_SetText(&osl_ctx, hbScanProgressId, progresstxt);
    TEXTBOX_DrawContext(&osl_ctx);
}

static void _hb_scan_open(void)
{
    progressval = 100;
    progress_cb(0);

    hbScanOpen.bgcolor = LCD_RGB(128, 128, 0);
    TEXTBOX_DrawContext(&osl_ctx);

    OSL_ScanOpen(progress_cb);
    openScanned = 1;
    hbScanOpen.bgcolor = LCD_RGB(0, 128, 0);
    if (shortScanned && openScanned && loadScanned)
        hbSave.bgcolor = LCD_RGB(0, 128, 0);
    progresstxt[0] = '\0';
    TEXTBOX_SetText(&osl_ctx, hbScanProgressId, progresstxt);
    TEXTBOX_DrawContext(&osl_ctx);
}

static void _hb_scan_load(void)
{
    progressval = 100;
    progress_cb(0);

    hbScanLoad.bgcolor = LCD_RGB(128, 128, 0);
    TEXTBOX_DrawContext(&osl_ctx);

    OSL_ScanLoad(progress_cb);
    loadScanned = 1;
    hbScanLoad.bgcolor = LCD_RGB(0, 128, 0);
    if (shortScanned && openScanned && loadScanned)
        hbSave.bgcolor = LCD_RGB(0, 128, 0);
    progresstxt[0] = '\0';
    TEXTBOX_SetText(&osl_ctx, hbScanProgressId, progresstxt);
    TEXTBOX_DrawContext(&osl_ctx);
}

static void _hit_save(void)
{
    if (!(shortScanned && openScanned && loadScanned))
        return;
    OSL_Calculate();
    Sleep(500);
    rqExit = 1;
}

void OSL_CalWnd(void)
{
    //if (!OSL_IsErrCorrLoaded())
    //    OSL_ScanErrCorr();
    if (-1 == OSL_GetSelected())
        return;

    rqExit = 0;
    shortScanned = 0;
    openScanned = 0;
    loadScanned = 0;
    progresstxt[0] = '\0';

    LCD_FillAll(LCD_BLACK);
    while (TOUCH_IsPressed());

    FONT_Print(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 110, 0, "OSL Calibration, file %s", OSL_GetSelectedName());

    TEXTBOX_InitContext(&osl_ctx);
    TEXTBOX_t hbEx = {.x0 = 10, .y0 = 220, .text = " Cancel and exit ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLUE, .bgcolor = LCD_YELLOW, .cb = _hit_ex };

    static char shortvalue[50];
    sprintf(shortvalue, " Scan short: %d Ohm ", (int)CFG_GetParam(CFG_PARAM_OSL_RSHORT));
    static char loadvalue[50];
    sprintf(loadvalue, " Scan load: %d Ohm ", (int)CFG_GetParam(CFG_PARAM_OSL_RLOAD));
    static char openvalue[50];
    if (CFG_GetParam(CFG_PARAM_OSL_ROPEN) < 10000)
        sprintf(openvalue, " Scan open: %d Ohm ", (int)CFG_GetParam(CFG_PARAM_OSL_ROPEN));
    else
        strcpy(openvalue, " Scan open: infinite ");

    hbScanShort = (TEXTBOX_t){.x0 = 10, .y0 = 50, .text = shortvalue, .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLUE, .bgcolor = LCD_RGB(64,64,64), .cb = _hb_scan_short };
    hbScanLoad = (TEXTBOX_t){.x0 = 10, .y0 = 100, .text = loadvalue, .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLUE, .bgcolor = LCD_RGB(64,64,64), .cb = _hb_scan_load };
    hbScanOpen = (TEXTBOX_t){.x0 = 10, .y0 = 150, .text = openvalue, .font = FONT_FRANBIG,
                            .fgcolor = LCD_BLUE, .bgcolor = LCD_RGB(64,64,64), .cb = _hb_scan_open };
    hbSave = (TEXTBOX_t){.x0 = 300, .y0 = 220, .text = " Save and exit ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_RGB(128,128,128), .bgcolor = LCD_RGB(64,64,64), .cb = _hit_save };
    hbScanProgress = (TEXTBOX_t){.x0 = 350, .y0 = 50, .text = progresstxt, .font = FONT_FRANBIG, .nowait = 1, .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK };

    TEXTBOX_Append(&osl_ctx, &hbEx);
    TEXTBOX_Append(&osl_ctx, &hbScanShort);
    TEXTBOX_Append(&osl_ctx, &hbScanLoad);
    TEXTBOX_Append(&osl_ctx, &hbScanOpen);
    TEXTBOX_Append(&osl_ctx, &hbSave);
    hbScanProgressId = TEXTBOX_Append(&osl_ctx, &hbScanProgress);
    TEXTBOX_DrawContext(&osl_ctx);

    for(;;)
    {
        if (TEXTBOX_HitTest(&osl_ctx))
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
