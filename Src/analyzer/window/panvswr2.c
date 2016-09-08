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

#define X0 40
#define Y0 20
#define WWIDTH  400
#define WHEIGHT 200
#define WY(offset) ((WHEIGHT + Y0) - (offset))
#define WGRIDCOLOR LCD_RGB(80,80,80)
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

void Sleep(uint32_t nms);

typedef enum
{
    BS400, BS800, BS1600, BS2M, BS4M, BS8M, BS16M, BS20M, BS40M
} BANDSPAN;

typedef enum
{
    GRAPH_VSWR, GRAPH_RX, GRAPH_SMITH
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
};

static const uint32_t hamBandsNum = sizeof(hamBands) / sizeof(*hamBands);
static const uint32_t cx0 = 240; //Smith chart center
static const uint32_t cy0 = 120; //Smith chart center
static const char *modstr = "EU1KY AA v." AAVERSION;

static uint32_t modstrw = 0;

static const char* BSSTR[] = {"400 kHz", "800 kHz", "1.6 MHz", "2 MHz", "4 MHz", "8 MHz", "16 MHz", "20 MHz", "40 MHz"};
static const char* BSSTR_HALF[] = {"200 kHz", "400 kHz", "800 kHz", "1 MHz", "2 MHz", "4 MHz", "8 MHz", "10 MHz", "20 MHz"};

static const uint32_t BSVALUES[] = {400, 800, 1600, 2000, 4000, 8000, 16000, 20000, 40000};
static uint32_t f1 = 14000;
static BANDSPAN span = BS800;
static char buf[64];
static LCDPoint pt;
static float complex values[WWIDTH];
static int isMeasured = 0;
static uint32_t cursorPos = WWIDTH / 2;
static GRAPHTYPE grType = GRAPH_VSWR;
static uint32_t isSaved = 0;
static uint32_t cursorChangeCount = 0;

static void DrawRX();
static void DrawSmith();
static float complex SmoothRX(int idx, int useHighSmooth);

static int swroffset(float swr)
{
    int offs = (int)roundf(150. * log10f(swr));
    if (offs >= WHEIGHT)
        offs = WHEIGHT - 1;
    else if (offs < 0)
        offs = 0;
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
    LCDPoint p;
    if (!isMeasured)
        return;
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 2, 110, "<");
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 465, 110, ">");

    if (grType == GRAPH_SMITH)
    {
        float complex rx = values[cursorPos]; //SmoothRX(cursorPos, f1 > (BAND_FMAX / 1000) ? 1 : 0);
        float complex g = OSL_GFromZ(rx, (float)CFG_GetParam(CFG_PARAM_R0));
        uint32_t x = (uint32_t)roundf(cx0 + crealf(g) * 100.);
        uint32_t y = (uint32_t)roundf(cy0 - cimagf(g) * 100.);
        p = LCD_MakePoint(x, y);
        LCD_InvertPixel(p);
        p.x -=1;
        LCD_InvertPixel(p);
        p.x += 2;
        LCD_InvertPixel(p);
        p.x -= 1;
        p.y -=1;
        LCD_InvertPixel(p);
        p.y += 2;
        LCD_InvertPixel(p);
    }
    else
    {
        //Draw cursor line as inverted image
        p = LCD_MakePoint(X0 + cursorPos, Y0);
        while (p.y < Y0 + WHEIGHT)
        {
            LCD_InvertPixel(p);
            p.y++;
        }
    }
}

static void DrawCursorText()
{
    float complex rx = values[cursorPos]; //SmoothRX(cursorPos, f1 > (BAND_FMAX / 1000) ? 1 : 0);
    float ga = cabsf(OSL_GFromZ(rx, (float)CFG_GetParam(CFG_PARAM_R0))); //G magnitude

    uint32_t fstart;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - BSVALUES[span] / 2;

    float fcur = ((float)(fstart + cursorPos * BSVALUES[span] / WWIDTH))/1000.;
    if (fcur * 1000000.f > (float)(BAND_FMAX + 1))
        fcur = 0.f;
    FONT_Print(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 0, Y0 + WHEIGHT + 16, "F: %.3f   Z: %.1f%+.1fj   SWR: %.2f   MCL: %.2f dB          ",
        fcur,
        crealf(rx),
        cimagf(rx),
        DSP_CalcVSWR(rx),
        (ga > 0.01f) ? (-10. * log10f(ga)) : 99.f // Matched cable loss
    );
}

static void DrawSaveText(void)
{
    static const char* txt = "  Save snapshot  ";
    FONT_ClearLine(FONT_FRAN, LCD_BLACK, Y0 + WHEIGHT + 16 + 16);
    FONT_Write(FONT_FRAN, LCD_BLUE, LCD_YELLOW, 480 / 2 - FONT_GetStrPixelWidth(FONT_FRAN, txt) / 2,
               Y0 + WHEIGHT + 16 + 16, txt);
}

static void DrawSavingText(void)
{
    static const char* txt = "  Saving snapshot...  ";
    FONT_ClearLine(FONT_FRAN, LCD_BLACK, Y0 + WHEIGHT + 16 + 16);
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_BLUE, 480 / 2 - FONT_GetStrPixelWidth(FONT_FRAN, txt) / 2,
               Y0 + WHEIGHT + 16 + 16, txt);
    Sleep(20);
}

static void DrawSavedText(void)
{
    static const char* txt = "  Snapshot saved  ";
    FONT_ClearLine(FONT_FRAN, LCD_BLACK, Y0 + WHEIGHT + 16 + 16);
    FONT_Write(FONT_FRAN, LCD_WHITE, LCD_RGB(0, 60, 0), 480 / 2 - FONT_GetStrPixelWidth(FONT_FRAN, txt) / 2,
               Y0 + WHEIGHT + 16 + 16, txt);
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
    DrawCursorText();
    //SCB_CleanDCache();
    if (cursorChangeCount++ < 10)
        Sleep(100); //Slow down at first steps
}

static void AdvCursor()
{
    if (!isMeasured)
        return;
    if (cursorPos == WWIDTH-1)
        return;
    DrawCursor();
    cursorPos++;
    DrawCursor();
    DrawCursorText();
    //SCB_CleanDCache();
    if (cursorChangeCount++ < 10)
        Sleep(100); //Slow down at first steps
}

static void DrawGrid(int drawSwr)
{
    int i;
    LCD_FillAll(LCD_BLACK);

    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 1, 0, modstr);

    uint32_t fstart;
    if (0 == CFG_GetParam(CFG_PARAM_PAN_CENTER_F))
    {
        fstart = f1;
        if (drawSwr)
            sprintf(buf, "VSWR graph: %d kHz + %s   (Z0 = %d)", (int)fstart, BSSTR[span], CFG_GetParam(CFG_PARAM_R0));
        else
            sprintf(buf, "R/X graph: %d kHz + %s", (int)fstart, BSSTR[span]);
    }
    else
    {
        fstart = f1 - BSVALUES[span] / 2;
        if (drawSwr)
            sprintf(buf, "VSWR graph: %d kHz +/- %s   (Z0 = %d)", (int)f1, BSSTR_HALF[span], CFG_GetParam(CFG_PARAM_R0));
        else
            sprintf(buf, "R/X graph: %d kHz +/- %s", (int)f1, BSSTR_HALF[span]);
    }

    FONT_Write(FONT_FRAN, LCD_BLUE, LCD_BLACK, modstrw + 10, 0, buf);

    //Mark ham bands with colored background
    for (i = 0; i < WWIDTH; i++)
    {
        uint32_t f = fstart + (i * BSVALUES[span]) / WWIDTH;
        if (IsFinHamBands(f))
        {
            LCD_Line(LCD_MakePoint(X0 + i, Y0), LCD_MakePoint(X0 + i, Y0 + WHEIGHT), LCD_RGB(0, 0, 64));
        }
    }

    //Draw F grid and labels
    int lmod = (BS20M == span) || (BS40M == span) || (BS16M == span) || (BS1600 == span) ? 4 : 5;
    int linediv = 10; //Draw vertical line every linediv pixels
    for (i = 0; i <= WWIDTH/linediv; i++)
    {
        int x = X0 + i * linediv;
        if ((i % lmod) == 0 || i == WWIDTH/linediv)
        {
            char f[10];
            float flabel = ((float)(fstart + i * BSVALUES[span] / (WWIDTH/linediv)))/1000.f;
            if (flabel * 1000000.f > (float)(BAND_FMAX+1))
                continue;
            sprintf(f, "%.2f", ((float)(fstart + i * BSVALUES[span] / (WWIDTH/linediv)))/1000.f);
            int w = FONT_GetStrPixelWidth(FONT_SDIGITS, f);
            FONT_Write(FONT_SDIGITS, LCD_WHITE, LCD_BLACK, x - w / 2, Y0 + WHEIGHT + 5, f);
            LCD_Line(LCD_MakePoint(x, Y0), LCD_MakePoint(x, Y0 + WHEIGHT), WGRIDCOLORBR);
        }
        else
        {
            LCD_Line(LCD_MakePoint(x, Y0), LCD_MakePoint(x, Y0 + WHEIGHT), WGRIDCOLOR);
        }
    }

    if (drawSwr)
    {
        //Draw SWR grid and labels
        static const float swrs[]  = { 1., 1.1, 1.2, 1.3, 1.4, 1.5, 2., 2.5, 3., 4., 5., 6., 7., 8., 9., 10., 13., 16., 20.};
        static const char labels[] = { 1,  0,   0,   0,    0,   1,  1,   1,  1,  1,  1,  0,  1,  0,  0,   1,   1,   1,   1 };
        static const int nswrs = sizeof(swrs) / sizeof(float);
        for (i = 0; i < nswrs; i++)
        {
            int yofs = swroffset(swrs[i]);
            if (labels[i])
            {
                char s[10];
                sprintf(s, "%.1f", swrs[i]);
                FONT_Write(FONT_SDIGITS, LCD_WHITE, LCD_BLACK, X0 - 15, WY(yofs) - 2, s);
            }
            LCD_Line(LCD_MakePoint(X0, WY(yofs)), LCD_MakePoint(X0 + WWIDTH, WY(yofs)), WGRIDCOLOR);
        }
    }
}

static inline const char* bsstr(BANDSPAN bs)
{
    return BSSTR[bs];
}

static void print_span(BANDSPAN sp)
{
    sprintf(buf, "<<  <  Span: %s  >  >>", bsstr(sp));
    FONT_ClearLine(FONT_CONSBIG, LCD_BLACK, 50);
    FONT_Write(FONT_CONSBIG, LCD_BLUE, LCD_BLACK, 0, 50, buf);
}

static void print_f1(uint32_t f)
{
    sprintf(buf, "<<  <  %s: %d kHz  >  >>", CFG_GetParam(CFG_PARAM_PAN_CENTER_F) ? "Fc" : "F0", (int)f);
    FONT_ClearLine(FONT_CONSBIG, LCD_BLACK, 100);
    FONT_Write(FONT_CONSBIG, LCD_BLUE, LCD_BLACK, 0, 100, buf);
}

static void nextspan(BANDSPAN *sp)
{
    if (*sp == BS40M)
    {
        *sp = BS400;
    }
    else
    {
        *sp = (BANDSPAN)((int)*sp + 1);
    }
}

static void prevspan(BANDSPAN *sp)
{
    if (*sp == BS400)
    {
        *sp = BS40M;
    }
    else
    {
        *sp = (BANDSPAN)((int)*sp - 1);
    }
}

static void SELFREQ_Proc(void)
{
    static BANDSPAN spantmp;
    static uint32_t f1tmp;
    static uint32_t speedcnt;

    spantmp = span;
    f1tmp = f1;
    speedcnt = 0;

    LCD_FillAll(LCD_BLACK);
    while(TOUCH_IsPressed());
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 30, 5, "Select Freq and Bandspan");

    print_span(span);
    print_f1(f1);

    FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_BLACK, 40, 200, "OK");
    FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 220, 200, "Cancel");

    for(;;)
    {
        if (TOUCH_Poll(&pt))
        {
            if (pt.y < 90)
            { //Span
                speedcnt = 0;
                if (pt.x < 140)
                { //minus
                    prevspan(&spantmp);
                    print_span(spantmp);
                }
                else if (pt.x > 180)
                {//plus
                    nextspan(&spantmp);
                    print_span(spantmp);
                }
                Sleep(100); //slow down span cycling
            }
            else if (pt.y < 140)
            { //f1
                ++speedcnt;
                if (pt.x < 140)
                {
                    if (f1tmp > BAND_FMIN/1000)
                    {
                        f1tmp -= 100;
                        if (f1tmp < BAND_FMIN/1000)
                            f1tmp = BAND_FMIN;
                        else if (f1tmp > BAND_FMAX/1000)
                            f1tmp = BAND_FMAX/1000;
                        print_f1(f1tmp);
                    }
                }
                else if (pt.x > 180)
                {
                    f1tmp += 100;
                    if (f1tmp > BAND_FMAX/1000)
                        f1tmp = BAND_FMAX/1000;
                    print_f1(f1tmp);
                }
            }
            else if (pt.y > 200)
            {
                speedcnt = 0;
                if (pt.x < 140)
                {//OK
                    f1 = f1tmp;
                    span = spantmp;
                    CFG_SetParam(CFG_PARAM_PAN_F1, f1);
                    CFG_SetParam(CFG_PARAM_PAN_SPAN, span);
                    CFG_Flush();
                    while(TOUCH_IsPressed());
                    Sleep(100);
                    isMeasured = 0;
                    return;
                }
                else if (pt.x > 180)
                {//Cancel
                    while(TOUCH_IsPressed());
                    Sleep(100);
                    return;
                }
            }
        }
        else
        {
            speedcnt = 0;
        }
        if (speedcnt < 10)
            Sleep(150);
        else if (speedcnt < 20)
            Sleep(30);
    }
}


static void ScanRX()
{
    int i;
    uint32_t fstart;
    if (CFG_GetParam(CFG_PARAM_PAN_CENTER_F) == 0)
        fstart = f1;
    else
        fstart = f1 - BSVALUES[span] / 2;

    DSP_Measure(BAND_FMIN, 1, 1, 1); //Fake initial run to let the circuit stabilize

    for(i = 0; i < WWIDTH; i++)
    {
        uint32_t freq;
        freq = (fstart + (i * BSVALUES[span]) / WWIDTH) * 1000;
        if (freq == 0) //To overcome special case in DSP_Measure, where 0 is valid value
            freq = 1;
        DSP_Measure(freq, 1, 1, CFG_GetParam(CFG_PARAM_PAN_NSCANS));
        float complex rx = DSP_MeasuredZ();
        if (isnan(crealf(rx)) || isinf(crealf(rx)))
            rx = 0.0f + cimagf(rx) * I;
        if (isnan(cimagf(rx)) || isinf(cimagf(rx)))
            rx = crealf(rx) + 0.0fi;
        /*
        if (crealf(rx) > 1999.)
            rx = 1999.f + cimagf(rx) * I;
        if (cimagf(rx) > 1999.)
            rx = crealf(rx) + 1999.fi;
        else if (cimagf(rx) < -1999.)
            rx = crealf(rx) - 1999.fi;
        */
        values[i] = rx;
        LCD_SetPixel(LCD_MakePoint(X0 + i, 135), LCD_BLUE);
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

static void DrawVSWR()
{
    if (!isMeasured)
        return;
    int lastoffset = 0;
    int lastoffset_sm = 0;
    int i;
    for(i = 0; i < WWIDTH; i++)
    {
        int offset = swroffset(DSP_CalcVSWR(values[i]));
        int offset_sm = swroffset(DSP_CalcVSWR(SmoothRX(i,  f1 > (BAND_FMAX / 1000) ? 1 : 0)));
        int x = X0 + i;
        if(i == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, WY(offset)), LCD_RGB(0, SM_INTENSITY, 0));
            LCD_SetPixel(LCD_MakePoint(x, WY(offset_sm)), LCD_GREEN);
        }
        else
        {
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)), LCD_MakePoint(x, WY(offset)), LCD_RGB(0, SM_INTENSITY, 0));
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)), LCD_MakePoint(x, WY(offset_sm)), LCD_GREEN);
        }
        lastoffset = offset;
        lastoffset_sm = offset_sm;
    }
}

static void LoadBkups()
{
    //Load saved frequency and span values from config file
    uint32_t fbkup = CFG_GetParam(CFG_PARAM_PAN_F1);
    if (fbkup != 0 && fbkup >= BAND_FMIN/1000 && fbkup <= BAND_FMAX/1000 && (fbkup % 100) == 0)
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
    if (spbkup <= BS40M)
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

static void DrawHelp()
{
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 160, 15, "(Tap here to set F and Span)");
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 10, 190, "(Tap to exit to main window)");
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 180, 110, "(Tap to change graph)");
    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 360, 190, "(Tap to run scan)");
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

static void DrawRX()
{
    int i;
    if (!isMeasured)
        return;
    //Find min and max values among scanned R and X to set up scale
    float minRX = 1000000.f;
    float maxRX = -1000000.f;
    for (i = 0; i < WWIDTH; i++)
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
    int ystep = roundf((WHEIGHT * d) / grange);
    float labelValue;
    for (labelValue = graphmin; labelValue < graphmax + (.5 * d); labelValue += d)
    {
        sprintf(buf, str, labelValue); //Get label string in buf
        FONT_Write(FONT_SDIGITS, LCD_WHITE, LCD_BLACK, X0 - 25, WY(yofs) - 2, buf);
        if (roundf(labelValue) == 0)
            LCD_Line(LCD_MakePoint(X0, WY(yofs)), LCD_MakePoint(X0 + WWIDTH, WY(yofs)), WGRIDCOLORBR);
        else
            LCD_Line(LCD_MakePoint(X0, WY(yofs)), LCD_MakePoint(X0 + WWIDTH, WY(yofs)), WGRIDCOLOR);
        yofs += ystep;
    }

    #define RXOFFS(rx) ((int)roundf(((rx - graphmin) * WHEIGHT) / grange) + 1)
    //Now draw R graph
    int lastoffset = 0;
    int lastoffset_sm = 0;
    for(i = 0; i < WWIDTH; i++)
    {
        float r = crealf(values[i]);
        if (r < -1999.f)
            r = -1999.f;
        else if (r > 1999.f)
            r = 1999.f;
        yofs = RXOFFS(r);
        r = crealf(SmoothRX(i,  f1 > (BAND_FMAX / 1000) ? 1 : 0));
        if (r < -1999.f)
            r = -1999.f;
        else if (r > 1999.f)
            r = 1999.f;
        yofs_sm = RXOFFS(r);
        int x = X0 + i;
        if(i == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs)), LCD_RGB(0, SM_INTENSITY, 0));
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs_sm)), LCD_GREEN);
        }
        else
        {
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)), LCD_MakePoint(x, WY(yofs)), LCD_RGB(0, SM_INTENSITY, 0));
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)), LCD_MakePoint(x, WY(yofs_sm)), LCD_GREEN);
        }
        lastoffset = yofs;
        lastoffset_sm = yofs_sm;
    }

    //Now draw X graph
    lastoffset = 0;
    lastoffset_sm = 0;
    for(i = 0; i < WWIDTH; i++)
    {
        float ix = cimagf(values[i]);
        if (ix < -1999.f)
            ix = -1999.f;
        else if (ix > 1999.f)
            ix = 1999.f;
        yofs = RXOFFS(ix);
        ix = cimagf(SmoothRX(i,  f1 > (BAND_FMAX / 1000) ? 1 : 0));
        if (ix < -1999.f)
            ix = -1999.f;
        else if (ix > 1999.f)
            ix = 1999.f;
        yofs_sm = RXOFFS(ix);
        int x = X0 + i;
        if(i == 0)
        {
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs)), LCD_RGB(SM_INTENSITY, 0, 0));
            LCD_SetPixel(LCD_MakePoint(x, WY(yofs_sm)), LCD_RED);
        }
        else
        {
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset)), LCD_MakePoint(x, WY(yofs)), LCD_RGB(SM_INTENSITY, 0, 0));
            LCD_Line(LCD_MakePoint(x - 1, WY(lastoffset_sm)), LCD_MakePoint(x, WY(yofs_sm)), LCD_RED);
        }
        lastoffset = yofs;
        lastoffset_sm = yofs_sm;
    }
}

static void DrawSmith()
{
    int i;

    LCD_FillAll(LCD_BLACK);
    if (0 == CFG_GetParam(CFG_PARAM_PAN_CENTER_F))
        sprintf(buf, "Smith chart: %d kHz + %s, red pt. is end. Z0 = %d.", (int)f1, BSSTR[span], CFG_GetParam(CFG_PARAM_R0));
    else
        sprintf(buf, "Smith chart: %d kHz +/- %s, red pt. is end. Z0 = %d.", (int)f1, BSSTR_HALF[span], CFG_GetParam(CFG_PARAM_R0));
    FONT_Write(FONT_FRAN, LCD_BLUE, LCD_BLACK, 0, 0, buf);
    LCD_FillCircle(LCD_MakePoint(cx0, cy0), 100, SMITH_CIRCLE_BG); //Chart circle
    LCD_Circle(LCD_MakePoint(cx0, cy0), 33, WGRIDCOLOR); //VSWR 2.0 circle
    LCD_Circle(LCD_MakePoint(cx0, cy0), 20, WGRIDCOLOR); //VSWR 1.5 circle
    LCD_Circle(LCD_MakePoint(cx0 - 50, cy0), 50, LCD_RGB(12,12,12)); //Constant Y circle
    LCD_Circle(LCD_MakePoint(cx0 + 17, cy0), 83, WGRIDCOLOR); //Constant R 10 circle
    LCD_Circle(LCD_MakePoint(cx0 + 33, cy0), 66, WGRIDCOLOR); //Constant R 25 circle
    LCD_Circle(LCD_MakePoint(cx0 + 50, cy0), 50, WGRIDCOLOR); //Constant R 50 circle
    LCD_Circle(LCD_MakePoint(cx0 + 66, cy0), 33, WGRIDCOLOR); //Constant R 100 circle
    LCD_Circle(LCD_MakePoint(cx0 + 80, cy0), 20, WGRIDCOLOR); //Constant R 200 circle
    LCD_Line(LCD_MakePoint(cx0 - 100, cy0),LCD_MakePoint(cx0 + 100, cy0), WGRIDCOLOR); //Middle line

    //Draw X arcs
    {
        static const float xx[] = {10., 25., 50., 100., 200.};
        for (i = 0; i < 5; i++)
        {
            float j;
            for (j = 1.; j < 1000.; j *= 1.3)
            {
                float complex g = OSL_GFromZ(j + xx[i] * I, 50.f); //Intentoionally using 50 Ohms to calc arcs from the xx[] values
                uint32_t x = (uint32_t)roundf(cx0 + crealf(g) * 100.);
                uint32_t y = (uint32_t)roundf(cy0 - cimagf(g) * 100.);
                LCD_SetPixel(LCD_MakePoint(x, y), WGRIDCOLOR);
                if (j == 1.)
                {
                    switch (i)
                    {
                    case 0:
                        FONT_Write(FONT_SDIGITS, WGRIDCOLORBR, LCD_BLACK, x - 20, y, "0.2");
                        break;
                    case 1:
                        FONT_Write(FONT_SDIGITS, WGRIDCOLORBR, LCD_BLACK, x - 15, y - 5, "0.5");
                        break;
                    case 3:
                        FONT_Write(FONT_SDIGITS, WGRIDCOLORBR, LCD_BLACK, x + 3, y - 5, "2");
                        break;
                    case 4:
                        FONT_Write(FONT_SDIGITS, WGRIDCOLORBR, LCD_BLACK, x + 5, y, "4");
                        break;
                    default:
                        break;
                    }
                }

                y = (uint32_t)roundf(cy0 + cimagf(g) * 100.);
                LCD_SetPixel(LCD_MakePoint(x, y), WGRIDCOLOR);
                if (j == 1.)
                {
                    switch (i)
                    {
                    case 0:
                        FONT_Write(FONT_SDIGITS, WGRIDCOLORBR, LCD_BLACK, x - 20, y, "-0.2");
                        break;
                    case 1:
                        FONT_Write(FONT_SDIGITS, WGRIDCOLORBR, LCD_BLACK, x - 15, y + 5, "-0.5");
                        break;
                    case 2:
                        FONT_Write(FONT_SDIGITS, WGRIDCOLORBR, LCD_BLACK, x - 7, y + 7, "-1");
                        break;
                    case 3:
                        FONT_Write(FONT_SDIGITS, WGRIDCOLORBR, LCD_BLACK, x + 3, y + 5, "-2");
                        break;
                    case 4:
                        FONT_Write(FONT_SDIGITS, WGRIDCOLORBR, LCD_BLACK, x + 5, y, "-4");
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }

    //Draw R cirle labels
    FONT_Write(FONT_SDIGITS, WGRIDCOLOR, SMITH_CIRCLE_BG, cx0 - 75, cy0 + 2, "0.2");
    FONT_Write(FONT_SDIGITS, WGRIDCOLOR, SMITH_CIRCLE_BG, cx0 - 42, cy0 + 2, "0.5");
    FONT_Write(FONT_SDIGITS, WGRIDCOLOR, SMITH_CIRCLE_BG, cx0 + 2, cy0 + 2, "1");
    FONT_Write(FONT_SDIGITS, WGRIDCOLOR, SMITH_CIRCLE_BG, cx0 + 34, cy0 + 2, "2");
    FONT_Write(FONT_SDIGITS, WGRIDCOLOR, SMITH_CIRCLE_BG, cx0 + 62, cy0 + 2, "4");

    LCD_Circle(LCD_MakePoint(cx0, cy0), 100, WGRIDCOLORBR); //Outer circle

    if (isMeasured)
    {
        uint32_t lastx = 0;
        uint32_t lasty = 0;
        for(i = 0; i < WWIDTH; i++)
        {
            float complex g = OSL_GFromZ(values[i], (float)CFG_GetParam(CFG_PARAM_R0));
            uint32_t x = (uint32_t)roundf(cx0 + crealf(g) * 100.);
            uint32_t y = (uint32_t)roundf(cy0 - cimagf(g) * 100.);
            if (i != 0)
            {
                LCD_Line(LCD_MakePoint(lastx, lasty), LCD_MakePoint(x, y), LCD_RGB(0, SM_INTENSITY, 0));
            }
            lastx = x;
            lasty = y;
        }
        //Draw smoothed
        lastx = lasty = 0;
        for(i = 0; i < WWIDTH; i++)
        {
            float complex g = OSL_GFromZ(SmoothRX(i,  f1 > (BAND_FMAX / 1000) ? 1 : 0), (float)CFG_GetParam(CFG_PARAM_R0));
            uint32_t x = (uint32_t)roundf(cx0 + crealf(g) * 100.);
            uint32_t y = (uint32_t)roundf(cy0 - cimagf(g) * 100.);
            if (i != 0)
            {
                LCD_Line(LCD_MakePoint(lastx, lasty), LCD_MakePoint(x, y), SMITH_LINE_FG);
            }
            lastx = x;
            lasty = y;
        }

        //Mark the end of sweep range with red cross
        LCD_SetPixel(LCD_MakePoint(lastx, lasty), LCD_RED);
        LCD_SetPixel(LCD_MakePoint(lastx-1, lasty), LCD_RED);
        LCD_SetPixel(LCD_MakePoint(lastx+1, lasty), LCD_RED);
        LCD_SetPixel(LCD_MakePoint(lastx, lasty-1), LCD_RED);
        LCD_SetPixel(LCD_MakePoint(lastx, lasty+1), LCD_RED);
    }
}

static void RedrawWindow()
{
    isSaved = 0;
    if (grType == GRAPH_VSWR)
    {
        DrawGrid(1);
        DrawVSWR();
    }
    else if (grType == GRAPH_RX)
    {
        DrawGrid(0);
        DrawRX();
    }
    else
        DrawSmith();
    DrawCursor();
    if (isMeasured)
    {
        DrawCursorText();
        DrawSaveText();
    }
    //SCB_CleanDCache(); //Flush D-Cache contents to the RAM to avoid cache coherency
}

static const uint8_t bmp_hdr[] =
{
    0x42, 0x4D,             //"BM"
    0x36, 0xFA, 0x05, 0x00, //size in bytes
    0x00, 0x00, 0x00, 0x00, //reserved
    0x36, 0x00, 0x00, 0x00, //offset to image in bytes
    0x28, 0x00, 0x00, 0x00, //info size in bytes
    0xE0, 0x01, 0x00, 0x00, //width
    0x10, 0x01, 0x00, 0x00, //height
    0x01, 0x00,             //planes
    0x18, 0x00,             //bits per pixel
    0x00, 0x00, 0x00, 0x00, //compression
    0x00, 0xfa, 0x05, 0x00, //image size
    0x00, 0x00, 0x00, 0x00, //x resolution
    0x00, 0x00, 0x00, 0x00, //y resolution
    0x00, 0x00, 0x00, 0x00, // colours
    0x00, 0x00, 0x00, 0x00  //important colours
};


static void save_snapshot(void)
{
    static const TCHAR *sndir = "/aa/snapshot";
    char path[64];
    char wbuf[256];

    if (!isMeasured || isSaved)
        return;

    DrawSavingText();

    SCB_CleanDCache_by_Addr((uint32_t*)LCD_FB_START_ADDRESS, BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4); //Flush and invalidate D-Cache contents to the RAM to avoid cache coherency
    Sleep(10);
    f_mkdir(sndir);

    //Scan dir for snapshot files
    uint32_t fmax = 0;
    uint32_t fmin = 0xFFFFFFFFul;
    DIR dir = { 0 };
    FILINFO fno = { 0 };
    FRESULT fr = f_opendir(&dir, sndir);
    uint32_t numfiles = 0;
    int i;
    if (fr == FR_OK)
    {
        for (;;)
        {
            fr = f_readdir(&dir, &fno); //Iterate through the directory
            if (fr != FR_OK || !fno.fname[0])
                break; //Nothing to do
            if (_FS_RPATH && fno.fname[0] == '.')
                continue; //bypass hidden files
            if (fno.fattrib & AM_DIR)
                continue; //bypass subdirs
            int len = strlen(fno.fname);
            if (len != 12) //Bypass filenames with unexpected name length
                continue;
            if (0 != strcasecmp(&fno.fname[8], ".s1p"))
                continue; //Bypass files that are not s1p
            for (i = 0; i < 8; i++)
                if (!isdigit(fno.fname[i]))
                    break;
            if (i != 8)
                continue; //Bypass file names that are not 8-digit hex numbers
            numfiles++;
            //Now convert file name to number
            uint32_t hexn = 0;
            char* endptr;
            hexn = strtoul(fno.fname, &endptr, 10);
            if (hexn < fmin)
                fmin = hexn;
            if (hexn > fmax)
                fmax = hexn;
        }
        f_closedir(&dir);
    }
    else
    {
        CRASHF("Failed to open directory %s", sndir);
    }
    //Erase one oldest file if needed
    if (numfiles >= 100)
    {
        sprintf(path, "%s/%08d.s1p", sndir, fmin);
        f_unlink(path);
        sprintf(path, "%s/%08d.bmp", sndir, fmin);
        f_unlink(path);
    }

    //Now write measured data to file fmax+1 in s1p format
    sprintf(path, "%s/%08d.s1p", sndir, fmax+1);
    FIL fo = { 0 };
    UINT bw;
    fr = f_open(&fo, path, FA_CREATE_ALWAYS |FA_WRITE);
    if (FR_OK != fr)
        CRASHF("Failed to open file %s", path);
    sprintf(wbuf, "! Touchstone file by EU1KY antenna analyzer\r\n"
                  "# MHz S RI R 50\r\n"
                  "! Format: Frequency S-real S-imaginary (normalized to 50 Ohm)\r\n");
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
        float fmhz = (float)(fstart + i * BSVALUES[span] / WWIDTH) / 1000.0f;
        sprintf(wbuf, "%.3f %.6f %.6f\r\n", fmhz, crealf(g), cimagf(g));
        fr = f_write(&fo, wbuf, strlen(wbuf), &bw);
        if (FR_OK != fr) goto CRASH_WR;
    }
    f_close(&fo);

    //Now write screenshot as bitmap
    sprintf(path, "%s/%08d.bmp", sndir, fmax+1);
    fr = f_open(&fo, path, FA_CREATE_ALWAYS |FA_WRITE);
    if (FR_OK != fr)
        CRASHF("Failed to open file %s", path);
    fr = f_write(&fo, bmp_hdr, sizeof(bmp_hdr), &bw);
    if (FR_OK != fr) goto CRASH_WR;
    int x = 0;
    int y = 0;
    for (y = 271; y >= 0; y--)
    {
        uint32_t line[480];
        BSP_LCD_ReadLine(y, line);
        for (x = 0; x < 480; x++)
        {
            fr = f_write(&fo, &line[x], 3, &bw);
            if (FR_OK != fr) goto CRASH_WR;
        }
    }
    f_close(&fo);

    isSaved = 1;
    DrawSavedText();
    return;
CRASH_WR:
    CRASHF("Failed to write to file %s", path);
}

void PANVSWR2_Proc(void)
{
    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 120, 100, "Panoramic scan mode");
    Sleep(500);
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
        //SCB_CleanDCache(); //Flush D-Cache contents to the RAM to avoid cache coherency
    }
    else
        RedrawWindow();

    for(;;)
    {
        Sleep(50);
        if (TOUCH_Poll(&pt))
        {
            if (pt.y < 80)
            {// Top
                SELFREQ_Proc();
                RedrawWindow();
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
                    else if (grType == GRAPH_RX)
                        grType = GRAPH_SMITH;
                    else
                        grType = GRAPH_VSWR;
                    RedrawWindow();
                }
                else if (pt.x > 430)
                {
                    AdvCursor();
                    continue;
                }
            }
            else if (pt.y > 180 && pt.y < 255)
            {
                if (pt.x < 140)
                {// Lower left corner
                    while(TOUCH_IsPressed());
                    Sleep(100);
                    return;
                }
                else if (pt.x > 340)
                {//Lower right corner: perform scan
                    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 180, 100, "  Scanning...  ");
                    ScanRX();
                    RedrawWindow();
                }
            }
            else if (pt.y > 260)
            {
                if (pt.x > 160 && pt.x < 320 && isMeasured && !isSaved)
                {
                    save_snapshot();
                }
            }
            while(TOUCH_IsPressed())
            {
                Sleep(50);
            }
        }
        else
        {
            cursorChangeCount = 0;
        }
    }
}
