/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#include "config.h"
#include "LCD.h"
#include "touch.h"
#include "ff.h"
#include "textbox.h"
#include "screenshot.h"
#include "crash.h"
#include "stm32746g_discovery_lcd.h"
#include "keyboard.h"
#include "lodepng.h"
#include "mainwnd.h"
#include "DS3231.h"

extern void Sleep(uint32_t);

static const TCHAR *SNDIR = "/aa/snapshot";
static uint32_t oldest = 0xFFFFFFFFul;
static uint32_t numfiles = 0;

uint8_t __attribute__((section (".user_sdram"))) __attribute__((used)) bmpFileBuffer[SCREENSHOT_FILE_SIZE];

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
static const char *g_logo_fpath1 = "/aa/logo.bmp";

static const char *g_logo_fpath2 = "/aa/logo.png";

DWORD get_fattime(void){
uint8_t second1;
short AMPM1;
uint32_t mon,yy,mm,dd,h,m,s;

    if(NoDate==1) return 0;
    if(RTCpresent==1){
        getTime(&time, &second1,&AMPM1,0);
        getDate(&date);

    }
    else{

        time=GetInternTime(&second1);
        date=CFG_GetParam(CFG_PARAM_Date);
    }
    mon=date%10000;
    yy=date/10000-1980;
    mm=mon/100;
    dd=mon%100;
    h=time/100;
    m=time%100;
    s=second1/2;
    return ((yy<<25)+(mm<<21)+(dd<<16)+ (h<<11)+(m<<5)+s);
}

int32_t ShowLogo(void){

uint32_t br = 0;
unsigned w,h, result1;
int i, type=0;
FRESULT res;
FIL fo = { 0 };
FILINFO finfo;
DIR dir = { 0 };
FILINFO fno = { 0 };
uint8_t* OutBuf=0;

    res = f_open(&fo, g_logo_fpath1, FA_READ);
    if (FR_OK == res) {
            type=1;//bmp
    }

    else
    {
        res = f_open(&fo, g_logo_fpath2, FA_READ);
        if(res==FR_OK){
            type=2;// png
        }
        else{

        Sleep(1000);
        }
    }
    if(type==0) {
        return -1;
    }
    if(type==1){
        res = f_read(&fo, bmpFileBuffer, SCREENSHOT_FILE_SIZE, &br);
        f_close(&fo);
        if (FR_OK != res)
            return -1;// no logo file found
        if (br != SCREENSHOT_FILE_SIZE || FR_OK != res)
            return -1;
        LCD_DrawBitmap(LCD_MakePoint(0,0), bmpFileBuffer, SCREENSHOT_FILE_SIZE);
        return 0;
    }
    else{
        res = f_read(&fo, bmpFileBuffer, fo.fsize, &br);
        if(res!=0){
            f_close(&fo);
            return -1;
        }
        else{
            result1=lodepng_decode24(&OutBuf, &w, &h, bmpFileBuffer, fo.fsize);
            f_close(&fo);
            if(result1==0){
               PixPict((480-w)/2,(272-h)/2,&OutBuf[0]);
               lodepng_free((void*) &OutBuf);
            }
            else {
                return -1;
            }
        }
    }
    return 0;
 }


extern volatile int  Page;
extern uint16_t Pointer;
extern TCHAR  fileNames[13][13];
static DWORD  FileLength[12];

void AnalyzeDate(WORD datex, char* str){
int yy, mm, dd;
    if(NoDate==1) return ;
    yy=datex>>9;
    dd=datex&0x1f;
    mm=(datex>>5)&0xf;
    sprintf(&str[0], "%04d %02d %02d", yy+1980, mm, dd);
}
void AnalyzeTime(WORD datex, char* str){
int hh, mm;
    if(NoDate==1) return ;
    hh=datex>>11;
    mm=(datex>>5)&0x3f;
    sprintf(&str[0], "%02d:%02d", hh, mm);
}

int16_t SCREENSHOT_SelectFileNames(int fileNoMin)
{
char string0[10];

    f_mkdir(SNDIR);
    //Scan dir for snapshot files
    DIR dir = { 0 };
    FILINFO fno = { 0 };
    FRESULT fr = f_opendir(&dir, SNDIR);
    int i,j, fileNoMax;
    fileNoMax=0;

    if (fr == FR_OK)
    {
        for (j=0;j<fileNoMin+12;)//*Page;)
        {
            fr = f_readdir(&dir, &fno); //Iterate through the directory
            if (fr != FR_OK || !fno.fname[0])
                break; //Nothing to do
            if (_FS_RPATH && fno.fname[0] == '.')
                continue; //bypass hidden files
            if (fno.fattrib & AM_DIR)
                continue; //bypass subdirs

            const char *pdot = strchr(fno.fname, (int)'.');
            if (0 == pdot)
                continue;
            if (0 != strcasecmp(pdot, ".bmp") && 0 != strcasecmp(pdot, ".png"))
                continue; //Bypass files that are not bmp or png
            if(j>=fileNoMin){// only show and store the last 12 files
                strcpy((char*) &fileNames[j%12][0], fno.fname);
                FileLength[j%12]=fno.fsize;
                FONT_Write(FONT_FRAN, TextColor, BackGrColor, 0, 32+16*(j%12), fno.fname);
                AnalyzeDate(fno.fdate, &string0[0]);
                FONT_Write(FONT_FRAN, TextColor, BackGrColor, 150, 32+16*(j%12), &string0[0]);
                AnalyzeTime(fno.ftime, &string0[0]);
                FONT_Write(FONT_FRAN, TextColor, BackGrColor, 240, 32+16*(j%12), &string0[0]);
                fileNoMax++;
            }
            j++;
            FileNo++;

        }
        f_closedir(&dir);
    }
    else
    {
        char error1[64];
        sprintf(error1, "error %s %d", SNDIR, fr);
        FONT_Write(FONT_FRAN, CurvColor, BackGrColor, 0, 0, error1);
        return -1;
        //CRASHF("Failed to open directory %s  %d", path, fr);
    }


    return fileNoMax;
}

char* SCREENSHOT_SelectFileName(void)
{
    static char fname[64];
    char path[128];
    uint32_t dfnum = 0;
    uint32_t retVal;

    f_mkdir(SNDIR);

    //Scan dir for snapshot files
    uint32_t fmax = 0;
    uint32_t fmin = 0xFFFFFFFFul;
    DIR dir = { 0 };
    FILINFO fno = { 0 };
    FRESULT fr = f_opendir(&dir, SNDIR);
    numfiles = 0;
    oldest = 0xFFFFFFFFul;
    fname[0] = '\0';
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
            const char *pdot = strchr(fno.fname, (int)'.');
            if (0 == pdot)
                continue;
            if (0 != strcasecmp(pdot, ".bmp") && 0 != strcasecmp(pdot, ".png"))
                continue; //Bypass files that are not bmp
            for (i = 0; i < 8; i++)
                if (!isdigit(fno.fname[i]))
                    break;
            if (i != 8)
                continue; //Bypass file names that are not 8-digit numbers
            numfiles++;
            //Now convert file name to number

            char* endptr;
            dfnum = strtoul(fno.fname, &endptr, 10);
            if (dfnum < fmin)
                fmin = dfnum;
            if (dfnum > fmax)
                fmax = dfnum;
        }
        f_closedir(&dir);
    }
    else
    {
        CRASHF("Failed to open directory %s", SNDIR);
    }

    oldest = fmin;
    dfnum = fmax + 1;
    sprintf(fname, "%08u", dfnum);

    if(KeyboardWindow(fname, 8, "Enter the file name")==0)
        fname[0]='\0';
    return fname;
}

void SCREENSHOT_DeleteOldest(void)
{
    char path[128];
    if (0xFFFFFFFFul != oldest && numfiles >= 100)
    {
        sprintf(path, "%s/%08d.s1p", SNDIR, oldest);
        f_unlink(path);
        sprintf(path, "%s/%08d.bmp", SNDIR, oldest);
        f_unlink(path);
        sprintf(path, "%s/%08d.png", SNDIR, oldest);
        f_unlink(path);
        numfiles = 0;
        oldest = 0xFFFFFFFFul;
    }
}

void Date_Time_Stamp(void){
char text[24], second1;
uint32_t mon;
short AMPM1;

    LCD_FillRect((LCDPoint){50,248}, (LCDPoint){479,271}, BackGrColor);
    if(NoDate==1) return;
    if(RTCpresent==1){
        getTime(&time, &second1,&AMPM1,0);
        getDate(&date);
    }
    else{
        time=GetInternTime(&second1);
        date=CFG_GetParam(CFG_PARAM_Date);
    }
    mon=date%10000;
    sprintf(text, "%04d %02d %02d ", date/10000,mon/100, mon%100);
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 100, 252, text);
    sprintf(text, "%02d:%02d:%02d ", time/100, time%100, second1);
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 180, 252, text);

}

void SCREENSHOT_Save(const char *fname)
{
    char path[64];
    char wbuf[256];
    FRESULT fr = FR_OK;
    FIL fo = { 0 };
    sprintf(path, "%s.bmp", fname);
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 300, 252, path);

    SCB_CleanDCache_by_Addr((uint32_t*)LCD_FB_START_ADDRESS, BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4); //Flush and invalidate D-Cache contents to the RAM to avoid cache coherency
    Sleep(10);

    BSP_LCD_DisplayOff();
    f_mkdir(SNDIR);

    //Now write screenshot as bitmap
    sprintf(path, "%s/%s.bmp", SNDIR, fname);

    fr = f_open(&fo, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (FR_OK != fr)
        CRASHF("Failed to open file %s", path);
    uint32_t bw;
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
    BSP_LCD_DisplayOn();
    return;
CRASH_WR:
    BSP_LCD_DisplayOn();
    CRASHF("Failed to write to file %s", path);
}

//Custom allocators for LodePNG using SDRAM heap
#include "sdram_heap.h"
void* lodepng_malloc(size_t size)
{
    return SDRH_malloc(size);
}

void* lodepng_realloc(void* ptr, size_t new_size)
{
    return SDRH_realloc(ptr, new_size);
}

void lodepng_free(void* ptr)
{
    SDRH_free(ptr);
}

//Exhange R and B colors for proper PNG encoding
static void _Change_B_R(uint32_t* image)
{
    uint32_t i;
    uint32_t endi = (uint32_t)LCD_GetWidth() * (uint32_t)LCD_GetHeight();
    for (i = 0; i < endi; i++)
    {
        uint32_t c = image[i];
        uint32_t r = (c >> 16) & 0xFF;
        uint32_t b = c & 0xFF;
        image[i] = (c & 0xFF00FF00) | r | (b << 16);
    }
}

void SCREENSHOT_SavePNG(const char *fname)
{
    char path[64];
    char wbuf[256];
    FRESULT fr = FR_OK;
    FIL fo = { 0 };
    sprintf(path, "%s.png", fname);
    FONT_Write(FONT_FRAN, TextColor, BackGrColor, 300, 252, path);
    uint8_t* image = LCD_Push();
    if (0 == image)
        CRASH("LCD_Push failed");

    _Change_B_R((uint32_t*)image);

    uint8_t* png = 0;
    size_t pngsize = 0;

    BSP_LCD_DisplayOff();
    uint32_t error = lodepng_encode32(&png, &pngsize, image, LCD_GetWidth(), LCD_GetHeight());
    if (error)
        CRASHF("lodepng_encode failed: %u ", error);

    f_mkdir(SNDIR);
    sprintf(path, "%s/%s.png", SNDIR, fname);
    fr = f_open(&fo, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (FR_OK != fr)
        CRASHF("Failed to open file %s", path);
    uint32_t bw;
    fr = f_write(&fo, png, pngsize, &bw);
    if (FR_OK != fr)
        CRASHF("Failed to write to file %s", path);
    f_close(&fo);
    BSP_LCD_DisplayOn();

    lodepng_free(png);
    _Change_B_R((uint32_t*)image);
    LCD_Pop();
}

extern unsigned lodepng_decode32(unsigned char** out, unsigned* w, unsigned* h,
                          const unsigned char* in, size_t insize);

static unsigned char* inBuf;
static unsigned char InBuf[8000];

void SCREENSHOT_ShowPicture(uint16_t Pointer1)
{
char path[255];
char FileName[13];
char error1[64];
int8_t i;
unsigned w,h,result1;
uint8_t* OutBuf=0; //&bmpFileBuffer[0];

    while(TOUCH_IsPressed());
    strcpy(FileName,(char*) &fileNames[Pointer1][0]);// *********************************
    sprintf(path, "%s/%s", SNDIR, (char*) &fileNames[Pointer1][0]);

    FONT_Write(FONT_FRAN, CurvColor, BackGrColor, 200, 0, path);
    Sleep(150);//1500

    FIL fo = { 0 };

    FRESULT fr = f_open(&fo, path, FA_READ);

    if (FR_OK != fr)
        return;
    uint32_t br = 0;
    const char *pdot = strchr(path, (int)'.');

    if (0 == strcasecmp(pdot, ".bmp"))
    {
        fr = f_read(&fo, bmpFileBuffer, SCREENSHOT_FILE_SIZE, &br);
        f_close(&fo);

    if (br != SCREENSHOT_FILE_SIZE || FR_OK != fr)
        return;

    LCD_DrawBitmap(LCD_MakePoint(0,0), bmpFileBuffer, SCREENSHOT_FILE_SIZE);
    }

    else if(0 == strcasecmp(pdot, ".png")){
        fr = f_read(&fo, bmpFileBuffer, fo.fsize, &br);
        if(fr!=0){
            sprintf(error1, "error3  %d", fr);
            FONT_Write(FONT_FRAN, CurvColor, BackGrColor, 400, 0, error1);
        }
        else{

            result1=lodepng_decode24(&OutBuf, &w, &h, bmpFileBuffer, fo.fsize);
            f_close(&fo);

            if(result1==0){
               PixPict(0,0,&OutBuf[0]);
               lodepng_free(OutBuf);
            }
            else {

                sprintf(error1, "error4  %d", result1);
                FONT_Write(FONT_FRAN, CurvColor, BackGrColor, 400, 0, error1);
            }
        }

    }
    //LCD_FillRect(LCD_MakePoint(0, 267),LCD_MakePoint(479, 271), LCD_COLOR_BLACK);
    while (!TOUCH_IsPressed())
        Sleep(50);
    LCD_FillAll(LCD_BLACK);
  //  while (TOUCH_IsPressed())
        Sleep(50);
}

void SCREENSHOT_DeleteFile(uint16_t Pointer1){
static char path[32];
static LCDPoint pt;
char FileName[13];

    while(TOUCH_IsPressed());
    strcpy(FileName,(char*) &fileNames[Pointer1][0]);// *********************************
    sprintf(path, "%s/%s", SNDIR, (char*) &fileNames[Pointer1][0]);


     FONT_Write(FONT_FRAN, CurvColor, BackGrColor, 200, 0, "delete ?? : ");
     FONT_Write(FONT_FRAN, CurvColor, BackGrColor, 280, 0, &path[0]);

     while (!TOUCH_IsPressed()) Sleep(50);
     if (TOUCH_Poll(&pt))
        {
            if ((pt.y >230)&&(pt.x>380))
            {
                 f_unlink(path);

            }

        }
    LCD_FillRect(LCD_MakePoint(200, 0),LCD_MakePoint(479,30), LCD_COLOR_BLACK);
}
