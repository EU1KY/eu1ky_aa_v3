/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdio.h>
#include <string.h>

#include "LCD.h"
#include "font.h"
#include "touch.h"

#include "textbox.h"
#include "config.h"
#include "fftwnd.h"
#include "generator.h"
#include "measurement.h"
#include "oslcal.h"
#include "panvswr2.h"
#include "main.h"
#include "usbd_storage.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "crash.h"

extern void Sleep(uint32_t);

static TEXTBOX_CTX_t main_ctx;
static TEXTBOX_t hbHwCal;
static TEXTBOX_t hbOslCal;
static TEXTBOX_t hbConfig;
static TEXTBOX_t hbPan;
static TEXTBOX_t hbMeas;
static TEXTBOX_t hbGen;
static TEXTBOX_t hbRemote;
static TEXTBOX_t hbDsp;
static TEXTBOX_t hbUSBD;

#define M_BGCOLOR LCD_RGB(0,0,64)    //Menu item background color
#define M_FGCOLOR LCD_RGB(255,255,0) //Menu item foreground color

#define COL1 10  //Column 1 x coordinate
#define COL2 250 //Column 2 x coordinate

static USBD_HandleTypeDef USBD_Device;
extern char SDPath[4];
extern FATFS SDFatFs;

static void USBD_Proc()
{
    while(TOUCH_IsPressed());

    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 10, 0, "USB storage access via USB HS port");
    FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLUE, 10, 200, "Reset to exit");

    FATFS_UnLinkDriver(SDPath);
    BSP_SD_DeInit();
    Sleep(100);

    USBD_Init(&USBD_Device, &MSC_Desc, 0);
    USBD_RegisterClass(&USBD_Device, USBD_MSC_CLASS);
    USBD_MSC_RegisterStorage(&USBD_Device, &USBD_DISK_fops);
    USBD_Start(&USBD_Device);

    for(;;);
    LCDPoint coord;
    while (!TOUCH_Poll(&coord))
    {
        if (coord.x < 150 && coord.y > 200)
        {
            USBD_Stop(&USBD_Device);
            USBD_DeInit(&USBD_Device);
            BSP_SD_DeInit();

            Sleep(100);

            //Mount SD card
            if (FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
                CRASH("FATFS_LinkDriver failed");
            if (f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
                CRASH("f_mount failed");

            while(TOUCH_IsPressed());
            return;
        }
    }
}

void MainWnd(void)
{
    LCD_FillAll(LCD_BLACK);
    while (TOUCH_IsPressed());

    //Initialize textbox context
    TEXTBOX_InitContext(&main_ctx);

    //Create menu items and append to textbox context

    //HW calibration menu
    hbHwCal = (TEXTBOX_t){.x0 = COL1, .y0 = 0, .text =    " HW Calibration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalErrCorr };
    TEXTBOX_Append(&main_ctx, &hbHwCal);

    //OSL calibration menu
    hbOslCal = (TEXTBOX_t){.x0 = COL1, .y0 = 60, .text =  " OSL Calibration ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalWnd };
    TEXTBOX_Append(&main_ctx, &hbOslCal);

    //Device configuration menu
    hbConfig = (TEXTBOX_t){.x0 = COL1, .y0 = 120, .text = " Configuration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = CFG_ParamWnd };
    TEXTBOX_Append(&main_ctx, &hbConfig);

    //Panoramic scan window
    hbPan = (TEXTBOX_t){.x0 = COL2, .y0 =   0, .text =    " Panoramic scan ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = PANVSWR2_Proc };
    TEXTBOX_Append(&main_ctx, &hbPan);

    //Measurement window
    hbMeas = (TEXTBOX_t){.x0 = COL2, .y0 =  50, .text =   " Measurement    ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = MEASUREMENT_Proc };
    TEXTBOX_Append(&main_ctx, &hbMeas);

    //Generator window
    hbGen  = (TEXTBOX_t){.x0 = COL2, .y0 = 100, .text =   " Generator      ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = GENERATOR_Window_Proc };
    TEXTBOX_Append(&main_ctx, &hbGen);

    //DSP window
    hbDsp  = (TEXTBOX_t){.x0 = COL2, .y0 = 150, .text =   " DSP            ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = FFTWND_Proc };
    TEXTBOX_Append(&main_ctx, &hbDsp);

    //USB access
    hbUSBD = (TEXTBOX_t){.x0 = COL2, .y0 = 200, .text =   " USB access     ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = USBD_Proc };
    TEXTBOX_Append(&main_ctx, &hbUSBD);

    //Draw context
    TEXTBOX_DrawContext(&main_ctx);

    //Main loop
    for(;;)
    {
        if (TEXTBOX_HitTest(&main_ctx))
        {
            Sleep(50);
            //Redraw main window
            LCD_FillAll(LCD_BLACK);
            TEXTBOX_DrawContext(&main_ctx);
        }
    }
}
