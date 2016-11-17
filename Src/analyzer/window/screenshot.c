/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <stdio.h>
#include "LCD.h"
#include "touch.h"
#include "ff.h"
#include "textbox.h"
#include "screenshot.h"

extern void Sleep(uint32_t);

#define SCREENSHOT_FILE_SIZE 391734
uint8_t __attribute__((section (".user_sdram"))) __attribute__((used)) bmpFileBuffer[SCREENSHOT_FILE_SIZE];

//This is the prototype of the function that draws a screenshot from a file on SD card,
//and waits for tapping the screen to exit.
//fname must be without path and extension
//For example: SCREENSHOT_Window("00000214");
//TODO: add error handling
void SCREENSHOT_Window(const char* fname)
{
    char path[255];
    while(TOUCH_IsPressed());
    sprintf(path, "/aa/snapshot/%s.bmp", fname);
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
