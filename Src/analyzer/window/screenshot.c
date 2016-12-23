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

#include "LCD.h"
#include "touch.h"
#include "ff.h"
#include "textbox.h"
#include "screenshot.h"
#include "crash.h"
#include "stm32746g_discovery_lcd.h"
#include "keyboard.h"
#include "lodepng.h"

extern void Sleep(uint32_t);

static const TCHAR *SNDIR = "/aa/snapshot";
static uint32_t oldest = 0xFFFFFFFFul;
static uint32_t numfiles = 0;

#define SCREENSHOT_FILE_SIZE 391734
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


//This is the prototype of the function that draws a screenshot from a file on SD card,
//and waits for tapping the screen to exit.
//fname must be without path and extension
//For example: SCREENSHOT_Window("00000214");
//TODO: add error handling
void SCREENSHOT_Show(const char* fname)
{
    char path[255];
    while(TOUCH_IsPressed());
    sprintf(path, "%s/%s.bmp", SNDIR, fname);
    FIL fo = { 0 };
    FRESULT fr = f_open(&fo, path, FA_READ);
    if (FR_OK != fr)
        return;
    uint32_t br = 0;
    fr = f_read(&fo, bmpFileBuffer, SCREENSHOT_FILE_SIZE, &br);
    f_close(&fo);
    if (br != SCREENSHOT_FILE_SIZE || FR_OK != fr)
        return;
    LCD_DrawBitmap(LCD_MakePoint(0,0), bmpFileBuffer, SCREENSHOT_FILE_SIZE);
    while (!TOUCH_IsPressed())
        Sleep(50);
    LCD_FillAll(LCD_BLACK);
    while (TOUCH_IsPressed())
        Sleep(50);
}

char* SCREENSHOT_SelectFileName(void)
{
    static char fname[64];
    char path[128];
    uint32_t dfnum = 0;

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

    KeyboardWindow(fname, 8, "Select screenshot file name");
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

void SCREENSHOT_Save(const char *fname)
{
    char path[64];
    char wbuf[256];
    FRESULT fr = FR_OK;
    FIL fo = { 0 };

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

//Custom allocators for LodePNG (TODO!)
#include <stdlib.h>
void* lodepng_malloc(size_t size)
{
  return malloc(size);
}

void* lodepng_realloc(void* ptr, size_t new_size)
{
  return realloc(ptr, new_size);
}

void lodepng_free(void* ptr)
{
  free(ptr);
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
