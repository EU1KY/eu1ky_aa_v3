/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include "smith.h"
#include "LCD.h"
#include <math.h>
#include <complex.h>
#include <limits.h>

static float complex lastg = 2.f + 0.fi;
static int32_t lastx = -1;
static int32_t lasty = -1;
static int32_t lastradius = -1;
static int32_t lastxoffset = INT_MAX;
static int32_t lastyoffset = 0;

//R circle
static void _rcirc(float R, float R0, int32_t radius, int32_t x, int32_t y, LCDColor color)
{
    float g = (R - R0) / (R + R0);
    float roffset = radius * g;
    int32_t rr = (int32_t)((radius - roffset) / 2);
    LCD_Arc(x + radius - rr, y, rr, 0.f, 360.f, color);
}

// Draw Smith chart grid
// x and y are center coordinates, r is radius, color is grid color
void SMITH_DrawGrid(int32_t x, int32_t y, int32_t r, LCDColor color, LCDColor bgcolor)
{
    SMITH_ResetStartPoint();

    LCD_FillCircle(LCD_MakePoint(x, y), r, bgcolor);
    lastx = x;
    lasty = y;
    lastradius = r;

    LCD_Line(LCD_MakePoint(x-r, y), LCD_MakePoint(x + r, y), color);
    LCD_Arc(x, y, r, 0.f, 360.f, color);

    //
    _rcirc(10.f, 50.f, r, x, y, color);
    _rcirc(25.f, 50.f, r, x, y, color);
    _rcirc(50.f, 50.f, r, x, y, color);
    _rcirc(100.f, 50.f, r, x, y, color);
    _rcirc(200.f, 50.f, r, x, y, color);
    _rcirc(500.f, 50.f, r, x, y, color);

    // Y=1/50 circle
    LCD_Arc(x - r/2, y, r/2, 0.f, 360.f, color);

    // j50 arcs
    LCD_Arc(x + r, y - r, r, 180.f, 270.f, color);
    LCD_Arc(x + r, y + r, r, 90.f, 180.f, color);

    // j10 arcs
    LCD_Arc(x + r, y - 5 * r, 5 * r, 247.4f, 270.f, color);
    LCD_Arc(x + r, y + 5 * r, 5 * r, 90.f, 112.6f, color);

    // j25 arcs
    LCD_Arc(x + r, y - 2 * r, 2 * r, 217.f, 270.f, color);
    LCD_Arc(x + r, y + 2 * r, 2 * r, 90.f, 143.f, color);

    // j100 arcs
    LCD_Arc(x + r, y - r / 2, r / 2, 143.3f, 270.f, color);
    LCD_Arc(x + r, y + r / 2, r / 2, 90.f, 216.7f, color);

    // j200 arcs
    LCD_Arc(x + r, y - r / 4, r / 4, 119.f, 270.f, color);
    LCD_Arc(x + r, y + r / 4, r / 4, 90.f, 241.f, color);

    // j500 arcs
    LCD_Arc(x + r, y - r / 10, r / 10, 105.f, 270.f, color);
    LCD_Arc(x + r, y + r / 10, r / 10, 90.f, 255.f, color);

    // SWR = 2.0 circle
    LCD_Arc(x , y, r / 3, 0.f, 360.f, color);
}

void SMITH_ResetStartPoint(void)
{
    lastxoffset = INT_MAX;
}

void SMITH_DrawG(float complex G, LCDColor color)
{
    if (cabsf(G) > 1.0f)
        return;

    int32_t xoffset = (int32_t)(crealf(G) * lastradius);
    int32_t yoffset = (int32_t)(-cimagf(G) * lastradius);

    if (lastxoffset == INT_MAX)
    {
        LCD_SetPixel(LCD_MakePoint(lastx + xoffset, lasty + yoffset), color);
    }
    else
    {
        LCD_Line(LCD_MakePoint(lastx + lastxoffset, lasty + lastyoffset), LCD_MakePoint(lastx + xoffset, lasty + yoffset), color);
    }
    lastxoffset = xoffset;
    lastyoffset = yoffset;
}
