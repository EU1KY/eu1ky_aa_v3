/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef LCD_H_INCLUDED
#define LCD_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

///LCD point descriptor
typedef struct __attribute__((packed))
{
    uint16_t x;
    uint16_t y;
} LCDPoint;


///LCD color type
typedef uint32_t LCDColor;

///Convert 24-bit RGB color to 888 format with macro
#define LCD_RGB(r, g, b) ((LCDColor) ( 0xFF000000ul | \
                             (( ((uint32_t)(r)) & 0xFF) << 16) |  \
                             (( ((uint32_t)(g)) & 0xFF) << 8) |   \
                             (  ((uint32_t)(b)) & 0xFF)           \
                         ))

///Some useful color definitions
enum
{
    LCD_BLACK = LCD_RGB(0, 0, 0),
    LCD_GRAY = LCD_RGB(127, 127, 127),
    LCD_RED = LCD_RGB(255, 0, 0),
    LCD_GREEN = LCD_RGB(0, 255, 0),
    LCD_BLUE = LCD_RGB(0, 0, 255),
    LCD_YELLOW = LCD_RGB(255, 255, 0),
    LCD_PURPLE = LCD_RGB(255, 0, 255),
    LCD_CYAN = LCD_RGB(0, 255, 255),
    LCD_WHITE = LCD_RGB(255, 255, 255)
};

///Initialize hardware, turn on and fill display with black
void LCD_Init(void);

///Make LCDPoint from x and y coordinates
LCDPoint LCD_MakePoint(int x, int y);

///Make LCDColor from R, G and B components with function. See also
///LCD_RGB macro that does the same at compile time.
LCDColor LCD_MakeRGB(uint8_t r, uint8_t g, uint8_t b);

///Sets pixel at given point to given color
void LCD_SetPixel(LCDPoint p, LCDColor color);

///Fill rectangle with given corner points with given color
void LCD_FillRect(LCDPoint p1, LCDPoint p2, LCDColor color);

///Fill the entire display with given color
void LCD_FillAll(LCDColor c);

///Draw lines forming a rectangle with given corner points with given color
void LCD_Rectangle(LCDPoint a, LCDPoint b, LCDColor c);


void LCD_Circle(LCDPoint center, uint16_t r, LCDColor color);

void LCD_FillCircle(LCDPoint center, uint16_t r, LCDColor color);

///Draw arc using start and end in degrees (0 .. 360)
void LCD_DrawArc(int32_t x, int32_t y, int32_t radius, float astartdeg, float aenddeg, LCDColor color);

void LCD_VLine(LCDPoint p1, uint16_t lenght, LCDColor color);

void LCD_HLine(LCDPoint p1, uint16_t lenght, LCDColor color);

///Draw line between given points with given color
void LCD_Line(LCDPoint p1, LCDPoint p2, LCDColor c);

///Draw 3-pixel wide line between given points with given color
void LCD_Line3(LCDPoint a, LCDPoint b, LCDColor color);

///Turn on LCD and backlight
void LCD_TurnOn(void);

///Turn off backlight and switch LCD to power saving mode (but draving to its
///memory remains available)
void LCD_TurnOff(void);

///Turn on LCD backlight
void LCD_BacklightOn(void);

///Turn off LCD backlight
void LCD_BacklightOff(void);

///Invert color of display pixel
void LCD_InvertPixel(LCDPoint p);

void LCD_InvertRect(LCDPoint p1, LCDPoint p2);

LCDColor LCD_ReadPixel(LCDPoint p);

uint16_t LCD_GetWidth(void);

uint16_t LCD_GetHeight(void);

void LCD_WaitForRedraw(void);

uint32_t LCD_IsOff(void);

void LCD_DrawBitmap(LCDPoint origin, const uint8_t *bmpData, uint32_t bmpDataSize);

void LCD_ShowActiveLayerOnly(void);

// Functions that store and recover window bitmaps
// to be used in temporary windows and pop-ups

/** @brief Store LCD contents to the stack in SDRAM memory
    @return A pointer to the memory area where the image has been stored to
*/
uint8_t* LCD_Push(void);

/** @brief Restore last saved LCD contents from the stack in SDRAM memory */
void LCD_Pop(void);

#ifdef __cplusplus
}
#endif

#endif //LCD_H_INCLUDED
