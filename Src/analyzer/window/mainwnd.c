/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
#include "dsp.h"
#include "gen.h"
#include "aauart.h"
#include "build_timestamp.h"
#include "tdr.h"

extern void Sleep(uint32_t);

static TEXTBOX_CTX_t main_ctx;
static TEXTBOX_t hbTitle;
static TEXTBOX_t hbHwCal;
static TEXTBOX_t hbOslCal;
static TEXTBOX_t hbConfig;
static TEXTBOX_t hbPan;
static TEXTBOX_t hbMeas;
static TEXTBOX_t hbGen;
static TEXTBOX_t hbDsp;
static TEXTBOX_t hbUSBD;
static TEXTBOX_t hbTimestamp;
static TEXTBOX_t hbTDR;
static TEXTBOX_t hbZ0;

#define M_BGCOLOR LCD_RGB(0,0,127)    //Menu item background color
#define M_FGCOLOR LCD_RGB(255,255,255) //Menu item foreground color

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
    FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 80, 200, "Exit (Reset device)");

    FATFS_UnLinkDriver(SDPath);
    BSP_SD_DeInit();
    Sleep(100);

    USBD_Init(&USBD_Device, &MSC_Desc, 0);
    USBD_RegisterClass(&USBD_Device, USBD_MSC_CLASS);
    USBD_MSC_RegisterStorage(&USBD_Device, &USBD_DISK_fops);
    USBD_Start(&USBD_Device);

    //USBD works in interrupts only, no need to leave CPU running in main.
    for(;;)
    {
        Sleep(50); //To enter low power if necessary
        LCDPoint coord;
        if (TOUCH_Poll(&coord))
        {
            while(TOUCH_IsPressed());
            if (coord.x > 80 && coord.x < 320 && coord.y > 200 && coord.y < 240)
            {
                USBD_Stop(&USBD_Device);
                USBD_DeInit(&USBD_Device);
                BSP_LCD_DisplayOff();
                BSP_SD_DeInit();
                Sleep(100);
                NVIC_SystemReset(); //Never returns
            }
        }
    }
}

//==========================================================================================
// Serial remote control protocol state machine implementation (emulates RigExpert AA-170)
// Runs in main window only
//==========================================================================================
#define RXB_SIZE 64
static char rxstr[RXB_SIZE+1];
static uint32_t rx_ctr = 0;
static char* rxstr_p;
static uint32_t _fCenter = 10000000ul;
static uint32_t _fSweep = 100000ul;
static const char *ERR = "ERROR\r";
static const char *OK = "OK\r";

static char* _trim(char *str)
{
    char *end;
    while(isspace((int)*str))
        str++;
    if (*str == '\0')
        return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((int)*end))
        end--;
    *(end + 1) = '\0';
    return str;
}

static int PROTOCOL_RxCmd(void)
{
    int ch = AAUART_Getchar();
    if (ch == EOF)
        return 0;
    else if (ch == '\r' || ch == '\n')
    {
        // command delimiter
        if (!rx_ctr)
            rxstr[0] = '\0';
        rx_ctr = 0;
        return 1;
    }
    else if (ch == '\0') //ignore it
        return 0;
    else if (ch == '\t')
        ch = ' ';
    else if (rx_ctr >= (RXB_SIZE - 1)) //skip all characters that do not fit in the rx buffer
        return 0;
    //Store character to buffer
    rxstr[rx_ctr++] = (char)(ch & 0xFF);
    rxstr[rx_ctr] = '\0';
    return 0;
}

static void PROTOCOL_Reset(void)
{
    rxstr[0] = '\0';
    rx_ctr = 0;
    //Purge contents of RX buffer
    while (EOF != AAUART_Getchar());
}

static void PROTOCOL_Handler(void)
{
    if (!PROTOCOL_RxCmd())
        return;
    //Command received
    rxstr_p = _trim(rxstr);
    strlwr(rxstr_p);

    if (rxstr_p[0] == '\0') //empty command
    {
        AAUART_PutString(ERR);
        return;
    }

    if (0 == strcmp("ver", rxstr_p))
    {
        AAUART_PutString("AA-600 401\r");
        return;
    }

    if (0 == strcmp("on", rxstr_p))
    {
        GEN_SetMeasurementFreq(GEN_GetLastFreq());
        AAUART_PutString(OK);
        return;
    }

    if (0 == strcmp("off", rxstr_p))
    {
        GEN_SetMeasurementFreq(0);
        AAUART_PutString(OK);
        return;
    }

    if (rxstr_p == strstr(rxstr_p, "am"))
    {//Amplitude setting
        AAUART_PutString(OK);
        return;
    }

    if (rxstr_p == strstr(rxstr_p, "ph"))
    {//Phase setting
        AAUART_PutString(OK);
        return;
    }

    if (rxstr_p == strstr(rxstr_p, "de"))
    {//Set delay
        AAUART_PutString(OK);
        return;
    }

    if (rxstr_p == strstr(rxstr_p, "fq"))
    {
        uint32_t FHz = 0;
        if(isdigit((int)rxstr_p[2]))
        {
            FHz = (uint32_t)atoi(&rxstr_p[2]);
        }
        else
        {
            AAUART_PutString(ERR);
            return;
        }
        if (FHz < BAND_FMIN)
        {
            AAUART_PutString(ERR);
        }
        else
        {
            _fCenter = FHz;
            AAUART_PutString(OK);
        }
        return;
    }

    if (rxstr_p == strstr(rxstr_p, "sw"))
    {
        uint32_t sw = 0;
        if(isdigit((int)rxstr_p[2]))
        {
            sw = (uint32_t)atoi(&rxstr_p[2]);
        }
        else
        {
            AAUART_PutString(ERR);
            return;
        }
        _fSweep = sw;
        AAUART_PutString(OK);
        return;
    }

    if (rxstr_p == strstr(rxstr_p, "frx"))
    {
        uint32_t steps = 0;
        uint32_t fint;
        uint32_t fstep;

        if(isdigit((int)rxstr_p[3]))
        {
            steps = (uint32_t)atoi(&rxstr_p[3]);
        }
        else
        {
            AAUART_PutString(ERR);
            return;
        }
        if (steps == 0)
        {
            AAUART_PutString(ERR);
            return;
        }
        if (_fSweep / 2 > _fCenter)
            fint = 10;
        else
            fint = _fCenter - _fSweep / 2;
        fstep = _fSweep / steps;
        steps += 1;
        char txstr[64];
        while (steps--)
        {
            DSP_RX rx;

            DSP_Measure(fint, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
            rx = DSP_MeasuredZ();
            while(AAUART_IsBusy())
                Sleep(0); //prevent overwriting the data being transmitted
            sprintf(txstr, "%.6f,%.2f,%.2f\r", ((float)fint) / 1000000., crealf(rx), cimagf(rx));
            AAUART_PutString(txstr);
            fint += fstep;
        }
        AAUART_PutString(OK);
        return;
    }
    AAUART_PutString(ERR);
}

// ================================================================================================
// Main window procedure (never returns)
void MainWnd(void)
{
    BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_BLACK);
    LCD_ShowActiveLayerOnly();

    while (TOUCH_IsPressed());

    //Initialize textbox context
    TEXTBOX_InitContext(&main_ctx);

    //Create menu items and append to textbox context

    hbTitle = (TEXTBOX_t) {.x0 = COL1, .y0 = 0, .text = " Main menu ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK };
    TEXTBOX_Append(&main_ctx, &hbTitle);

    //HW calibration menu
    hbHwCal = (TEXTBOX_t){.x0 = COL1, .y0 = 50, .text =    " HW Calibration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalErrCorr,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbHwCal);

    //OSL calibration menu
    hbOslCal = (TEXTBOX_t){.x0 = COL1, .y0 = 100, .text =  " OSL Calibration ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalWnd,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbOslCal);

    //Device configuration menu
    hbConfig = (TEXTBOX_t){.x0 = COL1, .y0 = 150, .text = " Configuration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = CFG_ParamWnd,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbConfig);

    //USB access
    hbUSBD = (TEXTBOX_t){.x0 = COL1, .y0 = 200, .text =   " USB HS cardrdr ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = USBD_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbUSBD);

    //Panoramic scan window
    hbPan = (TEXTBOX_t){.x0 = COL2, .y0 =   0, .text =    " Panoramic scan ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = PANVSWR2_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbPan);

    //Measurement window
    hbMeas = (TEXTBOX_t){.x0 = COL2, .y0 =  40, .text =   " Measurement    ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = MEASUREMENT_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbMeas);

    //Generator window
    hbGen  = (TEXTBOX_t){.x0 = COL2, .y0 = 80, .text =   " Generator      ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = GENERATOR_Window_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbGen);

    //DSP window
    hbDsp  = (TEXTBOX_t){.x0 = COL2, .y0 = 120, .text =   " DSP            ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = FFTWND_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbDsp);

    //TDR window
    hbTDR = (TEXTBOX_t){.x0 = COL2, .y0 = 160, .text = " Time Domain ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = TDR_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbTDR);

    //Z0 window
    hbZ0 = (TEXTBOX_t){.x0 = COL2, .y0 = 200, .text = " Measure line Z0 ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Z0_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbZ0);

    //Version and Build Timestamp
    hbTimestamp = (TEXTBOX_t) {.x0 = 0, .y0 = 256, .text = "EU1KY AA v." AAVERSION ", hg rev: " HGREVSTR(HGREV) ", Build: " BUILD_TIMESTAMP, .font = FONT_FRAN,
                            .fgcolor = LCD_LGRAY, .bgcolor = LCD_BLACK };
    TEXTBOX_Append(&main_ctx, &hbTimestamp);

    //Draw context
    TEXTBOX_DrawContext(&main_ctx);

    PROTOCOL_Reset();

    //Main loop
    for(;;)
    {
        Sleep(0); //for autosleep to work
        if (TEXTBOX_HitTest(&main_ctx))
        {
            Sleep(50);
            //Redraw main window
            BSP_LCD_SelectLayer(1);
            LCD_FillAll(LCD_BLACK);
            TEXTBOX_DrawContext(&main_ctx);
            LCD_ShowActiveLayerOnly();
            PROTOCOL_Reset();
        }
        PROTOCOL_Handler();
    }
}
