/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "font.h"
#include "touch.h"

#include "textbox.h"
#include "config.h"
#include "fftwnd.h"
#include "generator.h"
#include "measurement.h"
#include "oslcal.h"
#include "panvswr2.h"
#include "panfreq.h"
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
static TEXTBOX_t hbHwCal;
static TEXTBOX_t hbOslCal;
static TEXTBOX_t hbConfig;
static TEXTBOX_t hbPan;
static TEXTBOX_t hbMeas;
static TEXTBOX_t hbGen;
static TEXTBOX_t hbRemote;
static TEXTBOX_t hbDsp;
static TEXTBOX_t hbUSBD;
static TEXTBOX_t hbTimestamp;
static TEXTBOX_t hbTDR;
static TEXTBOX_t hbMulti;
static TEXTBOX_t hbExitCal;


static TEXTBOX_CTX_t menu1_ctx;
static void Menu1(void);
static void Colours(void);

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
uint8_t i;

//    BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_BLACK);
//    LCD_ShowActiveLayerOnly();
    ColourSelection=1;
    FatLines=false;
    for(i=0;i<5;i++)fr[i]=0;
    while (TOUCH_IsPressed());

    //Initialize textbox context
    TEXTBOX_InitContext(&main_ctx);

    //Create menu items and append to textbox context

    //HW calibration menu
    hbHwCal = (TEXTBOX_t){.x0 = COL1, .y0 = 0, .text =    " Calibration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Menu1 };
    TEXTBOX_Append(&main_ctx, &hbHwCal);

    //OSL calibration menu
    hbOslCal = (TEXTBOX_t){.x0 = COL1, .y0 = 50, .text =  " Colours ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Colours };
    TEXTBOX_Append(&main_ctx, &hbOslCal);

    //Device configuration menu
    hbConfig = (TEXTBOX_t){.x0 = COL1, .y0 = 100, .text = " Configuration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = CFG_ParamWnd };
    TEXTBOX_Append(&main_ctx, &hbConfig);

    //USB access
    hbUSBD = (TEXTBOX_t){.x0 = COL1, .y0 = 150, .text =   " USB HS cardrdr ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = USBD_Proc };
    TEXTBOX_Append(&main_ctx, &hbUSBD);

    //MultiScan  WK
    hbMulti = (TEXTBOX_t){.x0 = COL1, .y0 = 200, .text =   " Multi SWR ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = MultiSWR_Proc };
    TEXTBOX_Append(&main_ctx, &hbMulti);

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

    //TDR window
    hbTDR = (TEXTBOX_t){.x0 = COL2, .y0 = 200, .text = " Time Domain ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = TDR_Proc };
    TEXTBOX_Append(&main_ctx, &hbTDR);

    hbTimestamp = (TEXTBOX_t) {.x0 = 0, .y0 = 256, .text = "EU1KY AA v." AAVERSION ", hg rev: " HGREVSTR(HGREV) ", Build: " BUILD_TIMESTAMP, .font = FONT_FRAN,
                            .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK };
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
//            BSP_LCD_SelectLayer(1);
            LCD_FillAll(LCD_BLACK);
            TEXTBOX_DrawContext(&main_ctx);
//            LCD_ShowActiveLayerOnly();
            PROTOCOL_Reset();
        }
        PROTOCOL_Handler();
    }
}
#define XXa 2
static void Daylight(void){
    ColourSelection=0;
    SetColours();
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 240, 10,   "  Daylight Colours   ");
   // FONT_Print(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 220, 52, "%X BackGrColor", BackGrColor);
  //  FONT_Print(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 220, 92, "%X TextColor", TextColor);
}

static void Inhouse(void){
    ColourSelection=1;
    SetColours();
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 240, 10,   "  Inhouse Colours   ");
  //  FONT_Print(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 220, 52, "%X BackGrColor", BackGrColor);
  //  FONT_Print(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 220, 92, "%X TextColor", TextColor);
}
static void Fatlines(void){
    FONT_Write(FONT_FRANBIG, LCD_BLACK, LCD_YELLOW, 20, 10,   "  Fat Lines  ");
    FatLines=true;
}
static void Thinlines(void){
    FONT_Write(FONT_FRANBIG, LCD_BLACK, LCD_YELLOW, 20, 10,   "  Thin Lines ");
    FatLines=false;
}
static bool rqExit1;

static void Exit1(void){
    rqExit1=true;
}

static const TEXTBOX_t tb_col[] = {
    (TEXTBOX_t){ .x0 = XXa, .y0 = 50, .text = "Daylight", .font = FONT_FRANBIG, .width = 200, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = (void(*)(void))Daylight, .cbparam = 1, .next = (void*)&tb_col[1] },
    (TEXTBOX_t){ .x0 = XXa, .y0 = 100, .text = "Inhouse", .font = FONT_FRANBIG, .width = 200, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = (void(*)(void))Inhouse, .cbparam = 1, .next = (void*)&tb_col[2] },
    (TEXTBOX_t){ .x0 = XXa, .y0 = 150, .text = "Fat Lines", .font = FONT_FRANBIG, .width = 200, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = (void(*)(void))Fatlines, .cbparam = 1, .next = (void*)&tb_col[3] },
    (TEXTBOX_t){ .x0 = XXa, .y0 = 200, .text = "Thin Lines", .font = FONT_FRANBIG, .width = 200, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = (void(*)(void))Thinlines, .cbparam = 1, .next = (void*)&tb_col[4] },
    (TEXTBOX_t){ .x0 = 240, .y0 = 200, .text = "  Exit   ", .font = FONT_FRANBIG, .width = 200, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = LCD_RED, .cb = (void(*)(void))Exit1, .cbparam = 1,  },
};


static void Colours(void){

    while(TOUCH_IsPressed());
    SetColours();
    rqExit1=false;
 //   BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_COLOR_DARKGRAY);
//    LCD_ShowActiveLayerOnly();
    TEXTBOX_CTX_t fctx;
    if(ColourSelection==1)
        FONT_Write(FONT_FRANBIG, LCD_BLACK, LCD_YELLOW, 240, 10,   "  Inhouse Colours   ");
    else
        FONT_Write(FONT_FRANBIG, LCD_BLACK, LCD_YELLOW, 240, 10,   "  Daylight Colours   ");
    if(FatLines==true)
        FONT_Write(FONT_FRANBIG, LCD_BLACK, LCD_YELLOW, 20, 10,   "  Fat Lines  ");
    else
        FONT_Write(FONT_FRANBIG, LCD_BLACK, LCD_YELLOW, 20, 10,   "  Thin Lines ");

    TEXTBOX_InitContext(&fctx);
    TEXTBOX_Append(&fctx, (TEXTBOX_t*)tb_col); //Append the very first element of the list in the flash.
                                                      //It is enough since all the .next fields are filled at compile time.
    TEXTBOX_DrawContext(&fctx);


    for(;;)
    {
        Sleep(0); //for autosleep to work
        if (TEXTBOX_HitTest(&fctx))
        {
            Sleep(50);
            if (rqExit1)
            {
                rqExit1=false;
                return;
            }
            Sleep(50);
        }
        Sleep(0);
    }
}
static const TEXTBOX_t tb_menu1[] = {
    (TEXTBOX_t){.x0 = COL1, .y0 = 0, .text =    " HW Calibration  ", .font = FONT_FRANBIG,.width = 200, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalErrCorr , .cbparam = 1, .next = (void*)&tb_menu1[1] },
    (TEXTBOX_t){.x0 = COL1, .y0 = 50, .text =  " OSL Calibration ", .font = FONT_FRANBIG,.width = 200, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalWnd , .cbparam = 1, .next = (void*)&tb_menu1[2] },
    (TEXTBOX_t){ .x0 = 0, .y0 = 200, .text = "  Exit   ", .font = FONT_FRANBIG, .width = 200, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = LCD_RED, .cb = (void(*)(void))Exit1, .cbparam = 1,},
};

static void Menu1(void){

    while(TOUCH_IsPressed());
    rqExit1=false;
//    BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_BLACK);
//    LCD_ShowActiveLayerOnly();
    //Initialize textbox context
    TEXTBOX_InitContext(&menu1_ctx);
//HW calibration menu
    TEXTBOX_Append(&menu1_ctx, (TEXTBOX_t*)tb_menu1);
    TEXTBOX_DrawContext(&menu1_ctx);

 for(;;)
    {
        Sleep(0); //for autosleep to work
        if (TEXTBOX_HitTest(&menu1_ctx))
        {
            Sleep(0);
            //BSP_LCD_SelectLayer(1);
            LCD_FillAll(LCD_BLACK);
            TEXTBOX_DrawContext(&menu1_ctx);
            //LCD_ShowActiveLayerOnly();
            if (rqExit1)
            {
                rqExit1=false;
                return;
            }
            Sleep(50);
        }
        Sleep(0);
    }
}
