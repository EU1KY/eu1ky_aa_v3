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

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

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

void Sleep(uint32_t nms);

typedef enum
{
    GRAPH_VSWR, GRAPH_RX, GRAPH_SMITH, GRAPH_S11
} GRAPHTYPE;

typedef struct
{
    uint32_t flo;
    uint32_t fhi;
} HAM_BANDS;

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

const char* BSSTR[] = {"200 kHz", "400 kHz", "800 kHz", "1.6 MHz", "2 MHz", "4 MHz", "8 MHz", "16 MHz", "20 MHz", "40 MHz", "80 MHz"};
const char* BSSTR_HALF[] = {"100 kHz", "200 kHz", "400 kHz", "800 kHz", "1 MHz", "2 MHz", "4 MHz", "8 MHz", "10 MHz", "20 MHz", "40 MHz"};
const uint32_t BSVALUES[] = {200, 400, 800, 1600, 2000, 4000, 8000, 16000, 20000, 40000, 80000};

static uint32_t f1 = 14000; //Scan range start frequency, in kHz
static BANDSPAN span = BS800;
static char buf[64];
static LCDPoint pt;
static float complex values[WWIDTH+1];
static int isMeasured = 0;
static uint32_t cursorPos = WWIDTH / 2;
static GRAPHTYPE grType = GRAPH_VSWR;
static uint32_t isSaved = 0;
static uint32_t cursorChangeCount = 0;
static uint32_t autofast = 0;
extern volatile uint32_t autosleep_timer;

static void DrawRX();
static void DrawSmith();
static float complex SmoothRX(int idx, int useHighSmooth);

static uint32_t fr[5]  = {0,0,27800,3670,0};//Multi SWR frequencies in kHz
static uint32_t bw[5]  = {200,800,200,400,200};//Multi SWR bandwidth in kHz

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
        if(ColourSelection==0){// Daylightcolours
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
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 2, 110, "<");
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 460, 110, ">");
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
        fstart = f1 - BSVALUES[span] / 2;

    float fcur = ((float)(fstart + (float)cursorPos * BSVALUES[span] / WWIDTH))/1000.;
    if (fcur * 1000000.f > (float)(CFG_GetParam(CFG_PARAM_BAND_FMAX) + 1))
        fcur = 0.f;

    float Q = 0.f;
    if ((crealf(rx) > 0.1f) && (fabs(cimagf(rx)) > crealf(rx)))
        Q = fabs(cimagf(rx) / crealf(rx));
    if (Q > 2000.f)
        Q = 2000.f;
    FONT_ClearLine(FONT_FRAN, BackGrColor, Y0 + WHEIGHT + 16);
    FONT_Print(FONT_FRAN, TextColor, BackGrColor, 60, Y0 + WHEIGHT + 16, "F: %.3f   Z: %.1f%+.1fj   SWR: %.1f   MCL: %.2f dB   Q: %.1f       ",
               fcur,
               crealf(rx),
               cimagf(rx),
               DSP_CalcVSWR(rx),
               (ga > 0.01f) ? (-10. * log10f(ga)) : 99.f, // Matched cable loss
               Q
              );
}

static void DrawCursorTextWithS11()
{
    float complex rx = values[cursorPos]; //SmoothRX(cursorPos, f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) / 1000) ? 1 : 0);
    float ga = cabsf(OSL_GFromZ(rx, (float)CFG_GetParam(CFG_PARAM_R0))); //G magnitude

    uint32_t fstart;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - BSVALUES[span] / 2;

    float fcur = ((float)(fstart + (float)cursorPos * BSVALUES[span] / WWIDTH))/1000.;
    if (fcur * 1000000.f > (float)(CFG_GetParam(CFG_PARAM_BAND_FMAX) + 1))
        fcur = 0.f;
    FONT_ClearLine(FONT_FRAN, BackGrColor, Y0 + WHEIGHT + 16);
    FONT_Print(FONT_FRAN, TextColor, BackGrColor, 60, Y0 + WHEIGHT + 16, "F: %.3f   Z: %.1f%+.1fj   SWR: %.1f   S11: %.2f dB          ",
               fcur,
               crealf(rx),
               cimagf(rx),
               DSP_CalcVSWR(rx),
               S11Calc(DSP_CalcVSWR(rx))
              );
}

static void DrawAutoText(void)
{
    static const char* atxt = "  Auto (fast, 1/8 pts)  ";
    if (0 == autofast)
        FONT_Print(FONT_FRAN, TextColor, BackGrColor, 250, Y0 + WHEIGHT + 16 + 16,  atxt);
    else
        FONT_Print(FONT_FRAN, TextColor, LCD_MakeRGB(0, 128, 0), 250, Y0 + WHEIGHT + 16 + 16,  atxt);
}

static void DrawBottomText(void)
{
    static const char* txt = " Save snapshot ";
    FONT_ClearLine(FONT_FRAN, BackGrColor, Y0 + WHEIGHT + 16 + 16);
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 145,
               Y0 + WHEIGHT + 16 + 16, txt);
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 2, 244, "Exit");
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 420, 244, "Scan");
}

static void DrawSavingText(void)
{
    static const char* txt = "  Saving snapshot...  ";
    FONT_ClearLine(FONT_FRAN, BackGrColor, Y0 + WHEIGHT + 16 + 16);
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLUE, 120,
               Y0 + WHEIGHT + 16 + 16, txt);
    Sleep(20);
}

static void DrawSavedText(void)
{
    static const char* txt = "  Snapshot saved  ";
    FONT_ClearLine(FONT_FRAN, BackGrColor, Y0 + WHEIGHT + 16 + 16);
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_RGB(0, 60, 0), 120,
               Y0 + WHEIGHT + 16 + 16, txt);
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 2, 244, "Exit");
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 420, 244, "Scan");
    DrawAutoText();
}

static void DecrCursor()
{
    if (!isMeasured)
        return;
    if (cursorPos == 0)
        return;
//    BSP_LCD_SelectLayer(0);
    DrawCursor();
//    BSP_LCD_SelectLayer(1);
 //   DrawCursor();
    cursorPos--;
 //   BSP_LCD_SelectLayer(0);
    DrawCursor();
 //   BSP_LCD_SelectLayer(1);
 //   DrawCursor();
    if ((grType == GRAPH_S11) && (CFG_GetParam(CFG_PARAM_S11_SHOW) == 1))
    {
      //  BSP_LCD_SelectLayer(0);
        DrawCursorTextWithS11();
     //   BSP_LCD_SelectLayer(1);
     //   DrawCursorTextWithS11();
    }

    else
    {
    //    BSP_LCD_SelectLayer(0);
        DrawCursorText();
     //   BSP_LCD_SelectLayer(1);
     //   DrawCursorText();
    }
//    LCD_ShowActiveLayerOnly();
    if (cursorChangeCount++ < 10)
        Sleep(100); //Slow down at first steps
    Sleep(5);
}

static void AdvCursor()
{
    if (!isMeasured)
        return;
    if (cursorPos == WWIDTH)
        return;
 //   BSP_LCD_SelectLayer(0);
  //  DrawCursor();
   // BSP_LCD_SelectLayer(1);
    DrawCursor();
    cursorPos++;
  //  BSP_LCD_SelectLayer(0);
    DrawCursor();
  /*  BSP_LCD_SelectLayer(1);
    DrawCursor();*/
    if ((grType == GRAPH_S11) && (CFG_GetParam(CFG_PARAM_S11_SHOW) == 1))
    {
      //  BSP_LCD_SelectLayer(0);
        DrawCursorTextWithS11();
      //  BSP_LCD_SelectLayer(1);
      //  DrawCursorTextWithS11();
    }

    else
    {
      //  BSP_LCD_SelectLayer(0);
        DrawCursorText();
       // BSP_LCD_SelectLayer(1);
       // DrawCursorText();
    }
 //   LCD_ShowActiveLayerOnly();
    if (cursorChangeCount++ < 10)
        Sleep(100); //Slow down at first steps
    Sleep(5);
}

static void DrawGrid(int drawSwr)  // drawSwr: 0 - R/X, 1 - VSWR, 2 - S11
{
    int i;
    LCD_FillAll(BackGrColor);// LCD_BLACK WK

    FONT_Write(FONT_FRAN, LCD_BLACK, LCD_PURPLE, X0+1, 0, modstr);

    uint32_t fstart;
    uint32_t pos = modstrw + 8+ X0;//WK
    if (drawSwr == 0)// R/X
    {
        //  Print colored R/X
        FONT_Write(FONT_FRAN, CurvColor, BackGrColor, pos, 0, " R");
        pos += FONT_GetStrPixelWidth(FONT_FRAN, " R") + 1;
        FONT_Write(FONT_FRAN, TextColor, BackGrColor, pos, 0, "/");
        pos += FONT_GetStrPixelWidth(FONT_FRAN, "/") + 1;
        FONT_Write(FONT_FRAN, LCD_BLACK, LCD_RED, pos, 0, "X");
        pos += FONT_GetStrPixelWidth(FONT_FRAN, "X") + 1;
    }

    if (drawSwr == 2)
    {
        //  Print colored S11
        FONT_Write(FONT_FRAN, CurvColor, BackGrColor, pos, 0, " S11");
        pos += FONT_GetStrPixelWidth(FONT_FRAN, "S11") + 6;
    }

    if (0 == CFG_GetParam(CFG_PARAM_PAN_CENTER_F))
    {
        fstart = f1;
        if (drawSwr == 1)
            sprintf(buf, " VSWR graph: %d kHz + %s   (Z0 = %d)", (int)fstart, BSSTR[span], CFG_GetParam(CFG_PARAM_R0));
        else
            sprintf(buf, " graph: %d kHz + %s", (int)fstart, BSSTR[span]);
    }
    else
    {
        fstart = f1 - BSVALUES[span] / 2;
        if (drawSwr == 1)
            sprintf(buf, " VSWR graph: %d kHz +/- %s   (Z0 = %d)", (int)f1, BSSTR_HALF[span], CFG_GetParam(CFG_PARAM_R0));
        else
            sprintf(buf, " graph: %d kHz +/- %s", (int)f1, BSSTR_HALF[span]);
    }

    FONT_Write(FONT_FRAN, TextColor, BackGrColor, pos, 0, buf);//LCD_BLUE

    //Mark ham bands with colored background
    for (i = 0; i <= WWIDTH; i++)
    {
        uint32_t f = fstart + (i * BSVALUES[span]) / WWIDTH;
        if (IsFinHamBands(f))
        {
            LCD_VLine(LCD_MakePoint(X0 + i, Y0), WHEIGHT, Color3);// (0, 0, 64) darkblue << >> yellow
        }
    }



    //Draw F grid and labels
 //   int lmod = (BS20M == span) || (BS40M == span) || (BS16M == span) || (BS1600 == span) ? 4 : 5;
    int lmod = 5;
    int linediv = 10; //Draw vertical line every linediv pixels

    for (i = 0; i <= WWIDTH/linediv; i++)
    {
        int x = X0 + i * linediv;
        if ((i % lmod) == 0 || i == WWIDTH/linediv)
        {
            char f[10];
            float flabel = ((float)(fstart + i * BSVALUES[span] / (WWIDTH/linediv)))/1000.f;
            if (flabel * 1000000.f > (float)(CFG_GetParam(CFG_PARAM_BAND_FMAX)+1))
                continue;
            if(flabel>99.99)
                sprintf(f, "%.2f", ((float)(fstart + i * BSVALUES[span] / (WWIDTH/linediv)))/1000.f);
            else
                sprintf(f, "%.3f", ((float)(fstart + i * BSVALUES[span] / (WWIDTH/linediv)))/1000.f);// WK
            int w = FONT_GetStrPixelWidth(FONT_SDIGITS, f);
           // FONT_Write(FONT_SDIGITS, LCD_WHITE, LCD_BLACK, x - w / 2, Y0 + WHEIGHT + 5, f);// WK
            FONT_Write(FONT_FRAN, TextColor, BackGrColor, x -8 - w / 2, Y0 + WHEIGHT +3, f);
            LCD_VLine(LCD_MakePoint(x, Y0), WHEIGHT, WGRIDCOLOR);
            LCD_VLine(LCD_MakePoint(x+1, Y0), WHEIGHT, WGRIDCOLOR);// WK
        }
        else
        {
            LCD_VLine(LCD_MakePoint(x, Y0), WHEIGHT, WGRIDCOLOR);
        }
    }

    if (drawSwr == 1)
    {
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
                FONT_Write(FONT_FRAN, TextColor, BackGrColor, X0 - 21, WY(yofs) - 12, s);
            }
            LCD_HLine(LCD_MakePoint(X0, WY(yofs)), WWIDTH, WGRIDCOLOR);
        }
    }
    LCD_FillRect((LCDPoint){X0 ,Y0+WHEIGHT+1},(LCDPoint){X0 + WWIDTH+2,Y0+WHEIGHT+3},BackGrColor);
}

static inline const char* bsstr(BANDSPAN bs)
{
    return BSSTR[bs];
}

static void print_span(BANDSPAN sp)
{
    sprintf(buf, "<<  <  Span: %s  >  >>", bsstr(sp));
    FONT_ClearLine(FONT_CONSBIG, BackGrColor, 50);
    FONT_Write(FONT_CONSBIG, LCD_BLUE, BackGrColor, 0, 50, buf);
}

static void print_f1(uint32_t f)
{
    sprintf(buf, "<<  <  %s: %d kHz  >  >>", CFG_GetParam(CFG_PARAM_PAN_CENTER_F) ? "Fc" : "F0", (int)f);
    FONT_ClearLine(FONT_CONSBIG, LCD_BLACK, 100);
    FONT_Write(FONT_CONSBIG, LCD_BLUE, LCD_BLACK, 0, 100, buf);
}

static void nextspan(BANDSPAN *sp)
{
    if (*sp == BS80M)
    {
        *sp = BS200;
    }
    else
    {
        *sp = (BANDSPAN)((int)*sp + 1);
    }
}

static void prevspan(BANDSPAN *sp)
{
    if (*sp == BS200)
    {
        *sp = BS80M;
    }
    else
    {
        *sp = (BANDSPAN)((int)*sp - 1);
    }
}

static void ScanRXFast(void)
{
    uint64_t i;
    uint32_t fstart;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - BSVALUES[span] / 2;
    fstart *= 1000; //Convert to Hz

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
        int f0, f1, f2;
        if (i < 8)
        {
            f0 = i - fr;
            f1 = i + 8 - fr;
            f2 = i + 16 - fr;
        }
        else
        {
            f0 = i - 8 - fr;
            f1 = i - fr;
            f2 = i + (8 - fr);
        }
        float complex G0 = OSL_GFromZ(values[f0], 50.f);
        float complex G1 = OSL_GFromZ(values[f1], 50.f);
        float complex G2 = OSL_GFromZ(values[f2], 50.f);
        float complex Gi = OSL_ParabolicInterpolation(G0, G1, G2, (float)f0, (float)f1, (float)f2, (float)i);
        values[i] = OSL_ZFromG(Gi, 50.f);
    }
}

static void ScanRX(void)
{
    uint64_t i;
    uint32_t fstart;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - BSVALUES[span] / 2;
    fstart *= 1000; //Convert to Hz

    DSP_Measure(fstart, 1, 1, 1); //Fake initial run to let the circuit stabilize

    for(i = 0; i <= WWIDTH; i++)
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
        LCD_SetPixel(LCD_MakePoint(X0 + i, 135), LCD_BLUE);// progress line
        LCD_SetPixel(LCD_MakePoint(X0 + i, 136), LCD_BLUE);
    }
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
    if (!isMeasured)
        return;
    MinSWR=0;
    MinIndex=9999;
    int lastoffset = 0;
    int lastoffset_sm = 0;
    int i;
    for(i = 0; i <= WWIDTH; i++)
    {
        int offset = swroffset(DSP_CalcVSWR(values[i]));
        int offset_sm = swroffset(DSP_CalcVSWR(SmoothRX(i,  f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) / 1000) ? 1 : 0)));
        int x = X0 + i;
        if(WY(offset)>MinSWR) {
            MinSWR=WY(offset);
            MinIndex=i;
        }
        if(i == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, WY(offset)), LCD_RGB(0, SM_INTENSITY, 0));
            LCD_SetPixel(LCD_MakePoint(x, WY(offset_sm)), CurvColor);// LCD_GREEN WK
        }
        else
        {
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)), LCD_MakePoint(x, WY(offset)), LCD_RGB(0, SM_INTENSITY,0));
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)), LCD_MakePoint(x, WY(offset_sm)), CurvColor);// LCD_GREEN
            if(FatLines){
                LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)-1), LCD_MakePoint(x, WY(offset_sm)-1), CurvColor);
                LCD_Line(LCD_MakePoint(x - 2, WY(lastoffset_sm)-1), LCD_MakePoint(x-1, WY(offset_sm)-1), CurvColor);
                LCD_Line(LCD_MakePoint(x , WY(lastoffset_sm)-1), LCD_MakePoint(x+1, WY(offset_sm)+1), CurvColor);
            }
        }
        lastoffset = offset;
        lastoffset_sm = offset_sm;
    }
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, X0 +410, Y0, "S");
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, X0 +406, Y0+30, "W");
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, X0 +410, Y0+60, "R");
    cursorPos=MinIndex;
    DrawCursor();
}

static void LoadBkups()
{
    //Load saved frequency and span values from config file
    uint32_t fbkup = CFG_GetParam(CFG_PARAM_PAN_F1);
    if (fbkup != 0 && fbkup >= BAND_FMIN/1000 && fbkup <= CFG_GetParam(CFG_PARAM_BAND_FMAX)/1000 && (fbkup % 100) == 0)
    {
        f1 = fbkup;
    }
    else
    {
        f1 = 14000;
        CFG_SetParam(CFG_PARAM_PAN_F1, f1);
        CFG_SetParam(CFG_PARAM_PAN_SPAN, BS800);
        CFG_Flush();
    }

    int spbkup = CFG_GetParam(CFG_PARAM_PAN_SPAN);
    if (spbkup <= BS80M)
    {
        span = (BANDSPAN)spbkup;
    }
    else
    {
        span = BS800;
        CFG_SetParam(CFG_PARAM_PAN_SPAN, span);
        CFG_Flush();
    }
}

static void DrawHelp(void)
{
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 160,  20, "(Tap here to set F and Span)");
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 160, 110, "(Tap here change graph type)");
}

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
        FONT_Write(FONT_FRAN, TextColor, BackGrColor,  X0 - 30, WY(yofs) - 12, buf);// FONT_SDIGITS WK
        if (roundf(labelValue) == 0)
            LCD_HLine(LCD_MakePoint(X0, WY(S11OFFS(0.f))), WWIDTH, WGRIDCOLOR);
        else
            LCD_HLine(LCD_MakePoint(X0, WY(yofs)), WWIDTH, WGRIDCOLOR);

    }
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, X0 +410, Y0,    "S ");
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, X0 +410, Y0+26, "1 ");
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, X0 +410, Y0+52, "1 ");
    uint16_t lasty = 0;
    for(j = 0; j <= WWIDTH; j++)
    {
        int offset = roundf((WHEIGHT / (-graphmin)) * S11Calc(DSP_CalcVSWR(values[j])));
        uint16_t y = WY(offset + WHEIGHT);
        if (y > (WHEIGHT + Y0))
            y = WHEIGHT + Y0;
        int x = X0 + j;
        if(j == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, y), LCD_GREEN);
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
}

static void DrawRX()
{
    int i;
    int x;
    if (!isMeasured)
        return;
    //Find min and max values among scanned R and X to set up scale
    float minRX = 1000000.f;
    float maxRX = -1000000.f;
    for (i = 0; i <= WWIDTH; i++)
    {
        if (crealf(values[i]) < minRX)
            minRX = crealf(values[i]);
        if (cimagf(values[i]) < minRX)
            minRX = cimagf(values[i]);
        if (crealf(values[i]) > maxRX)
            maxRX = crealf(values[i]);
        if (cimagf(values[i]) > maxRX)
            maxRX = cimagf(values[i]);
    }

    if (minRX < -1999.f)
        minRX = -1999.f;
    if (maxRX > 1999.f)
        maxRX = 1999.f;

    int nticks = 8; //Max number of intermediate ticks of labels
    float range = nicenum(maxRX - minRX, 0);
    float d = nicenum(range / (nticks - 1), 1);
    float graphmin = floorf(minRX / d) * d;
    float graphmax = ceilf(maxRX / d) * d;
    float grange = graphmax - graphmin;
    float nfrac = MAX(-floorf(log10f(d)), 0);  // # of fractional digits to show
    char str[20];
    if (nfrac > 4) nfrac = 4;
    sprintf(str, "%%.%df", (int)nfrac);             // simplest axis labels

    //Draw horizontal lines and labels
    int yofs = 0;
    int yofs_sm = 0;
    float labelValue;

#define RXOFFS(rx) ((int)roundf(((rx - graphmin) * WHEIGHT) / grange) + 1)
    for (labelValue = graphmin; labelValue < graphmax + (.5 * d); labelValue += d)
    {
        sprintf(buf, str, labelValue); //Get label string in buf
        yofs = RXOFFS(labelValue);
        FONT_Write(FONT_FRAN,TextColor, BackGrColor, X0 - 30, WY(yofs) - 12, buf);// WK
        if (roundf(labelValue) == 0)
            LCD_HLine(LCD_MakePoint(X0, WY(RXOFFS(0.f))), WWIDTH, WGRIDCOLORBR);
        else
            LCD_HLine(LCD_MakePoint(X0, WY(yofs)), WWIDTH, WGRIDCOLOR);
    }
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, X0 +410, Y0, "R");
    FONT_Write(FONT_FRANBIG, TextColor, LCD_RED, X0 +410, Y0+46, "X");
    //Now draw R graph
    int lastoffset = 0;
    int lastoffset_sm = 0;
    for(i = 0; i <= WWIDTH; i++)
    {
        float r = crealf(values[i]);
        if (r < -1999.f)
            r = -1999.f;
        else if (r > 1999.f)
            r = 1999.f;
        yofs = RXOFFS(r);
        r = crealf(SmoothRX(i,  f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) / 1000) ? 1 : 0));
        if (r < -1999.f)
            r = -1999.f;
        else if (r > 1999.f)
            r = 1999.f;
        yofs_sm = RXOFFS(r);
        x = X0 + i;
        if(i == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs)), LCD_RGB(0, SM_INTENSITY, 0));
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs_sm)), LCD_GREEN);
        }
        else
        {
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)), LCD_MakePoint(x, WY(yofs)), LCD_RGB(0, SM_INTENSITY, 0));
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)), LCD_MakePoint(x, WY(yofs_sm)), CurvColor);
            if(FatLines){
                LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)+1), LCD_MakePoint(x, WY(yofs)+1), CurvColor);// LCD_GREEN WK
                LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)+1), LCD_MakePoint(x, WY(yofs_sm)+1), CurvColor);
            }
            LCD_Line(LCD_MakePoint(x , WY(lastoffset_sm)), LCD_MakePoint(x+1, WY(yofs_sm)), CurvColor);
        }
        lastoffset = yofs;
        lastoffset_sm = yofs_sm;
    }

    //Now draw X graph
    lastoffset = 0;
    lastoffset_sm = 0;
    for(i = 0; i <= WWIDTH; i++)
    {
        float ix = cimagf(values[i]);
        if (ix < -1999.f)
            ix = -1999.f;
        else if (ix > 1999.f)
            ix = 1999.f;
        yofs = RXOFFS(ix);
        ix = cimagf(SmoothRX(i,  f1 > (CFG_GetParam(CFG_PARAM_BAND_FMAX) / 1000) ? 1 : 0));
        if (ix < -1999.f)
            ix = -1999.f;
        else if (ix > 1999.f)
            ix = 1999.f;
        yofs_sm = RXOFFS(ix);
        x = X0 + i;
        if(i == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs)), LCD_RGB(SM_INTENSITY, 0, 0));
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs_sm)), LCD_RED);
        }
        else
        {
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)), LCD_MakePoint(x, WY(yofs)), LCD_RGB(SM_INTENSITY, 0, 0));
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
        sprintf(buf, "Smith chart: %d kHz + %s, red pt. is end. Z0 = %d.", (int)f1, BSSTR[span], CFG_GetParam(CFG_PARAM_R0));
    else
        sprintf(buf, "Smith chart: %d kHz +/- %s, red pt. is end. Z0 = %d.", (int)f1, BSSTR_HALF[span], CFG_GetParam(CFG_PARAM_R0));
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

    if (grType == GRAPH_VSWR)
    {
        DrawGrid(1);
        DrawVSWR();
        DrawCursor();
    }
    else if (grType == GRAPH_RX)
    {
        DrawGrid(0);
        DrawRX();
    }
    else if (grType == GRAPH_S11)
    {
        DrawGrid(2);
        DrawS11();
    }
    else
        DrawSmith();
    DrawCursor();
    if ((isMeasured) && (grType != GRAPH_S11))
    {
        DrawCursorText();
        DrawBottomText();
        DrawAutoText();
    }
    else if ((isMeasured) && (CFG_GetParam(CFG_PARAM_S11_SHOW) == 1) && (grType == GRAPH_S11))
    {
        DrawCursorTextWithS11();
        DrawBottomText();
        DrawAutoText();
    }
    else
    {
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 2, 244, "Exit");
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 420, 244, "Scan");
    //    FONT_Print(FONT_FRAN, LCD_MakeRGB(255, 255, 0), LCD_MakeRGB(0, 0, 128), 5, Y0 + WHEIGHT + 16 + 16, "  Exit  ");
    //    FONT_Print(FONT_FRAN, LCD_MakeRGB(255, 255, 0), LCD_MakeRGB(0, 0, 128), 400, Y0 + WHEIGHT + 16 + 16, "  Scan  ");
        DrawAutoText();
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

    DrawSavingText();

    fname = SCREENSHOT_SelectFileName();
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
        fstart = f1 - BSVALUES[span] / 2;

    for (i = 0; i < WWIDTH; i++)
    {
        float complex g = OSL_GFromZ(values[i], 50.f);
        float fmhz = ((float)fstart + (float)i * BSVALUES[span] / WWIDTH) / 1000.0f;
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

int8_t TouchTest(){

     if (TOUCH_Poll(&pt)){
        if((pt.y <40)&&(pt.x >420)){
            // Upper right corner
            while(TOUCH_IsPressed());
            Sleep(100);
            return 99;
        }
        if(pt.x<(XX0-8)){
            if(pt.y<YY0+48) return 1;
            if(pt.y<YY0+96) return 2;
            if(pt.y<YY0+144) return 3;
            if(pt.y<YY0+192) return 4;
            return 5;
        }
     }
     return 0;
 }

//Scan R-50 / X in +/- 200 kHz range around measurement frequency with 10 kHz step, to draw a small graph besides the measurement
static int8_t lastR;// WK
static int8_t lastX;
static int rMax;
static int xMax;
static bool reverse1;
static float complex z200[21] = { 0 };

uint8_t Scan200(uint8_t line, int index1){

uint8_t touch;
 int32_t r;
 int32_t x;
 int8_t idx;
 int fq;// frequency in Hz
 if(fr[line]==0) return 0;
    if(index1==0){
        rMax=0;
        xMax=0;
        for(idx=0;idx<21;idx++){
            fq = (int)fr[line]*1000 + (idx - 10) * bw[line]*50;
            touch=TouchTest();
            if(touch!=0) return touch;
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
        if(touch!=0) return touch;
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
    return 0;
}

void ShowResult(uint8_t indx){
float VSWR;
float complex z0;
char str[6];
uint8_t i;
uint32_t dp;
uint32_t mhz;
uint32_t freqx;
uint32_t bw1;

    if(indx>4) return;
    freqx=fr[indx];
    bw1=bw[indx];
    dp = (freqx % 1000) ;
    mhz = freqx / 1000;
    LCD_FillRect((LCDPoint){0, YY0-6 + indx*48}, (LCDPoint){XX0-6, FONT_GetHeight(FONT_FRANBIG)+ YY0-6 + indx*48}, BackGrColor);
    LCD_Rectangle((LCDPoint){2, YY0-10+48*indx}, (LCDPoint){XX0-8, YY0+30+48*indx}, LCD_BLACK);
    if(freqx==0) {
        LCD_FillRect((LCDPoint){4, YY0-10+48*indx}, (LCDPoint){XX0+229, YY0+32+48*indx}, BackGrColor);
        LCD_Rectangle((LCDPoint){2, YY0-10+48*indx}, (LCDPoint){XX0-8, YY0+30+48*indx}, TextColor);
        return;
    }
    LCD_FillRect((LCDPoint){XX0+230, YY0+5 + 48*indx}, (LCDPoint){XX0+288, YY0 + 20 + 48*indx}, BackGrColor);// clear bandwidth
    FONT_Print(FONT_FRAN, TextColor, BackGrColor, XX0+234, YY0+6 + 48*indx, "+-%u k", bw1/2);// bandwidth
    FONT_Print(FONT_FRANBIG, TextColor, BackGrColor, 4, YY0-6 + 48*indx, "%u.%03u", mhz, dp);// frequency
    GEN_SetMeasurementFreq(fr[indx]*1000);
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
uint32_t  GetFrequency(uint32_t f0){
f1=f0;
if (PanFreqWindow(&f1, &span))
    return f1;
}

void MultiSWR_Proc(void){// WK
int redrawRequired = 0;
uint8_t touch;
uint32_t activeLayer;
uint8_t i,j;

    while(TOUCH_IsPressed());
    SetColours();
    reverse1=true;
    fr[0]=CFG_GetParam(CFG_PARAM_MEAS_F)/1000;// now in kHz
    fr[1]=CFG_GetParam(CFG_PARAM_PAN_F1);
 //   BSP_LCD_SelectLayer(0);
 //   LCD_FillAll(BackGrColor);
 //   BSP_LCD_SelectLayer(1);
    LCD_FillAll(BackGrColor);
    LCD_FillRect((LCDPoint){421,1}, (LCDPoint){476,35}, LCD_MakeRGB(255, 0, 0));
    LCD_Rectangle(LCD_MakePoint(420, 1), LCD_MakePoint(476, 35), BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, LCD_MakeRGB(255, 0, 0), 428, 2, "Exit");
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 5, 0, "MHz           SWR      R /");
    FONT_Write(FONT_FRANBIG, Color1, BackGrColor, 254, 0, "X");
//    LCD_ShowActiveLayerOnly();
    Sleep(500);

    for(;;){
        for(j=0;j<5;j++){
            if(j==0) reverse1=!reverse1;
            ShowResult(j);
            for(i=0;i<21;i++){
                touch=TouchTest();//if all fr[i]==0
                if(touch==0) touch=Scan200(j,i);
                if (touch==99){
                   LCD_FillAll(BackGrColor);
                   Sleep(100);
                   return;
                }
                if(touch>0){
                    fr[touch-1]=GetFrequency(fr[touch-1]);// manual frequency input
                    if (rqDel==1) {
                        rqDel= 0;
                        fr[touch-1]=0;
                    }
                    bw[touch-1]=BSVALUES[span];//span?
                    j=touch-2;
                    if(j<0) j=0;
                    break;
                }

            }
        }
    }
}

void PANVSWR2_Proc(void)
{
int redrawRequired = 0;
uint32_t activeLayer;

    SetColours();
    LCD_FillAll(BackGrColor);
    FONT_Write(FONT_FRANBIG, TextColor, BackGrColor, 120, 100, "Panoramic scan mode");
    Sleep(1000);
    while(TOUCH_IsPressed());

    LoadBkups();

    grType = GRAPH_VSWR;
    if (!isMeasured)
    {
        isSaved = 0;
    }
    if (0 == modstrw)
    {
        modstrw = FONT_GetStrPixelWidth(FONT_FRAN, modstr);
    }

        if (!isMeasured)
        {
            DrawGrid(1);
            DrawHelp();
        }
        else
            RedrawWindow();
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 2, 244, "Exit");
    FONT_Write(FONT_FRANBIG, CurvColor, BackGrColor, 420, 244, "Scan");

        DrawAutoText();

    for(;;)
    {
        Sleep(5);

        activeLayer = BSP_LCD_GetActiveLayer();
 /*       BSP_LCD_SelectLayer(!activeLayer);

        if (autofast && (cursorChangeCount == 0))
        {
            ScanRXFast();
            RedrawWindow();
            autosleep_timer = 30000; //CFG_GetParam(CFG_PARAM_LOWPWR_TIME);
        }
        LCD_ShowActiveLayerOnly(); */

        BSP_LCD_SelectLayer(activeLayer);
        if (TOUCH_Poll(&pt))
        {
            if (pt.y < 80)
            {
                // Top
                if (PanFreqWindow(&f1, &span))
                {
                    //Span or frequency has been changed
                    isMeasured = 0;
                    redrawRequired = 1;
                }
            }
            else if (pt.y > 90 && pt.y <= 170)
            {
                if (pt.x < 50)
                {
                    DecrCursor();
                    continue;
                }
                else if (pt.x > 70 && pt.x < 410)
                {
                    if (grType == GRAPH_VSWR)
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
                else if (pt.x > 430)
                {
                    AdvCursor();
                    continue;
                }
            }
            else if (pt.y > 200)
            {
                if (pt.x < 100)
                {
                    // Lower left corner
                    while(TOUCH_IsPressed());
                    autofast = 0;
                    Sleep(100);
                    return;
                }
                if (pt.x > 380)
                { //Lower right corner: perform scan or turn off auto
                    if (0 == autofast)
                    {
                        FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 180, 100, "  Scanning...  ");
                        ScanRX();
                        redrawRequired = 1;
                    }
                    else
                    {
                        autofast = 0;
                        redrawRequired = 1;
                        RedrawWindow();
                    }

                }
                else if (pt.x > 120 && pt.x < 240 && isMeasured && !isSaved)
                {
                    save_snapshot();
                }
                else if (pt.x >= 250 && pt.x <= 370)
                {
                    autofast = !autofast;
                    redrawRequired = 1;
                }
            }
            if(redrawRequired)
            {
                BSP_LCD_SelectLayer(!activeLayer);
                RedrawWindow();
                BSP_LCD_SelectLayer(activeLayer);
                RedrawWindow();
            }
            while(TOUCH_IsPressed())
            {
                Sleep(250);
            }
        }
        else
        {
            cursorChangeCount = 0;
        }



        if (autofast && (cursorChangeCount == 0))
        {
            activeLayer = BSP_LCD_GetActiveLayer();
            BSP_LCD_SelectLayer(!activeLayer);
            ScanRXFast();
            RedrawWindow();
            autosleep_timer = 30000; //CFG_GetParam(CFG_PARAM_LOWPWR_TIME);
        }
        LCD_ShowActiveLayerOnly();
    }
}
