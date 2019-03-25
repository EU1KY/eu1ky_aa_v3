/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#include "LCD.h"
#include "main.h"
#include "touch.h"
#include "font.h"
#include "config.h"
#include "ff.h"
#include "crash.h"
#include "dsp.h"
#include "gen.h"
#include "oslfile.h"
#include "stm32746g_discovery_lcd.h"
#include "screenshot.h"
#include "panvswr2.h"
#include "panfreq.h"
#include "smith.h"
#include "textbox.h"
#include "generator.h"
#include "FreqCounter.h"

#define X0 51
#define Y0 18
#define WWIDTH  400
#define WHEIGHT 200
#define WY(offset) ((WHEIGHT + Y0) - (offset))
//#define WGRIDCOLOR LCD_RGB(80,80,80)
#define WGRIDCOLOR LCD_COLOR_DARKGRAY
#define RED1 LCD_RGB(245,0,0)
#define RED2 LCD_RGB(235,0,0)


#define WGRIDCOLORBR LCD_RGB(160,160,96)
#define SMITH_CIRCLE_BG LCD_BLACK
#define SMITH_LINE_FG LCD_GREEN

//#define MAX(a,b) (((a)>(b))?(a):(b))
//#define MIN(a,b) (((a)<(b))?(a):(b))

// Please read the article why smoothing looks beautiful but actually
// decreases precision, and averaging increases precision though looks ugly:
// http://www.microwaves101.com/encyclopedias/smoothing-is-cheating
// This analyzer draws both smoothed (bright) and averaged (dark) measurement
// results, you see them both.
#define SMOOTHWINDOW 3 //Must be odd!
#define SMOOTHOFS (SMOOTHWINDOW/2)
#define SMOOTHWINDOW_HI 7 //Must be odd!
#define SMOOTHOFS_HI (SMOOTHWINDOW_HI/2)
#define SM_INTENSITY 64
extern uint8_t rqDel;
extern void ShowF(void);
void  DrawFootText(void);

typedef enum
{
    GRAPH_VSWR, GRAPH_VSWR_Z, GRAPH_VSWR_RX, GRAPH_RX, GRAPH_SMITH, GRAPH_S11
} GRAPHTYPE;

/*typedef struct
{
    uint32_t flo;
    uint32_t fhi;
} HAM_BANDS;
*/
static const HAM_BANDS hamBands[] =
{
    {1800ul,  2000ul},
    {3500ul,  3800ul},
    {7000ul,  7200ul},
    {10100ul, 10150ul},
    {14000ul, 14350ul},
    {18068ul, 18168ul},
    {21000ul, 21450ul},
    {24890ul, 24990ul},
    {28000ul, 29700ul},
    {50000ul, 52000ul},
    {144000ul, 146000ul},
    {222000ul, 225000ul},
    {430000ul, 440000ul},
};

static const uint32_t hamBandsNum = sizeof(hamBands) / sizeof(*hamBands);
static const uint32_t cx0 = 240; //Smith chart center
static const uint32_t cy0 = 120; //Smith chart center
static const int32_t smithradius = 100;
static const char *modstr = "EU1KY AA v." AAVERSION " ";

static uint32_t modstrw = 0;
// ** WK ** / DL8MBY:

const char* BSSTR[] = {"2 kHz","4 kHz","10 kHz","20 kHz","40 kHz","100 kHz",\
"200 kHz", "400 kHz", "1000 kHz", "2 MHz", "4 MHz", "10 MHz", "20 MHz",\
 "30 MHz", "40 MHz", "100 MHz", "200 MHz", "250 Mhz", "300 MHz",\
"350 MHz", "400 MHz", "450 MHz", "500 MHz"};
const char* BSSTR_HALF[] = {"1 kHz","2 kHz","5 kHz","10 kHz","20 kHz",\
"50 kHz","100 kHz", "200 kHz", "500 kHz", "1 MHz", "2 MHz", "5 MHz",\
 "10 MHz", "15 MHz", "20 MHz", "50 MHz", "100 MHz", "125 MHz", "150 MHz",\
 "175 MHz", "200 MHz", "225 MHz", "250 MHz"};
const uint32_t BSVALUES[] = {2,4,10,20,40,100,200, 400, 1000, 2000,\
4000, 10000, 20000, 30000, 40000, 100000, 200000, 250000, 300000,\
350000, 400000, 450000, 500000};

/*const char* BSSTR[] = {
    "2 kHz","4 kHz","10 kHz","20 kHz","40 kHz","100 kHz","200 kHz", "400 kHz", "1000 kHz", "2 MHz", "4 MHz", "10 MHz", "20 MHz", "30 MHz", "40 MHz", "100 MHz"};
const char* BSSTR_HALF[] = {"1 kHz","2 kHz","5 kHz","10 kHz","20 kHz","50 kHz","100 kHz", "200 kHz", "500 kHz", "1 MHz", "2 MHz", "5 MHz", "10 MHz", "15 MHz", "20 MHz", "50 MHz"};
const uint32_t BSVALUES[] = {2,4,10,20,40,100,200, 400, 1000, 2000, 4000, 10000, 20000, 30000, 40000, 100000};
*/

static uint32_t f1 = 14000000; //Scan range start frequency, in Hz
static BANDSPAN span = BS400;
static float fcur;// frequency at cursor position in kHz
static char buf[64];
static LCDPoint pt;
static float complex values[WWIDTH+1];
static float complex SavedValues1[WWIDTH+1];
static float complex SavedValues2[WWIDTH+1];
static float complex SavedValues3[WWIDTH+1];
static int isStored;
static int isMeasured = 0;
static uint32_t cursorPos = WWIDTH / 2;
static GRAPHTYPE grType = GRAPH_VSWR;
static uint32_t isSaved = 0;
static uint32_t cursorChangeCount = 0;
static uint32_t autofast = 0;
static int loglog=0;// scale for SWR
extern volatile uint32_t autosleep_timer;

static void DrawRX();
static void DrawSmith();
static float complex SmoothRX(int idx, int useHighSmooth);
static TEXTBOX_t SWR_ctx;
static TEXTBOX_CTX_t SWR1;
void SWR_Exit(void);
static void SWR_2(void);
void SWR_Mute(void);
static void SWR_3(void);
void SWR_SetFrequency(void);
void SWR_SetFrequencyMuted(void);
void QuMeasure(void);
void QuCalibrate(void);
void DrawX_Scale(float maxRXi, float minRXi);
int sFreq, sCalib;

#define M_BGCOLOR LCD_RGB(0,0,64)    //Menu item background color
#define M_FGCOLOR LCD_RGB(255,255,0) //Menu item foreground color


static const TEXTBOX_t tb_menuQuartz[] = {
    (TEXTBOX_t){.x0 = 10, .y0 = 180, .text =    "Set Frequency", .font = FONT_FRANBIG,.width = 180, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = SWR_SetFrequency , .cbparam = 1, .next = (void*)&tb_menuQuartz[1] },
    (TEXTBOX_t){.x0 = 200, .y0 = 180, .text =   "Calibrate OPEN", .font = FONT_FRANBIG,.width = 220, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = QuCalibrate , .cbparam = 1, .next = (void*)&tb_menuQuartz[2] },
    (TEXTBOX_t){.x0 = 80, .y0 = 230, .text =  "Start", .font = FONT_FRANBIG,.width = 100, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = QuMeasure , .cbparam = 1, .next = (void*)&tb_menuQuartz[3] },
    (TEXTBOX_t){ .x0 = 0, .y0 = 230, .text = "Exit", .font = FONT_FRANBIG, .width = 70, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = LCD_RED, .cb = (void(*)(void))SWR_Exit, .cbparam = 1,},
};

static const TEXTBOX_t tb_menuQuartz2[] = {

    (TEXTBOX_t){.x0 = 270, .y0 = 230, .text =  "Start", .font = FONT_FRANBIG,.width = 100, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = QuMeasure , .cbparam = 1, .next = (void*)&tb_menuQuartz2[1] },
    (TEXTBOX_t){.x0 = 80, .y0 = 230, .text =    "Set Frequency", .font = FONT_FRANBIG,.width = 180, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = SWR_SetFrequency , .cbparam = 1, .next = (void*)&tb_menuQuartz2[2] },
    (TEXTBOX_t){ .x0 = 0, .y0 = 230, .text = "Exit", .font = FONT_FRANBIG, .width = 70, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = LCD_RED, .cb = (void(*)(void))SWR_Exit, .cbparam = 1,},
};


static const TEXTBOX_t tb_menuSWR[] = {
    (TEXTBOX_t){.x0 = 70, .y0 = 210, .text =    "Frequency", .font = FONT_FRANBIG,.width = 120, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = SWR_SetFrequencyMuted , .cbparam = 1, .next = (void*)&tb_menuSWR[1] },
   (TEXTBOX_t){.x0 = 280, .y0 = 210, .text =  "SWR_2", .font = FONT_FRANBIG,.width = 100, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = SWR_2 , .cbparam = 1, .next = (void*)&tb_menuSWR[2] },
    (TEXTBOX_t){.x0 = 380, .y0 = 210, .text =  "SWR_3", .font = FONT_FRANBIG,.width = 96, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = SWR_3 , .cbparam = 1, .next = (void*)&tb_menuSWR[3] },
    (TEXTBOX_t){.x0 = 190, .y0 = 210, .text =  "Mute", .font = FONT_FRANBIG,.width = 90, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = SWR_Mute , .cbparam = 1, .next = (void*)&tb_menuSWR[4] },
    (TEXTBOX_t){ .x0 = 0, .y0 = 210, .text = "Exit", .font = FONT_FRANBIG, .width = 70, .height = 34, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = LCD_RED, .cb = (void(*)(void))SWR_Exit, .cbparam = 1,},
};

static uint32_t multi_fr[5]  = {1850,21200,27800,3670,7150};//Multi SWR frequencies in kHz
static uint32_t multi_bw[5]  = {200,1000,200,400,100};//Multi SWR bandwidth in kHz
static BANDSPAN multi_bwNo[5]  = {6,8,6,5,4};//Multi SWR bandwidth number
static int BeepIsActive;

void Beep(int duration){
    if(BeepIsOn==0) return;
    if(SWRTone==1) return;// SWR Audio against beep
    //if(BeepIsActive==0){
        BeepIsActive=1;// make beep audible
        UB_TIMER2_Init_FRQ(880);
        UB_TIMER2_Start();
        Sleep(100);
        UB_TIMER2_Stop();
    //}
    BeepIsActive=0;// no sound

    //if(duration==1) {
      //  UB_TIMER2_Stop();
        //BeepIsActive=0;// beep not audible
    //}
}

unsigned long GetUpper(int i){
if((i>=0)&&(i<=12))
    return 1000*hamBands[i].fhi;
return 0;
}
unsigned long GetLower(int i){
if((i>=0)&&(i<=12))
    return 1000*hamBands[i].flo;
return 0;
}




int GetBandNr(unsigned long freq){
int i, found=0;
    for(i=0;i<=12;i++){
        if(GetLower(i)>=freq){
            found=1;
            i--;
            break;
        }
    }
    if(found==1){
        if(GetUpper(i)>=freq)
            return i;
    }
    if((GetLower(12)<=freq)&&(GetUpper(12)>=freq)) return 12;
    return -1;// not in a Ham band
}

static void WK_InvertPixel(LCDPoint p){
LCDColor    c;
    c=LCD_ReadPixel(p);
    switch (c){
    case LCD_COLOR_YELLOW:
        {
            LCD_SetPixel(p,LCD_COLOR_RED);
            return;
        }
    case LCD_COLOR_WHITE:
        {
            LCD_SetPixel(p,RED1);
            return;
        }
    case LCD_COLOR_DARKGRAY:
        {
            LCD_SetPixel(p,RED2);
            return;
        }
    case LCD_COLOR_RED:
        {
            LCD_SetPixel(p,LCD_COLOR_YELLOW);
            return;
        }
    case RED1:
        {
            LCD_SetPixel(p,LCD_COLOR_WHITE);
            return;
        }
    case RED2:
        {
            LCD_SetPixel(p,LCD_COLOR_DARKGRAY);
            return;
        }
    default:LCD_InvertPixel(p);
    }
}

static int swroffset(float swr)
{
    int offs = (int)roundf(150. * log10f(swr));
    if (offs >= WHEIGHT)
        offs = WHEIGHT - 1;
    else if (offs < 0)
        offs = 0;
    return offs;
}

static int Z_offset(float Z)
{
    int offs = (int)roundf(50. * log10f(Z+1));
    if (offs >= WHEIGHT)
        offs = WHEIGHT - 1;
    else if (offs < 0)
        offs = 0;
    return offs;
}


static float S11Calc(float swr)
{
    float offs = 20 * log10f((swr-1)/(swr+1));
    return offs;
}

static int IsFinHamBands(uint32_t f_kHz)
{
    uint32_t i;
    for (i = 0; i < hamBandsNum; i++)
    {
        if ((f_kHz >= hamBands[i].flo) && (f_kHz <= hamBands[i].fhi))
            return 1;
    }
    return 0;
}

static void DrawCursor()
{
    int8_t i;
    LCDPoint p;
    if (!isMeasured)
        return;

    if (grType == GRAPH_SMITH)
    {
        float complex rx = values[cursorPos]; //SmoothRX(cursorPos, f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) / 1000) ? 1 : 0);
        float complex g = OSL_GFromZ(rx, (float)CFG_GetParam(CFG_PARAM_R0));
        uint32_t x = (uint32_t)roundf(cx0 + crealf(g) * 100.);
        uint32_t y = (uint32_t)roundf(cy0 - cimagf(g) * 100.);
        p = LCD_MakePoint(x, y);
        for(i=-4;i<4;i++){
           p.x+=i;
           LCD_InvertPixel(p);
           p.x-=i;
        }
        for(i=-4;i<4;i++){
           p.y+=i;
           LCD_InvertPixel(p);
           p.y-=i;
        }
    }
    else
    {
        //Draw cursor line as inverted image
        p = LCD_MakePoint(X0 + cursorPos, Y0);
        if(ColourSelection==1){// Daylightcolours
            while (p.y < Y0 + WHEIGHT){
               if((p.y % 20)<10)
                    WK_InvertPixel(p);
               else LCD_InvertPixel(p);
               p.y++;

            }
        }
        else{
            while (p.y < Y0 + WHEIGHT){
                if((p.y % 20)<10)
                    LCD_InvertPixel(p);
                p.y++;

            }
        }
        if(FatLines){
            p.x--;
            while (p.y >= Y0)
            {
                LCD_InvertPixel(p);
                p.y--;
            }
            p.x+=2;
            while (p.y < Y0 + WHEIGHT)
            {
                LCD_InvertPixel(p);
                p.y++;
            }
            p.x--;
        }

        LCD_FillRect((LCDPoint){X0 + cursorPos-3,Y0+WHEIGHT+1},(LCDPoint){X0 + cursorPos+3,Y0+WHEIGHT+3},BackGrColor);
        LCD_FillRect((LCDPoint){X0 + cursorPos-2,Y0+WHEIGHT+1},(LCDPoint){X0 + cursorPos+2,Y0+WHEIGHT+3},TextColor);
    }
    Sleep(5);

}

static void DrawCursorText()
{
    float complex rx = values[cursorPos]; //SmoothRX(cursorPos, f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) / 1000) ? 1 : 0);
    float ga = cabsf(OSL_GFromZ(rx, (float)CFG_GetParam(CFG_PARAM_R0))); //G magnitude

    uint32_t fstart;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - 500*BSVALUES[span];

    fcur = ((float)(fstart/1000. + (float)cursorPos * BSVALUES[span] / WWIDTH));///1000.;
    if (fcur * 1000.f > (float)(CFG_GetParam(CFG_PARAM_BAND_FMAX) + 1))
        fcur = 0.f;

    float Q = 0.f;
    if ((crealf(rx) > 0.1f) && (fabs(cimagf(rx)) > crealf(rx)))
        Q = fabs(cimagf(rx) / crealf(rx));
    if (Q > 2000.f)
        Q = 2000.f;
    LCD_FillRect(LCD_MakePoint(150, Y0 + WHEIGHT + 16),LCD_MakePoint(409 , Y0 + WHEIGHT + 30),BackGrColor);
    FONT_Print(FONT_FRAN, TextColor, BackGrColor, 60, Y0 + WHEIGHT + 16, "F: %.3f   Z: %.1f%+.1fj   SWR: %.1f   MCL: %.2f dB   Q: %.1f       ",
               fcur,
               crealf(rx),
               cimagf(rx),
               DSP_CalcVSWR(rx),
               (ga > 0.01f) ? (-10. * log10f(ga)) : 99.f, // Matched cable loss
               Q
              );
   /* LCD_HLine(LCD_MakePoint(0,249), 62, CurvColor);
    LCD_HLine(LCD_MakePoint(70,249), 70, CurvColor);
    LCD_HLine(LCD_MakePoint(410,249), 69, CurvColor);*/
}

static void DrawCursorTextWithS11()
{
    float complex rx = values[cursorPos]; //SmoothRX(cursorPos, f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) / 1000) ? 1 : 0);
   // float ga = cabsf(OSL_GFromZ(rx, (float)CFG_GetParam(CFG_PARAM_R0))); //G magnitude

    uint32_t fstart;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - 500*BSVALUES[span];// / 2;

    fcur = ((float)(fstart/1000. + (float)cursorPos * BSVALUES[span] / WWIDTH));///1000.;
    if (fcur * 1000.f > (float)(CFG_GetParam(CFG_PARAM_BAND_FMAX) + 1))
        fcur = 0.f;
    LCD_FillRect(LCD_MakePoint(150, Y0 + WHEIGHT + 16),LCD_MakePoint(409 , Y0 + WHEIGHT + 30),BackGrColor);
    FONT_Print(FONT_FRAN, TextColor, BackGrColor, 60, Y0 + WHEIGHT + 16, "F: %.3f   Z: %.1f%+.1fj   SWR: %.1f   S11: %.2f dB          ",
               fcur,
               crealf(rx),
               cimagf(rx),
               DSP_CalcVSWR(rx),
               S11Calc(DSP_CalcVSWR(rx))
              );
   /* LCD_HLine(LCD_MakePoint(0,249), 62, CurvColor);
    LCD_HLine(LCD_MakePoint(70,249), 70, CurvColor);
    LCD_HLine(LCD_MakePoint(410,249), 69, CurvColor);*/
}

static void DrawAutoText(void)
{
   /* static const char* atxt = " Auto (fast, 1/8 pts)  ";
    if (0 == autofast)
        FONT_Print(FONT_FRAN, TextColor, BackGrColor, 260, Y0 + WHEIGHT + 16 + 16,  atxt);
    else
        FONT_Print(FONT_FRAN, TextColor, LCD_MakeRGB(0, 128, 0), 260, Y0 + WHEIGHT + 16 + 16,  atxt);*/
}

static void DrawBottomText(void)
{
   // static const char* txt = " Save snapshot ";
   // FONT_Write(FONT_FRAN, TextColor, BackGrColor, 165,
   //            Y0 + WHEIGHT + 16 + 16, txt);
    DrawFootText();
}

static void DrawSavingText(void)
{
    static const char* txt = "  Saving snapshot...  ";
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLUE, 165,
               Y0 + WHEIGHT + 16 + 16, txt);
    Sleep(20);
}

static void DrawSavedText(void)
{
    static const char* txt = "  Snapshot saved  ";
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_RGB(0, 60, 0), 165,
               Y0 + WHEIGHT + 16 + 16, txt);
    Sleep(2000);
    DrawFootText();
    DrawAutoText();
}

static void DecrCursor()
{
    if (!isMeasured)
        return;
    if (cursorPos == 0)
        return;
    DrawCursor();
    cursorPos--;
    DrawCursor();
    if ((grType == GRAPH_S11) && (CFG_GetParam(CFG_PARAM_S11_SHOW) == 1))
    {
        DrawCursorTextWithS11();
    }

    else
    {
        DrawCursorText();
    }
    if (cursorChangeCount++ < 10)
        Sleep(100); //Slow down at first steps
    Sleep(5);
}

static void IncrCursor()
{
    if (!isMeasured)
        return;
    if (cursorPos == WWIDTH)
        return;
    DrawCursor();
    cursorPos++;
    DrawCursor();
    if ((grType == GRAPH_S11) && (CFG_GetParam(CFG_PARAM_S11_SHOW) == 1))
    {
        DrawCursorTextWithS11();
    }

    else
    {
        DrawCursorText();
    }
    if (cursorChangeCount++ < 10)
        Sleep(100); //Slow down at first steps
    Sleep(5);
}

static void DrawGrid(GRAPHTYPE grType)  //
{
    int i;
    LCD_FillAll(BackGrColor);

    FONT_Write(FONT_FRAN, LCD_BLACK, LCD_PURPLE, X0+1, 0, modstr);
    //FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 2, 110, "<");
    //FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 460, 110, ">");
    uint32_t fstart;
    uint32_t pos = modstrw + 8+ X0;//WK
    if (grType == GRAPH_RX)// R/X
    {
        //  Print colored R/X
        FONT_Write(FONT_FRAN, CurvColor, BackGrColor, pos, 0, " R");
        pos += FONT_GetStrPixelWidth(FONT_FRAN, " R") + 1;
        FONT_Write(FONT_FRAN, TextColor, BackGrColor, pos, 0, "/");
        pos += FONT_GetStrPixelWidth(FONT_FRAN, "/") + 1;
        FONT_Write(FONT_FRAN, LCD_BLACK, LCD_RED, pos, 0, "X");
        pos += FONT_GetStrPixelWidth(FONT_FRAN, "X") + 1;
    }

    if (grType == GRAPH_S11)
    {
        //  Print colored S11
        FONT_Write(FONT_FRAN, CurvColor, BackGrColor, pos, 0, " S11");
        pos += FONT_GetStrPixelWidth(FONT_FRAN, "S11") + 6;
    }

    if (0 == CFG_GetParam(CFG_PARAM_PAN_CENTER_F))  {
        fstart = f1;

        if (grType == GRAPH_VSWR)
            sprintf(buf, "VSWR graph: %.3f MHz + %s   (Z0 = %d)", (float)f1/1000000, BSSTR[span], CFG_GetParam(CFG_PARAM_R0));
        else if (grType == 3)
            sprintf(buf, " VSWR/|X| graph: %.3f MHz +%s (Z0 = %d)", (float)f1/1000000, BSSTR[span], CFG_GetParam(CFG_PARAM_R0));
        else
            sprintf(buf, " graph: %.3f MHz +%s", (float)f1/1000000, BSSTR[span]);
    }

    else     {
        fstart = f1 - 500*BSVALUES[span];

        if (grType == GRAPH_VSWR)
            sprintf(buf, " VSWR graph: %.3f MHz +/- %s (Z0 = %d)", (float)f1/1000000, BSSTR_HALF[span], CFG_GetParam(CFG_PARAM_R0));
        else if (grType == 3)
            sprintf(buf, " VSWR/|X| graph: %.3f MHz +/- %s (Z0 = %d)", (float)f1/1000000, BSSTR_HALF[span], CFG_GetParam(CFG_PARAM_R0));
        else
            sprintf(buf, " graph: %.3f MHz +/- %s", (float)f1/1000000, BSSTR_HALF[span]);
    }

    FONT_Write(FONT_FRAN, TextColor, BackGrColor, pos, 0, buf);//LCD_BLUE

    //Mark ham bands with colored background
    for (i = 0; i <= WWIDTH; i++)
    {
        uint32_t f = fstart/1000 + (i * BSVALUES[span]) / WWIDTH;
        if (IsFinHamBands(f))
        {
            LCD_VLine(LCD_MakePoint(X0 + i, Y0), WHEIGHT, Color3);// (0, 0, 64) darkblue << >> yellow
        }
    }

    //Draw F grid and labels
    int lmod = 5;
    int linediv = 10; //Draw vertical line every linediv pixels

    for (i = 0; i <= WWIDTH/linediv; i++)
    {
        int x = X0 + i * linediv;
        if ((i % lmod) == 0 || i == WWIDTH/linediv)
        {
            char fr[10];
            float flabel = ((float)(fstart/1000. + i * BSVALUES[span] / (WWIDTH/linediv)))/1000.f;
            if (flabel * 1000000.f > (float)(CFG_GetParam(CFG_PARAM_BAND_FMAX)+1))
                continue;
            if(flabel>999.99)
                sprintf(fr, "%.1f", ((float)(fstart/1000. + i * BSVALUES[span] / (WWIDTH/linediv)))/1000.f);
            else if(flabel>99.99)
                sprintf(fr, "%.2f", ((float)(fstart/1000. + i * BSVALUES[span] / (WWIDTH/linediv)))/1000.f);
            else
                sprintf(fr, "%.3f", ((float)(fstart/1000. + i * BSVALUES[span] / (WWIDTH/linediv)))/1000.f);// WK
            int w = FONT_GetStrPixelWidth(FONT_SDIGITS, fr);
           // FONT_Write(FONT_SDIGITS, LCD_WHITE, LCD_BLACK, x - w / 2, Y0 + WHEIGHT + 5, f);// WK
            FONT_Write(FONT_FRAN, TextColor, BackGrColor, x -8 - w / 2, Y0 + WHEIGHT +3, fr);
            LCD_VLine(LCD_MakePoint(x, Y0), WHEIGHT, WGRIDCOLOR);
            LCD_VLine(LCD_MakePoint(x+1, Y0), WHEIGHT, WGRIDCOLOR);// WK
        }
        else
        {
            LCD_VLine(LCD_MakePoint(x, Y0), WHEIGHT, WGRIDCOLOR);
        }
    }

    if ((grType == GRAPH_VSWR)||(grType == GRAPH_VSWR_Z)||(grType == GRAPH_VSWR_RX))
    {
        if(loglog==0){
            //Draw SWR grid and labels
            static const float swrs[]  = { 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10., 13., 16., 20.};
            static const char labels[] = { 1,  0,   0,   0,    0,   1,  1,   0,  1,  1,  1,  0,  1,  0,  0,   1,   1,   1,   1 };
            static const int nswrs = sizeof(swrs) / sizeof(float);
            for (i = 0; i < nswrs; i++)
            {
                int yofs = swroffset(swrs[i]);
                if (labels[i])
                {
                    char s[10];
                    if((int)(10*swrs[i])%10==0){// WK
                       if(swrs[i]>9.0)
                        sprintf(s, "%d", (int)swrs[i]);
                       else
                        sprintf(s, " % d", (int)swrs[i]);
                    }
                    else
                        sprintf(s, "%.1f", swrs[i]);
                   // FONT_Write(FONT_SDIGITS, LCD_WHITE, LCD_BLACK, X0 - 15, WY(yofs) - 2, s);
                    FONT_Write(FONT_FRAN, CurvColor, BackGrColor, X0 - 21, WY(yofs) - 12, s);
                }
                LCD_HLine(LCD_MakePoint(X0, WY(yofs)), WWIDTH, WGRIDCOLOR);
            }
        }
        else{
            static const float swrsl[]  = { 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10., 13., 16., 20.};
            static const char labelsl[] = { 1,  1,   1,   0,    0,   1,  1,   0,  1,  0,  1,  0,  0,  0,  0,   1,   0,   0,   1 };
            static const int nswrsl = sizeof(swrsl) / sizeof(float);
            for (i = 0; i < nswrsl; i++)
            {
                int yofs = swroffset(14*log10f(swrsl[i])+1);
                if (labelsl[i])
                {
                    char s[10];
                    if((int)(10*swrsl[i])%10==0){// WK
                       if(swrsl[i]>9.0)
                        sprintf(s, "%d", (int)swrsl[i]);
                       else
                        sprintf(s, " % d", (int)swrsl[i]);
                    }
                    else
                        sprintf(s, "%.1f", swrsl[i]);
                   // FONT_Write(FONT_SDIGITS, LCD_WHITE, LCD_BLACK, X0 - 15, WY(yofs) - 2, s);
                    FONT_Write(FONT_FRAN, CurvColor, BackGrColor, X0 - 21, WY(yofs) - 12, s);
                }
                LCD_HLine(LCD_MakePoint(X0, WY(yofs)), WWIDTH, WGRIDCOLOR);
            }
        }
        /*LCD_FillRect((LCDPoint){0 ,155},(LCDPoint){X0 -22,215},BackGrColor);

        LCD_Rectangle((LCDPoint){0 ,155},(LCDPoint){X0 -22,215},CurvColor);
        FONT_Write(FONT_FRAN, CurvColor, BackGrColor, 4, 160, "Log");
        if(loglog==1)
            FONT_Write(FONT_FRAN, CurvColor, BackGrColor, 4, 190, "Log");*/
    }
    LCD_FillRect((LCDPoint){X0 ,Y0+WHEIGHT+1},(LCDPoint){X0 + WWIDTH+2,Y0+WHEIGHT+3},BackGrColor);
}

static void ScanRXFast(void)
{
    uint64_t i;
    uint32_t fstart;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - 500*BSVALUES[span];
   // fstart *= 1000; //Convert to Hz

    DSP_Measure(fstart, 1, 1, 1); //Fake initial run to let the circuit stabilize

    for(i = 0; i <= WWIDTH; i+=8)
    {
        uint32_t freq;
        freq = fstart + (i * BSVALUES[span] * 1000) / WWIDTH;
        if (freq == 0) //To overcome special case in DSP_Measure, where 0 is valid value
            freq = 1;
        DSP_Measure(freq, 1, 1, CFG_GetParam(CFG_PARAM_PAN_NSCANS));
        float complex rx = DSP_MeasuredZ();
        if (isnan(crealf(rx)) || isinf(crealf(rx)))
            rx = 0.0f + cimagf(rx) * I;
        if (isnan(cimagf(rx)) || isinf(cimagf(rx)))
            rx = crealf(rx) + 0.0fi;
        values[i] = rx;
        LCDPoint pt;
        if ((0 == (i % 32)) && TOUCH_Poll(&pt))
            break;
    }
    GEN_SetMeasurementFreq(0);
    isMeasured = 1;

    //Interpolate intermediate values
    for(i = 0; i <= WWIDTH; i++)
    {
        uint32_t fr = i % 8;
        if (0 == fr)
            continue;
        int fi0, fi1, fi2;
        if (i < 8)
        {
            fi0 = i - fr;
            fi1 = i + 8 - fr;
            fi2 = i + 16 - fr;
        }
        else
        {
            fi0 = i - 8 - fr;
            fi1 = i - fr;
            fi2 = i + (8 - fr);
        }
        float complex G0 = OSL_GFromZ(values[fi0], 50.f);
        float complex G1 = OSL_GFromZ(values[fi1], 50.f);
        float complex G2 = OSL_GFromZ(values[fi2], 50.f);
        float complex Gi = OSL_ParabolicInterpolation(G0, G1, G2, (float)fi0, (float)fi1, (float)fi2, (float)i);
        values[i] = OSL_ZFromG(Gi, 50.f);
    }
}

static uint32_t Fs, Fp;// in Hz
static float Cp, Rs;

static void ScanRX(int selector)
{
char str[15];
float complex rx, rx0;
float newX, oldX, MaxX, absX;
uint32_t i, k, sel, imax;
uint32_t fstart, freq1, deltaF;

    f1=CFG_GetParam(CFG_PARAM_PAN_F1);
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - 500*BSVALUES[span]; // 2;
    //fstart *= 1000; //Convert to Hz

    freq1=fstart-150000;
    if(freq1<100000)freq1=100000;
    DSP_Measure(freq1, 1, 1, 3); //Fake initial run to let the circuit stabilize
    rx0 = DSP_MeasuredZ();
    Sleep(20);

    DSP_Measure(freq1, 1, 1, 3);

    rx0 = DSP_MeasuredZ();

float r= fabsf(crealf(rx0));//calculate Cp (quartz)
float im= cimagf(rx0);
float xp=1.0f;
    if(im*im>0.0025)

        xp=im+r*(r/im);// else xp=im=-10000.0f;// ??

    Cp=-1/( 6.2832 *freq1* xp);
    if(selector==1)return;

    deltaF=(BSVALUES[span] * 1000) / WWIDTH;
    MaxX=0;
    sel=0;
    k=CFG_GetParam(CFG_PARAM_PAN_NSCANS);
    for(i = 0; i <= WWIDTH; i++)
    {
        if(i%40==0){
            FONT_Write(FONT_FRAN, LCD_BLACK, LCD_BLACK, 450, 0, "TS");// ??
            Sleep(50);
        }
        Sleep(10);
        freq1 = fstart + i * deltaF;
        if (freq1 == 0) //To overcome special case in DSP_Measure, where 0 is valid value
            freq1 = 1;
        DSP_Measure(freq1, 1, 1, k);
        rx = DSP_MeasuredZ();

        if (isnan(crealf(rx)) || isinf(crealf(rx))){
            if(i>0) rx = crealf(values[i-1]) + cimagf(rx) * I;
            else rx = 0.0f + cimagf(rx) * I;
        }
        if (isnan(cimagf(rx)) || isinf(cimagf(rx))){
            if(i>0) rx = crealf(rx) + cimagf(values[i-1]) * I;
            else rx = crealf(rx) + 99999.0f * I;
            if(sel==1){
                Fp=freq1;// first pole above Fs
                sel=3;
            }
        }
        else{
            newX=cimagf(rx);
            absX=fabsf(newX);
            if((sel==0)&&(newX>=0)&&(oldX<0)){// serial frequency of a quartz
                Fs=freq1;
                Rs=crealf(rx);
                sel=1;
            }
            else if ((sel==1)||(sel==2)){
                if(absX>MaxX){
                    MaxX=absX;
                    imax=i;
                    sel=2;// increasing X
                }
                else if(sel==2){
                    if((newX<MaxX)&&(i==imax+1)){// first decrease of X
                        Fp=freq1;
                        sel=3;
                    }
                }
            }
        }
        FONT_Write(FONT_FRAN, LCD_BLACK, LCD_BLACK, 450, 0, "dP");// ??
        Sleep(10);
        values[i] = rx;
        oldX=newX;
        LCD_SetPixel(LCD_MakePoint(X0 + i, 135), LCD_BLUE);// progress line
        LCD_SetPixel(LCD_MakePoint(X0 + i, 136), LCD_BLUE);
    }
    FONT_Write(FONT_FRAN, LCD_RED, LCD_BLACK, 420, 0, "     ");
    GEN_SetMeasurementFreq(0);
    isMeasured = 1;
}

//Calculates average R and X of SMOOTHWINDOW measurements around frequency
//In the beginning and the end of measurement data missing measurements are replaced
//with first and last measurement respectively.
static float complex SmoothRX(int idx, int useHighSmooth)
{
    int i;
    float complex sample;
    float resr = 0.0f;
    float resx = 0.0f;
    int smoothofs;
    int smoothwindow;
    if (useHighSmooth)
    {
        smoothofs = SMOOTHOFS_HI;
        smoothwindow = SMOOTHWINDOW_HI;
    }
    else
    {
        smoothofs = SMOOTHOFS;
        smoothwindow = SMOOTHWINDOW;
    }
    for (i = -smoothofs; i <= smoothofs; i++)
    {
        if ((idx + i) < 0)
            sample = values[0];
        else if ((idx + i) >= (WWIDTH - 1))
            sample = values[WWIDTH - 1];
        else
            sample  = values[idx + i];
        resr += crealf(sample);
        resx += cimagf(sample);
    }
    resr /= smoothwindow;
    resx /= smoothwindow;
    return resr + resx * I;
}
static uint32_t MinSWR;
static uint32_t MinIndex;

static void DrawVSWR(void)
{
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, X0 -46, Y0-12, "S");
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, X0 -50, Y0+18, "W");
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, X0 -46, Y0+48, "R");
    if (!isMeasured)
        return;
    MinSWR=0;
    MinIndex=9999;
    float MaxZ, MinZ, factorA, factorB;
    int lastoffset = 0;
    int lastoffset_sm = 0;
    int i, x;
    float swr_float, swr_float_sm;
    int offset_log, offset_log_sm;
    int offset;
    int offset_sm;
    for(i = 0; i <= WWIDTH; i++)
    {
        swr_float=DSP_CalcVSWR(values[i]);
        swr_float_sm = DSP_CalcVSWR(SmoothRX(i,  f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) ) ? 1 : 0));
        offset_log=14*log10f(swr_float)+1;
        if(loglog==1){
            offset=swroffset(offset_log);
            offset_sm=swroffset(14*log10f(swr_float_sm)+1);
        }
        else{
            offset=swroffset(swr_float);
            offset_sm=swroffset(swr_float_sm);
        }
        int x = X0 + i;
        if(WY(offset_sm)>MinSWR) {//offset
            MinSWR=WY(offset_sm);
            MinIndex=i;
        }
        if(i == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, WY(offset_sm)), CurvColor);
        }
        else
        {
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)), LCD_MakePoint(x, WY(offset_sm)), CurvColor);
            if(FatLines){
                LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)-1), LCD_MakePoint(x, WY(offset_sm)-1), CurvColor);
                LCD_Line(LCD_MakePoint(x - 2, WY(lastoffset_sm)-1), LCD_MakePoint(x-1, WY(offset_sm)-1), CurvColor);
                LCD_Line(LCD_MakePoint(x , WY(lastoffset_sm)-1), LCD_MakePoint(x+1, WY(offset_sm)+1), CurvColor);
            }
        }
        lastoffset = offset;
        lastoffset_sm = offset_sm;
    }
    cursorPos=MinIndex;
    DrawCursor();
    if(grType==GRAPH_VSWR_Z){
        float impedance;
        int yofs, yofs_sm;
        lastoffset = 0;
        lastoffset_sm = 0;
        MaxZ=0;
        MinZ=999999.;
        for(i = 0; i <= WWIDTH; i++)
        {
            impedance = cimagf(values[i])*cimagf(values[i])+crealf(values[i])*crealf(values[i]);
            impedance=sqrtf(impedance);
            if (impedance < MinZ)
                MinZ=impedance ;
            if (impedance > MaxZ)
                MaxZ=impedance ;
        }
        if(MinZ<2) MinZ=2;
        if(MaxZ/MinZ<2) {
            MaxZ=1.5f*MinZ;
            MinZ=0.5f*MinZ;
        }
        factorA=200./(MaxZ-MinZ);
        factorB=-200.*MinZ/(MaxZ-MinZ);
        for(i = 0; i <= WWIDTH; i++)
        {
            impedance = cimagf(values[i])*cimagf(values[i])+crealf(values[i])*crealf(values[i]);
            impedance=sqrtf(impedance);
            yofs=factorA*impedance+factorB;
            x = X0 + i;
            if(i == 0)
            {
                LCD_SetPixel(LCD_MakePoint(x, WY(yofs)), LCD_RED);
                LCD_SetPixel(LCD_MakePoint(x, WY(yofs)+1), LCD_RED);
            }
            else
            {
                LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)), LCD_MakePoint(x, WY(yofs)), LCD_RED);
                if(FatLines){
                    LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)+1), LCD_MakePoint(x, WY(yofs)+1), LCD_RED);
                    LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)+2), LCD_MakePoint(x, WY(yofs)+2), LCD_RED);
                }

            }
            lastoffset = yofs;
            lastoffset_sm = yofs_sm;
        }
    DrawX_Scale(MaxZ,  MinZ);
    FONT_Write(FONT_FRANBIG, LCD_RED, BackGrColor, X0 +405, Y0+40, "|Z|");
    }
    else if (grType==GRAPH_VSWR_RX){
        DrawRX(0,1);
    }
}

static void LoadBkups()
{
    //Load saved frequency and span values from config file
    uint32_t fbkup = CFG_GetParam(CFG_PARAM_PAN_F1);
    if (fbkup != 0 && fbkup >= BAND_FMIN && fbkup <= CFG_GetParam(CFG_PARAM_BAND_FMAX) && (fbkup % 100) == 0)
    {
        f1 = fbkup;
    }
    else
    {
        f1 = 14000000;
        CFG_SetParam(CFG_PARAM_PAN_F1, f1);
        CFG_SetParam(CFG_PARAM_PAN_SPAN, BS400);
        CFG_Flush();
    }

    int spbkup = CFG_GetParam(CFG_PARAM_PAN_SPAN);
    if (spbkup <= BS500M)// DL8MBY
    {
        span = (BANDSPAN)spbkup;
    }
    else
    {
        span = BS400;
        CFG_SetParam(CFG_PARAM_PAN_SPAN, span);
        CFG_Flush();
    }
}
/*
static void DrawHelp(void)
{
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 160,  20, "(Tap here to set F and Span)");
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 160, 110, "(Tap here change graph type)");
}*/

/*
   This function is based on:
   "Nice Numbers for Graph Labels" article by Paul Heckbert
   from "Graphics Gems", Academic Press, 1990
   nicenum: find a "nice" number approximately equal to x.
   Round the number if round=1, take ceiling if round=0
 */
static float nicenum(float x, int round)
{
    int expv;   /* exponent of x */
    float f;    /* fractional part of x */
    float nf;   /* nice, rounded fraction */

    expv = floorf(log10f(x));
    f = x / powf(10., expv);    /* between 1 and 10 */
    if (round)
    {
        if (f < 1.5)
            nf = 1.;
        else if (f < 3.)
            nf = 2.;
        else if (f < 7.)
            nf = 5.;
        else
            nf = 10.;
    }
    else
    {
        if (f <= 1.)
            nf = 1.;
        else if (f <= 2.)
            nf = 2.;
        else if (f <= 5.)
            nf = 5.;
        else
            nf = 10.;
    }
    return nf * powf(10., expv);
}

static void DrawS11()
{
    int i;
    int j;
    if (!isMeasured)
        return;
    //Find min value among scanned S11 to set up scale
    float minS11 = 0.f;
    for (i = 0; i <= WWIDTH; i++)
    {
        if (S11Calc(DSP_CalcVSWR(values[i])) < minS11)
            minS11 = S11Calc(DSP_CalcVSWR(values[i]));
    }

    if (minS11 < -60.f)
        minS11 = -60.f;

    int nticks = 14; //Max number of intermediate ticks of labels
    float range = nicenum(-minS11, 0);
    float d = nicenum(range / (nticks - 1), 1);
    float graphmin = floorf(minS11 / d) * d;
    float graphmax = 0.f;
    float grange = graphmax - graphmin;
    float nfrac = MAX(-floorf(log10f(d)), 0);  // # of fractional digits to show
    char str[20];
    if (nfrac > 4) nfrac = 4;
    sprintf(str, "%%.%df", (int)nfrac);             // simplest axis labels

    //Draw horizontal lines and labels
    int yofs = 0;
    int yofs_sm = 0;
    float labelValue;

#define S11OFFS(s11) ((int)roundf(((s11 - graphmin) * WHEIGHT) / grange) + 1)

    for (labelValue = graphmin; labelValue < graphmax + (.5 * d); labelValue += d)
    {
        sprintf(buf, str, labelValue); //Get label string in buf
        yofs = S11OFFS(labelValue);
        FONT_Write(FONT_FRAN, TextColor, BackGrColor,  X0 - 21, WY(yofs) - 12, buf);// FONT_SDIGITS WK
        if (roundf(labelValue) == 0)
            LCD_HLine(LCD_MakePoint(X0, WY(S11OFFS(0.f))), WWIDTH, WGRIDCOLOR);
        else
            LCD_HLine(LCD_MakePoint(X0, WY(yofs)), WWIDTH, WGRIDCOLOR);

    }
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, X0 +410, Y0,    "S ");
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, X0 +410, Y0+26, "1 ");
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, X0 +410, Y0+52, "1 ");
    uint16_t lasty = 0;
    int MaxJ, maxY=0;
    for(j = 0; j <= WWIDTH; j++)
    {
        int offset = roundf((WHEIGHT / (-graphmin)) * S11Calc(DSP_CalcVSWR(values[j])));

        uint16_t y = WY(offset + WHEIGHT);
        if (y > (WHEIGHT + Y0))
            y = WHEIGHT + Y0;
        int x = X0 + j;
        if(maxY<y){
            maxY=y;
            MaxJ=j;
        }
        if(j == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, y), CurvColor);
        }
        else
        {
            if(FatLines){
                LCD_Line(LCD_MakePoint(x - 1, lasty), LCD_MakePoint(x, y), CurvColor);// LCD_GREEN WK
                LCD_Line(LCD_MakePoint(x - 1, lasty+1), LCD_MakePoint(x, y+1), CurvColor);
            }
            LCD_Line(LCD_MakePoint(x , lasty), LCD_MakePoint(x+1, y), CurvColor);
        }
        lasty = y;
    }
    cursorPos=MaxJ;
    DrawCursor();
}

void DrawX_Scale(float MaxZ, float MinZ){
float labelValue, d, factorA, factorB;
int yofs;
char str[20];
int nticks = 7; //Max number of intermediate ticks of labels 8
float range_i = nicenum(MaxZ - MinZ, 0);
    d = nicenum(range_i / (nticks - 1), 1);
    float graphmin_i = floorf(MinZ / d) * d;
    float graphmax_i = MaxZ*0.95;//ceilf(MaxZ / d) * d;
    float grange_i = graphmax_i - graphmin_i;
    float nfrac_i = MAX(-floorf(log10f(d)), 0);  // # of fractional digits to show

    if (nfrac_i > 3) nfrac_i = 3;
    sprintf(str, "%%.%df", (int)nfrac_i);             // simplest axis labels

    //Draw  labels
    yofs = 0;

    factorA=200./(MaxZ-MinZ);
    factorB=-200.*MinZ/(MaxZ-MinZ);
    for (labelValue = graphmin_i; labelValue < graphmax_i + (.5 * d); labelValue += d)
    {
        if (graphmax_i >=10000 )
            sprintf(buf, "%.0f k", labelValue/1000);
        else
            sprintf(buf, str, labelValue); //Get label string in buf
        yofs=factorA*labelValue+factorB;

        FONT_Write(FONT_FRAN, LCD_RED, BackGrColor, 440, WY(yofs) - 12, buf);
    }

}

static void DrawRX(int SelQu, int SelEqu)// SelQu=1, if quartz measurement  SelEqu=1, if equal scales
{
#define LimitR 1999.f
    float LimitX;
    int i, imax;
    int x, RXX0;
    if (!isMeasured)
        return;
    //Find min and max values among scanned R and X to set up scale

    if(SelQu==0) {
        LimitX=LimitR;
        RXX0=X0;
    }
    else {
        LimitX= 99999.f;
        RXX0=21;
    }
    float minRXr = 1000000.f, minRXi = 1000000.f;
    float maxRXr = -1000000.f, maxRXi = -1000000.f;
    for (i = 0; i <= WWIDTH; i++)
    {
        if (crealf(values[i]) < minRXr)
            minRXr = crealf(values[i]);
        if (cimagf(values[i]) < minRXi)
            minRXi = cimagf(values[i]);
        if (crealf(values[i]) > maxRXr)
            maxRXr = crealf(values[i]);
        if (cimagf(values[i]) > maxRXi){
            maxRXi = cimagf(values[i]);
            if(cimagf(values[i+1])<=maxRXi)
                imax=i;
        }
    }


    if (minRXr < -LimitR)
        minRXr = -LimitR;
    if (maxRXr > LimitR)
        maxRXr = LimitR;

    if (minRXi < -LimitX)// 1999.f or 49999.f
        minRXi = -LimitX;
    if (maxRXi > LimitX)
        maxRXi = LimitX;

    if(SelEqu==1){
        if(maxRXr<maxRXi)
            maxRXr=maxRXi;
        else   maxRXi=maxRXr;
        if(minRXr>minRXi)
            minRXr=minRXi;
        else   minRXi=minRXr;
    }
    if(maxRXr-minRXr<40){
        maxRXr+=20;
        minRXr-=10;
    }
    if(maxRXi-minRXi<40){
        maxRXi+=20;
        minRXi-=10;
    }

    int nticks = 8; //Max number of intermediate ticks of labels
    float range_r = nicenum(maxRXr - minRXr, 0);

    float d = nicenum(range_r / (nticks - 1), 1);
    float graphmin_r = floorf(minRXr / d) * d;
    float graphmax_r = ceilf(maxRXr / d) * d;
    float grange_r = graphmax_r - graphmin_r;
    float nfrac_r = MAX(-floorf(log10f(d)), 0);  // # of fractional digits to show
    char str[20];
    if (nfrac_r > 4) nfrac_r = 4;
    sprintf(str, "%%.%df", (int)nfrac_r);             // simplest axis labels

    //Draw horizontal lines and labels
    int yofs = 0;
    int yofs_sm = 0;
    float labelValue;

#define RXOFFS(rx) ((int)roundf(((rx - graphmin_r) * WHEIGHT) / grange_r) + 1)
    int32_t RCurvColor;
    if(SelEqu==0)  RCurvColor = CurvColor;
    else RCurvColor = TextColor;
    for (labelValue = graphmin_r; labelValue < graphmax_r + (.5 * d); labelValue += d)
    {
        yofs = RXOFFS(labelValue);
        sprintf(buf, str, labelValue); //Get label string in buf
        if(SelEqu==0)// print only if we don't have equal scales
            FONT_Write(FONT_FRAN,RCurvColor, BackGrColor, 27, WY(yofs) - 12, buf);// WK
        if (roundf(labelValue) == 0)
            LCD_HLine(LCD_MakePoint(RXX0, WY(RXOFFS(0.f))), WWIDTH, WGRIDCOLORBR);
        else
            LCD_HLine(LCD_MakePoint(RXX0, WY(yofs)), WWIDTH, WGRIDCOLOR);
    }
    if(SelQu==0){
        FONT_Write(FONT_FRANBIG, RCurvColor, BackGrColor, RXX0 +412, Y0, "R");
        FONT_Write(FONT_FRANBIG, TextColor, LCD_RED, RXX0 +412, Y0+46, "X");
    }
    //Now draw R graph
    int lastoffset = 0;
    int lastoffset_sm = 0;

    for(i = 0; i <= WWIDTH; i++)
    {
        float r = crealf(values[i]);
        if (r < -LimitR)
            r = -LimitR;
        else if (r > LimitR)
            r = LimitR;
        yofs = RXOFFS(r);
        r = crealf(SmoothRX(i,  f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) ) ? 1 : 0));
        if (r < -LimitR)
            r = -LimitR;
        else if (r > LimitR)
            r = LimitR;
        yofs_sm = RXOFFS(r);
        x = RXX0 + i;
        if(i == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs)), RCurvColor);
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs_sm)), RCurvColor);
        }
        else
        {
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)), LCD_MakePoint(x, WY(yofs)), RCurvColor);
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)), LCD_MakePoint(x, WY(yofs_sm)), RCurvColor);
            if(FatLines){
                LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)+1), LCD_MakePoint(x, WY(yofs)+1), RCurvColor);// LCD_GREEN WK
                LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)+1), LCD_MakePoint(x, WY(yofs_sm)+1), RCurvColor);
            }
            LCD_Line(LCD_MakePoint(x , WY(lastoffset_sm)), LCD_MakePoint(x+1, WY(yofs_sm)), RCurvColor);
        }
        lastoffset = yofs;
        lastoffset_sm = yofs_sm;
    }
    float range_i = nicenum(maxRXi - minRXi, 0);
    d = nicenum(range_i / (nticks - 1), 1);
    float graphmin_i = floorf(minRXi / d) * d;
    float graphmax_i = ceilf(maxRXi / d) * d;
    float grange_i = graphmax_i - graphmin_i;
    float nfrac_i = MAX(-floorf(log10f(d)), 0);  // # of fractional digits to show

    if (nfrac_i > 4) nfrac_i = 4;
    sprintf(str, "%%.%df", (int)nfrac_i);             // simplest axis labels

    //Draw  labels
    yofs = 0;
    yofs_sm = 0;
   // draw right scale:
    for (labelValue = graphmin_i; labelValue < graphmax_i + (.5 * d); labelValue += d)
    {
        if (graphmax_i >=10000 )
            sprintf(buf, "%.0f k", labelValue/1000);
        else
            sprintf(buf, str, labelValue); //Get label string in buf

        yofs = ((int)roundf(((labelValue - graphmin_i) * WHEIGHT) / grange_i) + 1);

        FONT_Write(FONT_FRAN, LCD_RED, BackGrColor, 440, WY(yofs) - 12, buf);// WK
    }

    //Now draw X graph
    lastoffset = 0;
    lastoffset_sm = 0;
    for(i = 0; i <= WWIDTH; i++)
    {
        float ix = cimagf(values[i]);
        if (ix < -LimitX)
            ix = -LimitX;
        else if (ix > LimitX)
            ix = LimitX;

        yofs = ((int)roundf(((ix - graphmin_i) * WHEIGHT) / grange_i) + 1);

        ix = cimagf(SmoothRX(i,  f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) ) ? 1 : 0));
        if (ix < -LimitX)
            ix = -LimitX;
        else if (ix > LimitX)
            ix = LimitX;
        yofs_sm = ((int)roundf(((ix - graphmin_i) * WHEIGHT) / grange_i) + 1);
        x = RXX0 + i;
        if(i == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs)), LCD_RED);//LCD_RGB(SM_INTENSITY, 0, 0
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs_sm)), LCD_RED);
        }
        else
        {
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)), LCD_MakePoint(x, WY(yofs)), LCD_RED);
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)), LCD_MakePoint(x, WY(yofs_sm)), LCD_RED);
            if(FatLines){
                LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)+1), LCD_MakePoint(x, WY(yofs)+1), LCD_RED);// WK
                LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)+1), LCD_MakePoint(x, WY(yofs_sm)+1), LCD_RED);
            }
            LCD_Line(LCD_MakePoint(x , WY(lastoffset_sm)), LCD_MakePoint(x+1, WY(yofs_sm)), LCD_RED);

        }
        lastoffset = yofs;
        lastoffset_sm = yofs_sm;
    }
}

static void DrawSmith(void)
{
    int i;

    LCD_FillAll(BackGrColor);
    FONT_Write(FONT_FRAN, LCD_BLACK, LCD_PURPLE, 1, 0, modstr);
    if (0 == CFG_GetParam(CFG_PARAM_PAN_CENTER_F))
        sprintf(buf, "Smith chart: %.3f MHz + %s, red pt. is end. Z0 = %d.", (float)f1/1000000, BSSTR[span], CFG_GetParam(CFG_PARAM_R0));
    else
        sprintf(buf, "Smith chart: %.3f MHz +/- %s, red pt. is end. Z0 = %d.", (float)f1/1000000, BSSTR_HALF[span], CFG_GetParam(CFG_PARAM_R0));
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, modstrw + 10, 0, buf);

    SMITH_DrawGrid(cx0, cy0, smithradius, WGRIDCOLOR, BackGrColor, SMITH_R50 | SMITH_R25 | SMITH_R10 | SMITH_R100 | SMITH_R200 | SMITH_R500 |
                                 SMITH_J50 | SMITH_J100 | SMITH_J200 | SMITH_J25 | SMITH_J10 | SMITH_J500 | SMITH_SWR2 | SMITH_Y50);

    float r0f = (float)CFG_GetParam(CFG_PARAM_R0);


    SMITH_DrawLabels(TextColor, BackGrColor, SMITH_R10 | SMITH_R25 | SMITH_R50 | SMITH_R100 | SMITH_R200 | SMITH_R500 |
                                      SMITH_J10 | SMITH_J25 | SMITH_J50 | SMITH_J100 | SMITH_J200 | SMITH_J500);

    //Draw measured data
    if (isMeasured)
    {
        uint32_t lastx = 0;
        uint32_t lasty = 0;
        for(i = 0; i <= WWIDTH; i++)
        {
            float complex g = OSL_GFromZ(values[i], r0f);
            lastx = (uint32_t)roundf(cx0 + crealf(g) * smithradius);
            lasty = (uint32_t)roundf(cy0 - cimagf(g) * smithradius);
            SMITH_DrawG(i, g, CurvColor);
        }
        //Mark the end of sweep range with red cross
        SMITH_DrawGEndMark(LCD_RED);
    }
}

static void RedrawWindow()
{
    isSaved = 0;

    if ((grType == GRAPH_VSWR)||(grType == GRAPH_VSWR_Z)||(grType == GRAPH_VSWR_RX))
    {
        DrawGrid(GRAPH_VSWR);
        DrawVSWR();
        DrawCursor();
    }
    else if (grType == GRAPH_RX)
    {
        DrawGrid(GRAPH_RX);
        DrawRX(0,0);// 1
    }
    else if (grType == GRAPH_S11)
    {
        DrawGrid(GRAPH_S11);
        DrawS11();
        DrawCursor();
    }
    else
        DrawSmith();
    DrawCursor();
    if ((isMeasured) && (grType != GRAPH_S11))
    {
        DrawCursorText();
        DrawBottomText();
    //    DrawAutoText();
    }
    else if ((isMeasured) && (CFG_GetParam(CFG_PARAM_S11_SHOW) == 1) && (grType == GRAPH_S11))
    {
        DrawCursorTextWithS11();
        DrawBottomText();
  //      DrawAutoText();
    }
    else
    {
    DrawFootText();
    //DrawAutoText();
    }
}

static void save_snapshot(void)
{
    static const TCHAR *sndir = "/aa/snapshot";
    char path[64];
    char wbuf[256];
    char* fname = 0;
    uint32_t i = 0;
    FRESULT fr = FR_OK;

    if (!isMeasured || isSaved)
        return;

    //DrawSavingText();
    Date_Time_Stamp();

    fname = SCREENSHOT_SelectFileName();

    if(strlen(fname)==0) return;

    SCREENSHOT_DeleteOldest();
    if (CFG_GetParam(CFG_PARAM_SCREENSHOT_FORMAT))
        SCREENSHOT_SavePNG(fname);
    else
        SCREENSHOT_Save(fname);

    //Now write measured data to S1P file
    sprintf(path, "%s/%s.s1p", sndir, fname);
    FIL fo = { 0 };
    UINT bw;
    fr = f_open(&fo, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (FR_OK != fr)
        CRASHF("Failed to open file %s", path);
    if (CFG_S1P_TYPE_S_RI == CFG_GetParam(CFG_PARAM_S1P_TYPE))
    {
        sprintf(wbuf, "! Touchstone file by EU1KY antenna analyzer\r\n"
                "# MHz S RI R 50\r\n"
                "! Format: Frequency S-real S-imaginary (normalized to 50 Ohm)\r\n");
    }
    else // CFG_S1P_TYPE_S_MA
    {
        sprintf(wbuf, "! Touchstone file by EU1KY antenna analyzer\r\n"
                "# MHz S MA R 50\r\n"
                "! Format: Frequency S-magnitude S-angle (normalized to 50 Ohm, angle in degrees)\r\n");
    }
    fr = f_write(&fo, wbuf, strlen(wbuf), &bw);
    if (FR_OK != fr) goto CRASH_WR;

    uint32_t fstart;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - 500*BSVALUES[span];

    for (i = 0; i < WWIDTH; i++)
    {
        float complex g = OSL_GFromZ(values[i], 50.f);
        float fmhz = ((float)fstart/1000.f + (float)i * BSVALUES[span] / WWIDTH) / 1000.0f;
        if (CFG_S1P_TYPE_S_RI == CFG_GetParam(CFG_PARAM_S1P_TYPE))
        {
            sprintf(wbuf, "%.6f %.6f %.6f\r\n", fmhz, crealf(g), cimagf(g));
        }
        else // CFG_S1P_TYPE_S_MA
        {
            g = OSL_GtoMA(g); //Convert G to magnitude and angle in degrees
            sprintf(wbuf, "%.6f %.6f %.6f\r\n", fmhz, crealf(g), cimagf(g));
        }
        fr = f_write(&fo, wbuf, strlen(wbuf), &bw);
        if (FR_OK != fr) goto CRASH_WR;
    }
    f_close(&fo);

    isSaved = 1;
//    BSP_LCD_SelectLayer(0);
//    DrawSavedText();
//    BSP_LCD_SelectLayer(1);
    DrawSavedText();
    return;
CRASH_WR:
    CRASHF("Failed to write to file %s", path);
}
 #define XX0 190
 #define YY0 42

int TouchTest(){

     if (TOUCH_Poll(&pt)){
        if((pt.y <80)&&(pt.x >380)){
            // Upper right corner --> EXIT
            Beep(1);
            while(TOUCH_IsPressed());
            Sleep(100);
            return 99;
        }
        if(pt.x<(XX0-8)){// select the pressed field:
            Beep(1);
            if(pt.y<YY0+48) return 0;
            if(pt.y<YY0+96) return 1;
            if(pt.y<YY0+144) return 2;
            if(pt.y<YY0+192) return 3;
            return 4;
        }
     }
     return -1;
 }

//Scan R-50 / X in +/- 200 kHz range around measurement frequency with 10 kHz step, to draw a small graph besides the measurement
static int8_t lastR;// WK
static int8_t lastX;
static int rMax;
static int xMax;
static bool reverse1;
static float complex z200[21] = { 0 };

int Scan200(uint8_t line, int index1){

int touch;
 int32_t r;
 int32_t x;
 int8_t idx;
 int fq;// frequency in Hz
 if(multi_fr[line]==0) return -1;// nothing to do
    if(index1==0){
        rMax=0;
        xMax=0;
        for(idx=0;idx<21;idx++){
            fq = (int)multi_fr[line]*1000 + (idx - 10) * multi_bw[line]*50;
            touch=TouchTest();
            if(touch!=-1) return touch;
            if (fq > 0){
                GEN_SetMeasurementFreq(fq);
                Sleep(2);
                DSP_Measure(fq, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
                z200[idx] = DSP_MeasuredZ();
                r = (int32_t)crealf(z200[idx]);
                if(r<0) r=-r;
                if(rMax<r)rMax=r;
                x = (int32_t)cimagf(z200[idx]);
                if(x<0) x=-x;
                if(x>1000)x=1000;
                if(xMax<x) xMax=x;
            }
        }
        if(rMax<100)rMax=100;
        if(xMax<100)xMax=100;
        r=(int32_t)((crealf(z200[0])-50.0)*20.0/rMax);
        //if(r<0) r=-r;
        if(r>40) r=40;
        lastR=r;
        x=(int32_t)((cimagf(z200[0]))*16.0/xMax);
        if(x>16)x=16;
        if(x<-16) x=-16;
        lastX=x;
        r=(int32_t)(crealf(z200[10]));
        if(r>999) r=999;
        x=(int32_t)(cimagf(z200[10]));
        if(x>999) x=999;
        if(x<-999) x=-999;
        LCD_FillRect((LCDPoint){XX0+137, YY0 + line*48}, (LCDPoint){XX0+210, YY0 + 30 + line*48}, BackGrColor);
        FONT_Print(FONT_FRAN, TextColor, BackGrColor, XX0+138, 38 + 48*line, " %u Ohm", r);// r
        FONT_Print(FONT_FRAN, Color1, BackGrColor, XX0+138, 58 + 48*line, "%d *j Ohm", x);// x
        LCD_FillRect((LCDPoint){XX0-5, YY0-10  + line*48}, (LCDPoint){XX0+135, YY0 + 30 + line*48}, BackGrColor);
//        BSP_LCD_SelectLayer(BSP_LCD_GetActiveLayer());
        LCD_Line(LCD_MakePoint(XX0-5, YY0+10+48*line), LCD_MakePoint(XX0+135, YY0+10+48*line), Color2);
        LCD_Rectangle(LCD_MakePoint(XX0-5, YY0-10+48*line), LCD_MakePoint(XX0+135, YY0+30+48*line), Color2);
        LCD_Line(LCD_MakePoint(XX0+70, YY0+10+48*line), LCD_MakePoint(XX0+70, YY0+30+48*line), Color2);

    }
    else{
        touch=TouchTest();
        if(touch!=-1) return touch;
        r=(int)((crealf(z200[index1])-50.0)*20.0/rMax);// -50.0
        //if(r<0) r=-r;
        if(r>40) r=40;
        x=(int)((cimagf(z200[index1]))*16.0/xMax);
        if(x>16)x=16;
        if(x<-16) x=-16;
        if(index1!=0){
            if(reverse1){
                LCD_Line(LCD_MakePoint(XX0+index1*6, YY0+10+48*line-lastR), LCD_MakePoint(XX0+index1*6+5, YY0+10+48*line-r), TextColor);
                LCD_Line(LCD_MakePoint(XX0+index1*6, YY0+10+48*line-lastR-1), LCD_MakePoint(XX0+index1*6+5, YY0+10+48*line-r-1), TextColor);
                LCD_Line(LCD_MakePoint(XX0+index1*6, YY0+10+48*line-lastR+1), LCD_MakePoint(XX0+index1*6+5, YY0+10+48*line-r+1), TextColor);
            }
            LCD_Line(LCD_MakePoint(XX0+index1*6, YY0+10+48*line-lastX), LCD_MakePoint(XX0+index1*6+5, YY0+10+48*line-x), Color1);
            LCD_Line(LCD_MakePoint(XX0+index1*6, YY0+10+48*line-lastX-1), LCD_MakePoint(XX0+index1*6+5, YY0+10+48*line-x-1), Color1);
            LCD_Line(LCD_MakePoint(XX0+index1*6, YY0+10+48*line-lastX+1), LCD_MakePoint(XX0+index1*6+5, YY0+10+48*line-x+1), Color1);
            if(!reverse1){
                LCD_Line(LCD_MakePoint(XX0+index1*6, YY0+10+48*line-lastR), LCD_MakePoint(XX0+index1*6+5, YY0+10+48*line-r), TextColor);
                LCD_Line(LCD_MakePoint(XX0+index1*6, YY0+10+48*line-lastR-1), LCD_MakePoint(XX0+index1*6+5, YY0+10+48*line-r-1), TextColor);
                LCD_Line(LCD_MakePoint(XX0+index1*6, YY0+10+48*line-lastR+1), LCD_MakePoint(XX0+index1*6+5, YY0+10+48*line-r+1), TextColor);
            }
            lastR=r;
            lastX=x;
        }
    }
    return -1;
}


char str[6];
int i;

uint32_t freqx;// kHz

int ShowFreq(int indx){
uint32_t dp;
uint32_t mhz;
uint32_t bw1;

   if(indx>4) return -1;
    freqx=multi_fr[indx];
    bw1=multi_bw[indx];
    dp = (freqx % 1000) ;
    mhz = freqx / 1000;
    LCD_FillRect((LCDPoint){0, YY0-6 + indx*48}, (LCDPoint){XX0-6, FONT_GetHeight(FONT_FRANBIG)+ YY0-6 + indx*48}, BackGrColor);
    LCD_Rectangle((LCDPoint){2, YY0-10+48*indx}, (LCDPoint){XX0-8, YY0+30+48*indx}, LCD_BLACK);
    if(freqx==0) {
        LCD_FillRect((LCDPoint){4, YY0-10+48*indx}, (LCDPoint){XX0+229, YY0+32+48*indx}, BackGrColor);
        LCD_Rectangle((LCDPoint){2, YY0-10+48*indx}, (LCDPoint){XX0-8, YY0+30+48*indx}, TextColor);
        return -1;
    }
    LCD_FillRect((LCDPoint){XX0+230, YY0+5 + 48*indx}, (LCDPoint){XX0+288, YY0 + 20 + 48*indx}, BackGrColor);// clear bandwidth
    FONT_Print(FONT_FRAN, TextColor, BackGrColor, XX0+234, YY0+6 + 48*indx, "+-%u k", bw1/2);// bandwidth
    FONT_Print(FONT_FRANBIG, TextColor, BackGrColor, 4, YY0-6 + 48*indx, "%u.%03u", mhz, dp);// frequency
    return indx;
}

void ShowResult(int indx){

float VSWR;
float complex z0;

    if(ShowFreq(indx)==-1) return;// nothing to do
    GEN_SetMeasurementFreq(multi_fr[indx]*1000);
    Sleep(10);
    DSP_Measure(freqx*1000, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
    z0 = DSP_MeasuredZ();
    VSWR = DSP_CalcVSWR(z0);
    if(VSWR>99.0)
        sprintf(str, "%.0f", VSWR);
    else
        sprintf(str, "%.1f", VSWR);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 118, YY0-6 + indx*48, str);

}
uint32_t  GetFrequency(uint32_t f0){// fo in kHz
uint32_t fkhz=f0;
    if (PanFreqWindow(&fkhz, &span));
    return fkhz;
}

static bool  rqExitSWR;
static uint8_t SWRLimit;

void SWR_Exit(void){
    rqExitSWR=true;
}

static int muted;
static uint32_t ToneFreq;
static int ToneTrigger;

void SWR_Mute(void){
    if(muted==1){
        muted=0;
        SWRTone=1;//tone
        FONT_Write(FONT_FRANBIG, M_FGCOLOR, M_BGCOLOR, 198, 212, " Mute ");
        SWRLimit=1;
        UB_TIMER2_Init_FRQ(ToneFreq); //100...1000 Hz
        UB_TIMER2_Start();
    }
    else {
        muted=1;
        SWRTone=0;
        FONT_Write(FONT_FRANBIG, M_FGCOLOR, M_BGCOLOR, 198, 212, " Tone ");
    }
}

static void SWR_2(void){
    if(SWRLimit==3) LCD_Rectangle((LCDPoint){380, 210}, (LCDPoint){476, 244}, 0xffffff00);//yellow
    if(SWRLimit==2) {
        SWRLimit=1;
        LCD_Rectangle((LCDPoint){280, 210}, (LCDPoint){380, 244}, 0xffffff00);
    }
    else {
        SWRLimit=2;
        LCD_Rectangle((LCDPoint){280, 210}, (LCDPoint){380, 244}, 0xffff0000);
    }
    while(TOUCH_IsPressed());
    Sleep(50);
    SWRTone=1;
    muted=0;
    ToneTrigger=1;
}

static void SWR_3(void){
    if(SWRLimit==2) LCD_Rectangle((LCDPoint){280, 210}, (LCDPoint){380, 244}, 0xffffff00);
    if(SWRLimit==3){
        SWRLimit=1;
        LCD_Rectangle((LCDPoint){380, 210}, (LCDPoint){476, 244}, 0xffffff00);
    }
    else {
        SWRLimit=3;
        LCD_Rectangle((LCDPoint){380, 210}, (LCDPoint){476, 244}, 0xffff0000);// red
    }
    while(TOUCH_IsPressed());
    Sleep(50);
    SWRTone=1;
    muted=0;
    ToneTrigger=1;
}

static void ShowFr(void)
{
    char str[20];
    uint8_t i,j;
    unsigned int freq=(unsigned int)(CFG_GetParam(CFG_PARAM_MEAS_F) / 1000);
    sprintf(str, "F: %u MHz  ", freq);
    if(freq>999){// WK
        for(i=3;i<10;i++){
            if(str[i]==' ') break;// search space before "kHz"
        }
        for(j=i+3;j>i-4;j--){
           str[j+1]=str[j];
        }
        str[i-3]='.';
        str[i+5]=0;
    }
    LCD_FillRect(LCD_MakePoint(0, 60), LCD_MakePoint(200,115), BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 0, 60, str);// WK
}

static uint32_t fxs = 3600000ul; //Scan range start frequency, in Hz
static uint32_t fxkHzs;//Scan range start frequency, in kHz
static BANDSPAN *pBss;

void SWR_SetFrequency(void)
{
//    while(TOUCH_IsPressed()); WK
    fxs=CFG_GetParam(CFG_PARAM_MEAS_F);
    fxkHzs=fxs/1000;
    span=BS400;
    if (PanFreqWindow(&fxkHzs, (BANDSPAN*)&span))
        {
            //Span or frequency has been changed
            CFG_SetParam(CFG_PARAM_MEAS_F, fxkHzs*1000);
            f1=fxkHzs*1000;
           // span=(BANDSPAN)pBss;
        }
    CFG_Flush();
  //  redrawWindow = 1;
    Sleep(200);
    ShowFr();
    sFreq=1;
}

void SWR_SetFrequencyMuted(void){
int ToneSaved=SWRTone;
    SWRTone=0;
    SWR_SetFrequency();
    SWRTone=ToneSaved;
}

void setup_GPIO(void) // GPIO I Pin 2 for buzzer
{
GPIO_InitTypeDef gpioInitStructure;

  __HAL_RCC_GPIOI_CLK_ENABLE();
  gpioInitStructure.Pin = GPIO_PIN_2;
  gpioInitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  gpioInitStructure.Pull = GPIO_NOPULL;
  gpioInitStructure.Speed = GPIO_SPEED_MEDIUM;
  HAL_GPIO_Init(GPIOI, &gpioInitStructure);
  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, 0);
  //SWRTone=0;
}

static uint32_t freqChg;
uint8_t SWRTone=1;
//TEXTBOX_CTX_t SWR1_ctx;

void Tune_SWR_Proc(void){// -----------------------------------------------------------------------

GPIO_PinState OUTGpio;
char str[20];
float vswrf, vswrf_old, vswLogf, SwrDiff;
uint32_t width, vswLog=0, Timer;
uint32_t color1, vswr10, vsw_old, k=0;

    SWRLimit=1;
    ToneTrigger=0;
    setup_GPIO();// GPIO I Pin 2 for buzzer
    freqChg=0;
    rqExitSWR=false;
    SetColours();
    LCD_FillAll(BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 120, 10, "Tune SWR ");
    Sleep(1000);
    while(TOUCH_IsPressed());
    fxs=CFG_GetParam(CFG_PARAM_MEAS_F);
    fxkHzs=fxs/1000;
    ShowFr();
    TEXTBOX_InitContext(&SWR1);
    muted=0;// begin without mute
    SWRTone=1;//tone on
    UB_TIMER2_Init_FRQ((uint32_t)(400)); //100...1000 Hz
    UB_TIMER2_Start();

//HW calibration menu
    TEXTBOX_Append(&SWR1, (TEXTBOX_t*)tb_menuSWR);
    TEXTBOX_DrawContext(&SWR1);
for(;;)
    {
        Sleep(0); //for autosleep to work
        if (TEXTBOX_HitTest(&SWR1))
        {
            if (rqExitSWR)
            {
                SWRTone=0;
                //UB_TIMER2_Init_FRQ(1000);
                rqExitSWR=false;
                GEN_SetMeasurementFreq(0);
                return;
            }
            if(freqChg==1){
               ShowFr();
               freqChg=0;
            }
            Sleep(50);
        }
        k++;
        if(k>=5){
            k=0;
            DSP_Measure(fxkHzs*1000, 1, 1, CFG_GetParam(CFG_PARAM_MEAS_NSCANS));
            vswrf = DSP_CalcVSWR(DSP_MeasuredZ());
            SwrDiff=vswrf_old-vswrf;
            if(SwrDiff<0)SwrDiff=-SwrDiff;
            if((ToneTrigger==1)||(SwrDiff>0.01*vswrf)){// Difference more than 1 %
                ToneTrigger=0;
                vswrf_old=vswrf;
                vswr10=10.0*vswrf;
                if(SWRLimit==2){
                    if(vswr10>20) SWRTone=1;
                    else SWRTone=0;
                }
                if(SWRLimit==3){
                    if(vswr10>30) SWRTone=1;
                    else SWRTone=0;
                }
                vswLogf= 200.0*log10f(10.0*log10f(vswrf)+5.0);

                if(SWRTone==1){
                    ToneFreq= (uint32_t) (6.0*vswLogf-250.0);
                    UB_TIMER2_Init_FRQ(ToneFreq); //100...1000 Hz
                    UB_TIMER2_Start();
                }
                else UB_TIMER2_Stop();
                sprintf(str, "SWR: %.2f  ", vswrf);
                LCD_FillRect(LCD_MakePoint(200, 60), LCD_MakePoint(470,115), BackGrColor);
                FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 200, 60, str);
                width=(uint32_t)(3*vswLogf-400.0);
                if(width>479) width=479;
                if(vswrf<2.0) color1=0xff00ff00;
                else if(vswrf<3) color1=0xffffff00;
                else color1=0xffff0000;
                LCD_FillRect(LCD_MakePoint(0, 116), LCD_MakePoint(width,160), TextColor);
                LCD_FillRect(LCD_MakePoint(0, 161), LCD_MakePoint(width,205), color1);
                LCD_FillRect(LCD_MakePoint(width+1, 116), LCD_MakePoint(479,205), BackGrColor);
            }
        }
        Sleep(1);//5
    }
}

void MultiSWR_Proc(void){// WK ******************************************************************************
int redrawRequired = 0;
int touch;
int fx;// in kHz
uint32_t activeLayer;
int i,j;

    while(TOUCH_IsPressed());
    SetColours();
    SWRTone=0;
    reverse1=true;
    multi_fr[0]=CFG_GetParam(CFG_PARAM_MULTI_F1);//  in kHz
    multi_fr[1]=CFG_GetParam(CFG_PARAM_MULTI_F2);
    multi_fr[2]=CFG_GetParam(CFG_PARAM_MULTI_F3);//  in kHz
    multi_fr[3]=CFG_GetParam(CFG_PARAM_MULTI_F4);//  in kHz
    multi_fr[4]=CFG_GetParam(CFG_PARAM_MULTI_F5);//  in kHz
    multi_bwNo[0]=CFG_GetParam(CFG_PARAM_MULTI_BW1);
    multi_bwNo[1]=CFG_GetParam(CFG_PARAM_MULTI_BW2);
    multi_bwNo[2]=CFG_GetParam(CFG_PARAM_MULTI_BW3);
    multi_bwNo[3]=CFG_GetParam(CFG_PARAM_MULTI_BW4);
    multi_bwNo[4]=CFG_GetParam(CFG_PARAM_MULTI_BW5);
    if(multi_bwNo[0]>=5)
        multi_bw[0]=BSVALUES[multi_bwNo[0]];//  in kHz
    else  multi_bw[0] = 0;
    if(multi_bwNo[1]>=5)
        multi_bw[1]=BSVALUES[multi_bwNo[1]];//  in kHz
    else  multi_bw[1] = 0;
    if(multi_bwNo[2]>=5)
        multi_bw[2]=BSVALUES[multi_bwNo[2]];//  in kHz
    else  multi_bw[2] = 0;
    if(multi_bwNo[3]>=5)
        multi_bw[3]=BSVALUES[multi_bwNo[3]];//  in kHz
    else  multi_bw[3] = 0;
    if(multi_bwNo[4]>=5)
        multi_bw[4]=BSVALUES[multi_bwNo[4]];//  in kHz
    else  multi_bw[4] = 0;
    LCD_FillAll(BackGrColor);
    LCD_FillRect((LCDPoint){380,1}, (LCDPoint){476,35}, LCD_MakeRGB(255, 0, 0));
    LCD_Rectangle(LCD_MakePoint(420, 1), LCD_MakePoint(476, 35), BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, LCD_MakeRGB(255, 0, 0), 400, 2, "Exit");
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 5, 0, "MHz           SWR      R /");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor, 254, 0, "X");
//    LCD_ShowActiveLayerOnly();
    for(j=0;j<=4;j++){
        ShowFreq(j);// show stored frequencies and bandwidths
    }
    Sleep(500);

    for(;;){
        for(j=0;j<5;j++){
            if(j==0) reverse1=!reverse1;
            ShowResult(j);
            for(i=0;i<21;i++){
                touch=TouchTest();//if all fr[i]==0
                if(touch==-1) touch=Scan200(j,i);   // no touch
                if (touch==99){                     // Exit
                   LCD_FillAll(BackGrColor);
                   Sleep(100);
                   CFG_Flush();
                   return;
                }
                if(touch>=0){// new manual frequency input (touch = line 1..5
                    fx=GetFrequency(multi_fr[touch]);// manual frequency input
                    multi_fr[touch]=fx;
                    multi_bw[touch]=BSVALUES[span];
                    switch (touch){
                        case 0:
                            CFG_SetParam(CFG_PARAM_MULTI_F1, fx);
                            CFG_SetParam(CFG_PARAM_MULTI_BW1, span);
                            break;
                        case 1:
                            CFG_SetParam(CFG_PARAM_MULTI_F2, fx);
                            CFG_SetParam(CFG_PARAM_MULTI_BW2, span);
                            break;
                        case 2:
                            CFG_SetParam(CFG_PARAM_MULTI_F3, fx);
                            CFG_SetParam(CFG_PARAM_MULTI_BW3, span);
                            break;

                        case 3:
                            CFG_SetParam(CFG_PARAM_MULTI_F4, fx);
                            CFG_SetParam(CFG_PARAM_MULTI_BW4, span);
                            break;
                        case 4:
                            CFG_SetParam(CFG_PARAM_MULTI_F5, fx);
                            CFG_SetParam(CFG_PARAM_MULTI_BW5, span);
                            break;
                    }
                    ShowFreq(touch);// show new frequency and bandwidth
                    if(j<0) j=0;
                    break;
                }

            }
        }
    }
}

static int redrawRequired;
static int Switch;
static int Sel1,Sel2,Sel3;
static uint32_t Saving;
static int32_t FreqkHz;
static int32_t k;

void DrawFootText(void){
    TEXTBOX_DrawContext(&SWR1);
    LCD_Rectangle(LCD_MakePoint(0, 100),
        LCD_MakePoint(25, 128),M_FGCOLOR);
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 7, 105, "<");
    LCD_Rectangle(LCD_MakePoint(0, 132),
        LCD_MakePoint(25, 160),M_FGCOLOR);
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 7, 138, ">");

    if(Switch==0){
        TEXTBOX_SetText(&SWR1,1,"Menu2");
        TEXTBOX_SetText(&SWR1,2,"Store");
        TEXTBOX_SetText(&SWR1,5,"Auto(fast)");
        TEXTBOX_SetText(&SWR1,6,"Scan");
    }
    else{

        TEXTBOX_SetText(&SWR1,1,"Menu1");
        if(loglog==1) {
            TEXTBOX_SetText(&SWR1,5,"LogLog");
        }
        else {
            TEXTBOX_SetText(&SWR1,5,"Log");
        }
        TEXTBOX_SetText(&SWR1,6,"Frequency");
    }
    if((Saving==1)||(Sel1==1)||(Sel2==1)||(Sel3==1)){
        TEXTBOX_SetText(&SWR1,9,"M1");
        TEXTBOX_SetText(&SWR1,10,"M2");
        TEXTBOX_SetText(&SWR1,11,"M3");
        if(Sel1==1)
            LCD_Rectangle(LCD_MakePoint(454, 100), LCD_MakePoint(478, 125),0xffff0000);//  Red
        if(Sel2==1)
            LCD_Rectangle(LCD_MakePoint(454, 127), LCD_MakePoint(478, 152),0xffff0000);//  Red
        if(Sel3==1)
            LCD_Rectangle(LCD_MakePoint(454, 154), LCD_MakePoint(478, 179),0xffff0000);//  Red
    }
    else LCD_FillRect((LCDPoint){454,100}, (LCDPoint){479,180}, BackGrColor);// delete M1..M3 Buttons
}

void ZoomMinus(void){
    if(span>0){
        span--;
        isMeasured = 0;
        redrawRequired = 1;
    }
}
void ZoomPlus(void){
    if(span<BS500M){// DL8MBY
        span++;
        isMeasured = 0;
        redrawRequired = 1;
    }
}

void SWR_Exit0(void){//                     Button 0
    rqExitSWR=true;// exit program
}

void Switch_Menu(void){// Button 1
    if(Switch==0){// switch to menu 2
        Switch=1;
        TEXTBOX_SetText(&SWR1,1,"Menu1");
    }
    else{
        Switch=0;
        TEXTBOX_SetText(&SWR1,1,"Menu2");
    }
    Saving=0;
    redrawRequired = 1;
}

void Store(void){//             Button 2
int k;
    autofast=0;
    if(Switch==0){
        if(isMeasured==1){
            Saving=1;
            isStored=1;
        }
    }
    redrawRequired = 1;
}

void DiagType(void){//                      Button3
int k;
                                            // toggle Diagram Type
    if (grType == GRAPH_VSWR)
        grType = GRAPH_VSWR_Z;
    else if (grType == GRAPH_VSWR_Z)
        grType = GRAPH_VSWR_RX;
    else if (grType == GRAPH_VSWR_RX)
        grType = GRAPH_RX;
    else if ((grType == GRAPH_RX) && (CFG_GetParam(CFG_PARAM_S11_SHOW) == 1))
        grType = GRAPH_S11;
    else if ((grType == GRAPH_RX) && (CFG_GetParam(CFG_PARAM_S11_SHOW) == 0))
        grType = GRAPH_SMITH;
    else if (grType == GRAPH_S11)
        grType = GRAPH_SMITH;
    else
        grType = GRAPH_VSWR;
    redrawRequired = 1;

}

void Save_Snap(void){// Save Snap             Button4
    autofast=0;
    save_snapshot();
}

void Auto_Fast(void){// Auto(fast)            Button5
    if(Switch==0){
        if(autofast==0){
            autofast = 1;
        }
        else {
            autofast=0;
        }
    }
    else if(Switch==1){
        if(loglog==0) loglog=1;
        else loglog=0;
    redrawRequired=1;
    }
}

void Frequency(void){
    FreqkHz=f1/1000;
    if(PanFreqWindow(&FreqkHz, &span)) {
        //Span or frequency has been changed
        f1=1000*FreqkHz;
        isMeasured = 0;
    }
    redrawRequired = 1;
}

void Scan(void){// Scan  or  Frequency        Button6

    if((Switch==0)){
        if (0 == autofast) {
            FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 160, 100, "       Scanning...     ");
            ScanRX(0);
        }
        else  {
            autofast = 0;
        }

        redrawRequired = 1;
    }
    else{
       Frequency();
    }
}

void Mem1(void){//                                  M1
    if(Saving==1){
        memcpy(SavedValues1, values,(WWIDTH+1)*8);
        LCD_Rectangle(LCD_MakePoint(454, 100), LCD_MakePoint(479, 125),0xffff0000);//  Red
        Sel1=1;
        Saving=0;
    }
    else if((Saving==0)&&(Sel1==1)){//                       Recall
        memcpy(values, SavedValues1, (WWIDTH+1)*8);
    }
    redrawRequired = 1;
}
void Mem2(void){//                                  M2
   if(Saving==1){
        memcpy(SavedValues2, values,(WWIDTH+1)*8);
        LCD_Rectangle(LCD_MakePoint(454, 127), LCD_MakePoint(479, 152),0xffff0000);//  Red
        Sel2=1;
        Saving=0;
    }
    else if((Saving==0)&&(Sel2==1)){//                       Recall
        memcpy(values, SavedValues2, (WWIDTH+1)*8);
    }
    redrawRequired = 1;
}

void Mem3(void){//                                      M3

    if(Saving==1){
        memcpy(SavedValues3, values,(WWIDTH+1)*8);//    Save
        LCD_Rectangle(LCD_MakePoint(454, 154), LCD_MakePoint(479, 179),0xffff0000);//  Red
        Sel3=1;
        Saving=0;
    }
    else if((Saving==0)&&(Sel3==1)){//                       Recall
        memcpy(values, SavedValues3, (WWIDTH+1)*8);
    }
    redrawRequired = 1;
}
static void DrawHelp(void)
{
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 160,  20, "(Tap here to set F and Span)");
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 160, 110, "(Tap here to change graph type)");
}
static void wait(void){
    Sleep(200);
}

static const TEXTBOX_t tb_PANVSWR[] = {
   (TEXTBOX_t){ .x0 = 0, .y0 = 248, .text = "Exit", .font = FONT_FRAN, .width = 50, .height = 22, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = LCD_RED, .cb = (void(*)(void))SWR_Exit0, .cbparam = 1,.next = (void*)&tb_PANVSWR[1]},
   (TEXTBOX_t){.x0 = 52, .y0 = 248, .text = "Menu2", .font = FONT_FRAN,.width = 40, .height = 22, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Switch_Menu , .cbparam = 1, .next = (void*)&tb_PANVSWR[2] },
   (TEXTBOX_t){.x0 = 94, .y0 = 248, .text ="Store", .font = FONT_FRAN,.width = 54, .height = 22, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Store, .cbparam = 1, .next = (void*)&tb_PANVSWR[3] },
   (TEXTBOX_t){.x0 = 150, .y0 = 248, .text ="Diagram Type", .font = FONT_FRAN,.width = 76, .height = 22, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = DiagType , .cbparam = 1, .next = (void*)&tb_PANVSWR[4] },
   (TEXTBOX_t){.x0 = 230, .y0 = 248, .text ="Save Snapshot", .font = FONT_FRAN,.width = 96, .height = 22, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Save_Snap , .cbparam = 1, .next = (void*)&tb_PANVSWR[5] },
   (TEXTBOX_t){.x0 = 328, .y0 = 248, .text ="Auto (fast)", .font = FONT_FRAN,.width = 80, .height = 22, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Auto_Fast , .cbparam = 1, .next = (void*)&tb_PANVSWR[6] },
   (TEXTBOX_t){.x0 = 410, .y0 = 248, .text ="Scan", .font = FONT_FRAN,.width = 60, .height = 22, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Scan , .cbparam = 1, .next = (void*)&tb_PANVSWR[7] },
   (TEXTBOX_t){.x0 = 0, .y0 = 193, .text = "Z-", .font = FONT_FRAN,.width = 24, .height = 25, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = ZoomMinus , .cbparam = 1, .next = (void*)&tb_PANVSWR[8] },
   (TEXTBOX_t){.x0 = 454, .y0 = 193, .text = "Z+", .font = FONT_FRAN,.width = 24, .height = 25, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = ZoomPlus , .cbparam = 1, .next = (void*)&tb_PANVSWR[9] },
   (TEXTBOX_t){.x0 = 454, .y0 = 100, .text ="M1", .font = FONT_FRAN,.width = 24, .height = 25, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Mem1 , .cbparam = 1, .next = (void*)&tb_PANVSWR[10] },
   (TEXTBOX_t){.x0 = 454, .y0 = 127, .text ="M2", .font = FONT_FRAN,.width = 24, .height = 25, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Mem2 , .cbparam = 1, .next = (void*)&tb_PANVSWR[11] },
   (TEXTBOX_t){.x0 = 454, .y0 = 154, .text ="M3", .font = FONT_FRAN,.width = 24, .height = 25, .center = 1,
                 .border = 1, .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = Mem3 , .cbparam = 1, .next = (void*)&tb_PANVSWR[12] },
   (TEXTBOX_t){.type=TEXTBOX_TYPE_HITRECT,.x0 = 80, .y0 = 60, .text ="", .font = FONT_FRAN,.width = 320, .height = 120, .center = 0,
                 .border = 0, .fgcolor = 0, .bgcolor = 0, .cb = DiagType , .cbparam = 1,  },

};


void PANVSWR2_Proc(void)// **************************************************************************+*********
{
    BeepIsOn=1;//       ??
    autofast=0;
    Saving=0;
    redrawRequired=0;
    SetColours();
    Switch=0;// menu 1
    SWRTone=0;
    isStored=0;
    LCD_FillAll(BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 120, 100, "Panoramic scan mode");
    Sleep(1000);
    while(TOUCH_IsPressed());

    LoadBkups();
    grType = GRAPH_VSWR;

    if (0 == modstrw)
    {
        modstrw = FONT_GetStrPixelWidth(FONT_FRAN, modstr);
    }

    if (!isMeasured)
    {
        DrawGrid(1);
        DrawHelp();
        isSaved = 0;
    }
    else
        RedrawWindow();

    TEXTBOX_InitContext(&SWR1);

    TEXTBOX_Append(&SWR1, (TEXTBOX_t*)tb_PANVSWR);
    TEXTBOX_DrawContext(&SWR1);
    DrawFootText();
for(;;)
    {
        Sleep(0); //for autosleep to work
        if (TOUCH_Poll(&pt)) {
            if(pt.y<60) Frequency();//              Special Functions (invisible)

            else if((pt.x>=0)&&(pt.x<=24)){//       without beep
                if((pt.y>=100)&&(pt.y<=128))//       "<"
                    DecrCursor();
                else if((pt.y>=132)&&(pt.y<=160))//  ">"
                    IncrCursor();
                autofast=0;
                continue;
            }

        }
        if (TEXTBOX_HitTest(&SWR1))  {
            if (rqExitSWR) {
                rqExitSWR=false;
                while(TOUCH_IsPressed());
                autofast = 0;
                Sleep(100);
                return;
            }
        }

        if(redrawRequired!=0){
            RedrawWindow();
            DrawFootText();
            redrawRequired=0;
        }
        if(autofast==0)
            LCD_Rectangle(LCD_MakePoint(328, 248), LCD_MakePoint(408, 270),M_FGCOLOR);
        else {
            LCD_Rectangle(LCD_MakePoint(328, 248), LCD_MakePoint(408, 270),0xffff0000);//  Red
            ScanRXFast();
            redrawRequired=1;
        }
    }
}


TEXTBOX_CTX_t Quartz_ctx;
float C0;

void QuCalibrate(void){
    if(sFreq==0){
        FONT_Write(FONT_FRAN, TextColor, BackGrColor, 10, 50, "First set estimated Frequency ");
        Sleep(2000);
        return;
    }
    else    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 10, 50, "                                   ");
    if(span>BS1000) span=BS1000;// maximum: 1 MHz
    ScanRX(1);//only compute C0
    C0=Cp;
    sprintf(str, "C0 = %.2f pF", 1e12*C0);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 200, 100, str);
    sCalib=1;
    LCD_FillRect(LCD_MakePoint(10,180),LCD_MakePoint(440,215),BackGrColor);
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 10, 180, "Now insert the Quartz");
    Sleep(2000);
    TEXTBOX_InitContext(&Quartz_ctx);
    TEXTBOX_Append(&Quartz_ctx, (TEXTBOX_t*)tb_menuQuartz2);
    TEXTBOX_DrawContext(&Quartz_ctx);
}

void QuMeasure(void){
int i;
char str[10];
float Cs,Ls,Q;

    if(sFreq==0) {
        FONT_Write(FONT_FRAN, TextColor, BackGrColor, 10, 50, "First set estimated Frequency ");
        Sleep(2000);
        return;
    }
    else    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 10, 50, "                                   ");
    if(sCalib==0) {
        FONT_Write(FONT_FRAN, TextColor, BackGrColor, 10, 50, "First do Calibration without quartz");
        Sleep(2000);
        return;
    }
    else    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 10, 50, "                                         ");
    LCD_FillRect(LCD_MakePoint(10,180),LCD_MakePoint(440,215),BackGrColor);
    if(span>BS1000) span=BS1000;// maximum: 1 MHz
    //test((char) span);//Testpunkt
    ScanRX(0);
    Cp-=C0;
    sprintf(str, "Cp = %.2f  ", 1e12*Cp);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 200, 140, str);
    sprintf(str, "Fs = %d  ", Fs);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 200, 60, str);
    sprintf(str, "Fp = %d  ", Fp);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 200, 100, str);

    Sleep(2000);
    DrawGrid(GRAPH_RX);
    DrawRX(1,0);
    Sleep(2000);
    span=BS10;// second measurement
    f1=Fs;
    ScanRX(0);
    Cp-=C0;
    LCD_FillAll(BackGrColor);// LCD_BLACK WK
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 150, 20, "Quartz Data ");
    sprintf(str, "Fs = %d  ", Fs);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 20, 60, str);
    sprintf(str, "Fp = %d  ", Fp);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 240, 60, str);
    if(Fs!=0){
        sprintf(str, "Cp = %.2f pF ", 1e12*Cp);
        FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 240, 100, str);

        if(Fp!=0){
            Cs=2.0f*Cp*(Fp-Fs)/Fs;
            sprintf(str, "Cs = %.4f pF ", 1e12*Cs);
            FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 20, 100, str);
            Ls=1/(Cs*39.478f*Fs*Fs);
            sprintf(str, "Ls = %.1f mH ", 1e3*Ls);
            FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 20, 140, str);
            sprintf(str, "Rs = %.1f Ohm", Rs);
            FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 20, 180, str);
            Q=(1/Rs)*sqrtf(Ls/Cs);
            sprintf(str, "Q = %.0f   ", Q);
            FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 240, 180, str);
        }
    }
    Sleep(4000);
    TEXTBOX_InitContext(&Quartz_ctx);
    TEXTBOX_Append(&Quartz_ctx, (TEXTBOX_t*)tb_menuQuartz2);
    TEXTBOX_DrawContext(&Quartz_ctx);
    while(!TOUCH_IsPressed());
   // rqExitSWR=true;
}

void Quartz_proc(void){
char str[20];
uint32_t width, vswLog=0;
uint32_t k;

    if(span==0) span=BS1000;// +- 500 kHz
    sFreq=0;
    sCalib=0;
    freqChg=0;
    rqExitSWR=false;
    SetColours();
    LCD_FillAll(BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 150, 10, "Quartz Data ");
    Sleep(1000);
    while(TOUCH_IsPressed());
    fxs=CFG_GetParam(CFG_PARAM_MEAS_F);
    fxkHzs=fxs/1000;
    ShowFr();
    TEXTBOX_InitContext(&Quartz_ctx);

//HW calibration menu
    TEXTBOX_Append(&Quartz_ctx, (TEXTBOX_t*)tb_menuQuartz);
    TEXTBOX_DrawContext(&Quartz_ctx);
for(;;)
    {
        Sleep(0); //for autosleep to work
        if (TEXTBOX_HitTest(&Quartz_ctx))
        {
            if (rqExitSWR)
            {
                rqExitSWR=false;
                return;
            }
            if(freqChg==1){
               ShowFr();
               freqChg=0;
            }
            Sleep(50);
        }
        k++;
        if(k>=5){
            k=0;


        }
        Sleep(5);
    }
}
