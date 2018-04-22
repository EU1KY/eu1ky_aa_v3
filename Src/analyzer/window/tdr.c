/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <math.h>
#include <complex.h>
#include "arm_math.h"

#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "config.h"
#include "crash.h"
#include "dsp.h"
#include "gen.h"
#include "oslfile.h"
#include "stm32746g_discovery_lcd.h"
#include "screenshot.h"
#include "tdr.h"
#include "hit.h"

extern void Sleep(uint32_t);

#define NUMTDRSAMPLES 256
#define X0 40
#define Y0 135
#define WWIDTH 400

static float time_domain[NUMTDRSAMPLES*2];
static float complex freq_domain[NUMTDRSAMPLES];
static uint32_t rqExit = 0;
static float normFactor = 1.f; //Factor for calculating graph magnitude scale. Calculated from maximum value in time domain.

static uint32_t TDR_isScanned = 0;
static uint32_t TDR_cursorPos = WWIDTH / 2;
static uint32_t TDR_cursorChangeCount = 0;
static uint16_t TDR_Length = 155;// length of measure WK
static uint8_t colorScheme=0;
static uint32_t BGColor=LCD_BLACK;
static uint32_t GraphColor=LCD_BLUE;

//Half of Kaiser Bessel Derived (beta=3.5) window
static const float halfKBDwnd[] =
{
    0.99999436304 , 0.999949268201 , 0.999859083001 , 0.999723816398 , 0.999543481825 , 0.999318097191 , 0.999047684879 , 0.998732271741 ,
    0.998371889095 , 0.997966572724 , 0.997516362868 , 0.997021304218 , 0.996481445916 , 0.995896841543 , 0.995267549113 , 0.994593631069 ,
    0.993875154272 , 0.993112189992 , 0.992304813902 , 0.991453106066 , 0.990557150929 , 0.989617037306 , 0.988632858372 , 0.987604711648 ,
    0.986532698992 , 0.985416926582 , 0.984257504904 , 0.98305454874 , 0.981808177151 , 0.980518513461 , 0.979185685243 , 0.977809824304 ,
    0.976391066665 , 0.974929552546 , 0.973425426348 , 0.971878836631 , 0.970289936102 , 0.96865888159 , 0.966985834026 , 0.965270958426 ,
    0.963514423869 , 0.961716403473 , 0.959877074374 , 0.957996617707 , 0.956075218577 , 0.954113066041 , 0.952110353079 , 0.950067276575 ,
    0.947984037286 , 0.945860839822 , 0.943697892616 , 0.9414954079 , 0.939253601676 , 0.936972693691 , 0.934652907407 , 0.932294469974 ,
    0.9298976122 , 0.927462568525 , 0.924989576986 , 0.922478879191 , 0.919930720286 , 0.917345348927 , 0.914723017246 , 0.912063980819 ,
    0.909368498635 , 0.906636833063 , 0.903869249818 , 0.901066017929 , 0.898227409703 , 0.895353700693 , 0.892445169662 , 0.889502098546 ,
    0.886524772423 , 0.883513479473 , 0.880468510944 , 0.877390161113 , 0.874278727253 , 0.871134509591 , 0.867957811274 , 0.864748938329 ,
    0.861508199624 , 0.858235906832 , 0.854932374389 , 0.851597919456 , 0.848232861879 , 0.844837524149 , 0.841412231361 , 0.837957311176 ,
    0.834473093776 , 0.830959911826 , 0.827418100429 , 0.82384799709 , 0.820249941666 , 0.816624276333 , 0.812971345533 , 0.809291495941 ,
    0.805585076414 , 0.801852437954 , 0.798093933658 , 0.794309918679 , 0.790500750182 , 0.786666787295 , 0.78280839107 , 0.778925924436 ,
    0.775019752152 , 0.771090240765 , 0.767137758564 , 0.763162675534 , 0.759165363309 , 0.75514619513 , 0.751105545796 , 0.747043791617 ,
    0.74296131037 , 0.738858481252 , 0.734735684835 , 0.730593303013 , 0.726431718965 , 0.722251317099 , 0.718052483011 , 0.713835603434 ,
    0.709601066192 , 0.705349260155 , 0.701080575189 , 0.696795402108 , 0.692494132627 , 0.688177159317 , 0.683844875555 , 0.679497675473 ,
    0.675135953919 , 0.670760106399 , 0.666370529037 , 0.661967618523 , 0.657551772066 , 0.653123387346 , 0.648682862468 , 0.644230595912 ,
    0.639766986483 , 0.63529243327 , 0.630807335591 , 0.626312092948 , 0.62180710498 , 0.617292771415 , 0.612769492021 , 0.60823766656 ,
    0.603697694737 , 0.599149976159 , 0.59459491028 , 0.59003289636 , 0.585464333414 , 0.580889620164 , 0.576309154997 , 0.571723335912 ,
    0.567132560478 , 0.562537225783 , 0.557937728393 , 0.553334464298 , 0.548727828872 , 0.544118216825 , 0.539506022157 , 0.534891638108 ,
    0.530275457121 , 0.525657870789 , 0.521039269811 , 0.51642004395 , 0.511800581985 , 0.507181271668 , 0.502562499676 , 0.497944651573 ,
    0.493328111759 , 0.488713263431 , 0.484100488535 , 0.479490167727 , 0.474882680326 , 0.470278404275 , 0.465677716093 , 0.461080990836 ,
    0.456488602056 , 0.451900921754 , 0.447318320343 , 0.442741166606 , 0.43816982765 , 0.433604668874 , 0.429046053918 , 0.424494344632 ,
    0.419949901029 , 0.415413081252 , 0.410884241527 , 0.40636373613 , 0.401851917346 , 0.39734913543 , 0.392855738572 , 0.388372072855 ,
    0.38389848222 , 0.379435308429 , 0.374982891028 , 0.370541567311 , 0.366111672282 , 0.361693538624 , 0.357287496659 , 0.352893874316 ,
    0.348512997094 , 0.344145188033 , 0.339790767672 , 0.335450054026 , 0.331123362543 , 0.32681100608 , 0.322513294865 , 0.318230536467 ,
    0.313963035767 , 0.309711094925 , 0.305475013349 , 0.301255087667 , 0.297051611696 , 0.292864876411 , 0.288695169924 , 0.284542777444 ,
    0.28040798126 , 0.276291060707 , 0.272192292142 , 0.268111948915 , 0.264050301347 , 0.260007616701 , 0.255984159157 , 0.25198018979 ,
    0.247995966544 , 0.244031744206 , 0.240087774389 , 0.236164305502 , 0.232261582733 , 0.228379848025 , 0.224519340055 , 0.220680294213 ,
    0.216862942583 , 0.21306751392 , 0.209294233634 , 0.20554332377 , 0.201815002987 , 0.198109486544 , 0.194426986283 , 0.190767710607 ,
    0.187131864468 , 0.18351964935 , 0.179931263253 , 0.176366900678 , 0.172826752616 , 0.169311006528 , 0.165819846336 , 0.16235345241 ,
    0.158912001554 , 0.155495666994 , 0.152104618371 , 0.148739021723 , 0.14539903948 , 0.142084830453 , 0.138796549824 , 0.135534349139
};

static const float KBD_td_factor = 1.43502708f; //Factor for the above window to normalize time-domain cumulative power to 1.0


//Scan 255 samples from 500 KHz up to 127.5 MHz step 500KHz
static void TDR_Scan(void)
{
    DSP_Measure(BAND_FMIN, 1, 1, 1); //Fake initial run to let the circuit stabilize

    freq_domain[0] = 0.f+0.fi; //bin 0 is zero

    uint32_t i;
    for (i = 1; i < NUMTDRSAMPLES; i++)
    {
        uint32_t freqHz =i * 500000;
        DSP_Measure(freqHz, 1, 1, CFG_GetParam(CFG_PARAM_PAN_NSCANS));
        float complex G = OSL_GFromZ(DSP_MeasuredZ(), (float)CFG_GetParam(CFG_PARAM_R0));
        freq_domain[i] = G * halfKBDwnd[i];
        LCD_SetPixel(LCD_MakePoint(X0 + i + 72, 145), LCD_BLUE);
        LCD_SetPixel(LCD_MakePoint(X0 + i + 72, 146), LCD_BLUE);
        LCD_SetPixel(LCD_MakePoint(X0 + i + 72, 147), LCD_BLUE);
        LCD_SetPixel(LCD_MakePoint(X0 + i + 72, 148), LCD_BLUE);
    }
    GEN_SetMeasurementFreq(0);

    // Do IRFFT
    arm_rfft_fast_instance_f32 S;
    arm_rfft_fast_init_f32(&S, NUMTDRSAMPLES*2);
    arm_rfft_fast_f32(&S, (float32_t*)freq_domain, time_domain, 1);

    //Time domain data is in rfft_input now
    //Normalize
    float max = -9999999.f;
    for (i = 0; i < NUMTDRSAMPLES*2; i++)
    {
        time_domain[i] * KBD_td_factor; //Normalization to compensate windowing
        float d = fabs(time_domain[i]);
        if (d > max)
        {
            max = d;
        }
    }
    normFactor = 1.f / fabs(max);
}

static void TDR_Exit(void)
{
    rqExit = 1;
}

static void TDR_Screenshot (void)
{
    char* fname = 0;
    fname = SCREENSHOT_SelectFileName();

     if(strlen(fname)==0) return;

    SCREENSHOT_DeleteOldest();
    if (CFG_GetParam(CFG_PARAM_SCREENSHOT_FORMAT))
        SCREENSHOT_SavePNG(fname);
    else
        SCREENSHOT_Save(fname);
}

static void TDR_DrawCursorText(void)
{
    FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 4, Y0 - 10, "<");
    FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 462, Y0 - 10, ">");
    float val = time_domain[(uint16_t)(TDR_cursorPos*(TDR_Length/155.f))];
    float ns = (float)TDR_cursorPos * (2000.f/512.f)*(TDR_Length/155.f); //2000 ns is the period of 500 kHz scn step, 512 is number of equivalent time domain samples
    float vf = (float)CFG_GetParam(CFG_PARAM_TDR_VF) / 100.f;
    if (vf < 0.01f || vf > 1.00f)
        vf = 0.66f;
    float lenm = vf * (0.299792458f * ns) * 0.5f;
    FONT_Print(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 0, 15, "T: %.1f ns, Mag: %.5f Dist: %.1f m (@Vf=%.2f)      ",
               ns, val, lenm, vf);
}


static void TDR_DrawCursor(void)
{
    LCDPoint p;
    if (!TDR_isScanned)
        return;

    //Draw cursor line as inverted image
    p = LCD_MakePoint(X0 + TDR_cursorPos, Y0 - 100);
    while (p.y < Y0 + 100)
    {
        LCD_InvertPixel(p);
        p.y++;
    }
}


static void TDR_DrawGrid(void)
{
    LCD_FillRect(LCD_MakePoint(0, Y0 - 100 - 2), LCD_MakePoint(LCD_GetWidth() - 1, Y0 + 100 + 2), BGColor);
    LCD_Rectangle(LCD_MakePoint(X0, Y0 - 100), LCD_MakePoint(X0 + WWIDTH - 1, Y0 + 100), LCD_RGB(80,80,80));
    LCD_Line(LCD_MakePoint(X0, Y0), LCD_MakePoint(X0 + WWIDTH - 1, Y0), LCD_RGB(80,80,80));
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLUE, 364, 4, "10m");// WK
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLUE, 404, 4, "50m");
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLUE, 444, 4, "155m");
    if(TDR_Length==10)
        FONT_Write(FONT_FRAN, LCD_BLACK, LCD_WHITE, 364, 4, "10m");
    else if(TDR_Length==50)
        FONT_Write(FONT_FRAN, LCD_BLACK, LCD_WHITE, 404, 4, "50m");
    else
        FONT_Write(FONT_FRAN, LCD_BLACK, LCD_WHITE, 444, 4, "155m");
}

static void TDR_DecrCursor(void)
{
    if (!TDR_isScanned)
        return;
    if (TDR_cursorPos == 0)
        return;
    TDR_DrawCursor();
    TDR_cursorPos--;
    TDR_DrawCursor();
    TDR_DrawCursorText();
    if (TDR_cursorChangeCount++ < 10)
        Sleep(100); //Slow down at first steps
}

static void TDR_AdvCursor(void)
{
    if (!TDR_isScanned)
        return;
    if (TDR_cursorPos >= (WWIDTH - 1))
        return;
    TDR_DrawCursor();
    TDR_cursorPos++;
    TDR_DrawCursor();
    TDR_DrawCursorText();
    if (TDR_cursorChangeCount++ < 10)
        Sleep(100); //Slow down at first steps
}

static void TDR_DrawGraph(void)
{
    uint32_t i;
    uint32_t lasty = Y0;
    uint32_t y;
    for (i = 0; i*155/TDR_Length <= WWIDTH+1; i++)
    {
        y = Y0 - (int32_t)(time_domain[i] * normFactor * 100.f);
        if (0 != i)
        {
            LCD_Line(LCD_MakePoint(X0 + (i-1)*155/TDR_Length, lasty), LCD_MakePoint(X0 + i*155/TDR_Length, y), GraphColor);
            LCD_Line(LCD_MakePoint(X0 + (i-1)*155/TDR_Length, lasty+1), LCD_MakePoint(X0 + i*155/TDR_Length, y+1), GraphColor);
            LCD_Line(LCD_MakePoint(X0 + (i-1)*155/TDR_Length-1, lasty), LCD_MakePoint(X0 + i*155/TDR_Length-1, y+1), GraphColor);
        }
        lasty = y;
    }
    FONT_Print(FONT_FRAN, LCD_WHITE, LCD_BLACK, 0, Y0 - 100-1, "%.3f", 1./normFactor);// WK
    FONT_Print(FONT_FRAN, LCD_WHITE, LCD_BLACK, 0, Y0 + 100 - 12, "%.3f", -1./normFactor);
   /* FONT_Print(FONT_SDIGITS, LCD_WHITE, LCD_BLACK, 10, Y0 - 100, "%.5f", 1./normFactor);
    FONT_Print(FONT_SDIGITS, LCD_WHITE, LCD_BLACK, 5, Y0 + 100 - 5, "%.5f", -1./normFactor);*/
}

static void TDR_DoScan (void)
{
    TDR_DrawGrid();
    FONT_Write(FONT_FRANBIG, LCD_RED, LCD_BLACK, 180, 100, "  Scanning...  ");
    TDR_Scan();
    TDR_isScanned = 1;
    TDR_DrawGrid();
    TDR_DrawGraph();
    TDR_DrawCursor();
    TDR_DrawCursorText();
}

static void TDR_10m(void){
    TDR_Length=10;
    TDR_DrawGrid();
}
static void TDR_50m(void){
    TDR_Length=50;
    TDR_DrawGrid();
}
static void TDR_155m(void){
    TDR_Length=155;
    TDR_DrawGrid();
}

static void TDR_Colors(void){
    if(colorScheme==0){
        BGColor=LCD_WHITE;
        GraphColor=LCD_BLACK;
        colorScheme=1;
    }
    else{
        colorScheme=0;
        BGColor=LCD_BLACK;
        GraphColor=LCD_RGB(255, 255, 128);//   LCD_LightYELLOW;
    }
    TDR_DrawGrid();
    TDR_DrawGraph();
    TDR_DrawCursor();
    TDR_DrawCursorText();
    Sleep(500);
}

static const struct HitRect hitArr[] =
{
    //        x0,  y0, width, height, callback
    HITRECT(   0, 241,  80,     31, TDR_Exit),
    HITRECT( 190, 241,  80,     31, TDR_Screenshot),
    HITRECT( 300, 241,  80,     31, TDR_DoScan),
    HITRECT( 400, 241,  60,     31, TDR_Colors),
    HITRECT( 360, Y0 - 135,   38,     30, TDR_10m),
    HITRECT( 400, Y0 - 135,   38,     30, TDR_50m),
    HITRECT( 440, Y0 - 135,   38,     30, TDR_155m),
    HITRECT( 430, Y0 - 20,   60,     72, TDR_AdvCursor),
    HITRECT(   0, Y0 - 20,   60,     72, TDR_DecrCursor),
    HITEND
};

void TDR_Proc(void)
{
    rqExit = 0;
    TDR_cursorChangeCount = 0;
    BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_BLACK);
    LCD_ShowActiveLayerOnly();

    while(TOUCH_IsPressed());

    BSP_LCD_SelectLayer(0);
    LCD_FillAll(LCD_BLACK);
    BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_BLACK);
    LCD_ShowActiveLayerOnly();

    FONT_Write(FONT_FRAN, LCD_PURPLE, LCD_BLACK, 1, 0, "EU1KY AA v." AAVERSION " Time Domain Reflectometer mode");
    FONT_Write(FONT_FRANBIG, LCD_GREEN, LCD_RGB(0, 0, 64), 0, 242, " Exit ");
    FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLUE, 300, 242, " Scan ");
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLUE, 404, 250, "ChgColors");
    FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_RGB(0, 64, 0), 192, 250, "Save snapshot");

    ShowHitRect(hitArr);
    TDR_DrawGrid();
    if (TDR_isScanned)
    {
        TDR_DrawGraph();
        TDR_DrawCursor();
        TDR_DrawCursorText();
    }

    for(;;)
    {
        LCDPoint pt;
        if (TOUCH_Poll(&pt))
        {
            HitTest(hitArr, pt.x, pt.y);
            if (rqExit)
            {
                GEN_SetMeasurementFreq(0);
                Sleep(20);
               // while(TOUCH_IsPressed());
                return;
            }
        }
        else
        {
            TDR_cursorChangeCount = 0;
        }
        Sleep(20);
    }

}
