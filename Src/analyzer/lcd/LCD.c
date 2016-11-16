/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include "LCD.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"
//==============================================================================
//                  Module's static functions
//==============================================================================

static inline uint16_t _min(uint16_t a, uint16_t b)
{
    return a > b ? b : a;
}

static inline uint16_t _max(uint16_t a, uint16_t b)
{
    return a > b ? a : b;
}

static inline int _abs(int a)
{
    return (a < 0) ? -a : a;
}

uint16_t LCD_GetWidth(void)
{
    return BSP_LCD_GetXSize();
}

uint16_t LCD_GetHeight(void)
{
    return BSP_LCD_GetYSize();
}

void LCD_BacklightOn(void)
{
}

void LCD_BacklightOff(void)
{
}

void LCD_Init(void)
{
    RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
    /* LCD clock configuration */
    /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
    /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 Mhz */
    /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6Mhz */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 5; // Must be 2..7.
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_4;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    // Configure LCD : Only one layer is used
    BSP_LCD_Init();

    // Initialize LCD
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));

    // Enable LCD
    BSP_LCD_DisplayOn();

    // Select background layer
    BSP_LCD_SelectLayer(0);

    // Clear the Background Layer
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    // Select foreground layer
    BSP_LCD_SelectLayer(1);
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    BSP_LCD_SetTransparency(0, 255);
    BSP_LCD_SetTransparency(1, 255);
}

void LCD_TurnOn(void)
{
}

void LCD_TurnOff(void)
{
}

uint32_t LCD_IsOff(void)
{
    return (0 == (LCD_BL_CTRL_GPIO_PORT->ODR & LCD_BL_CTRL_PIN));
}

void LCD_WaitForRedraw(void)
{
    if (LCD_IsOff())
        return;
    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
}

void LCD_FillRect(LCDPoint p1, LCDPoint p2, LCDColor color)
{
    int32_t numPixels;
    int32_t i;
    uint16_t tmp;

    if (p1.x >= LCD_GetWidth()) p1.x = LCD_GetWidth() - 1;
    if (p2.x >= LCD_GetWidth()) p2.x = LCD_GetWidth() - 1;
    if (p1.y >= LCD_GetHeight()) p1.y = LCD_GetHeight() - 1;
    if (p2.y >= LCD_GetHeight()) p2.y = LCD_GetHeight() - 1;

    color |= 0xFF000000;
    BSP_LCD_SelectLayer(1);
    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillRect(p1.x, p1.y, p2.x - p1.x + 1, p2.y - p1.y + 1);
}

void LCD_InvertRect(LCDPoint p1, LCDPoint p2)
{
    if (p1.x >= LCD_GetWidth()) p1.x = LCD_GetWidth() - 1;
    if (p2.x >= LCD_GetWidth()) p2.x = LCD_GetWidth() - 1;
    if (p1.y >= LCD_GetHeight()) p1.y = LCD_GetHeight() - 1;
    if (p2.y >= LCD_GetHeight()) p2.y = LCD_GetHeight() - 1;
    uint16_t x, y;
    for (x = p1.x; x <= p2.x; x++)
    {
        for (y = p1.y; y <= p2.y; y++)
        {
            BSP_LCD_DrawPixel(x, y, BSP_LCD_ReadPixel(x, y) ^ 0x00FFFFFF);
        }
    }
}

void LCD_FillAll(LCDColor c)
{
    c |= 0xFF000000;
    BSP_LCD_Clear(c);
}

LCDColor LCD_MakeRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return LCD_RGB(r, g, b);
}

LCDPoint LCD_MakePoint(int x, int y)
{
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    LCDPoint pt = {x, y};
    return pt;
}

LCDColor LCD_ReadPixel(LCDPoint p)
{

    if (p.x >= LCD_GetWidth() || p.y >= LCD_GetHeight())
        return LCD_BLACK;
    return BSP_LCD_ReadPixel(p.x, p.y);
 }

void LCD_InvertPixel(LCDPoint p)
{
    LCDColor c = LCD_ReadPixel(p);
    LCD_SetPixel(p, (c ^ 0x00FFFFFFul) | 0xFF000000ul);
}

void LCD_SetPixel(LCDPoint p, LCDColor color)
{
    if (p.x >= LCD_GetWidth() || p.y >= LCD_GetHeight())
        return;
    BSP_LCD_DrawPixel(p.x, p.y, color | 0xFF000000ul);
}

void LCD_Rectangle(LCDPoint a, LCDPoint b, LCDColor c)
{
    c |= 0xFF000000ul;
    BSP_LCD_SetTextColor(c);
    BSP_LCD_DrawRect(a.x, a.y, b.x - a.x + 1, b.y - a.y + 1);
}

void LCD_Line(LCDPoint a, LCDPoint b, LCDColor color)
{
    if (a.x >= LCD_GetWidth())
        a.x = LCD_GetWidth() - 1;
    if (b.x >= LCD_GetWidth())
        b.x = LCD_GetWidth() - 1;
    if (a.y >= LCD_GetHeight())
        a.y = LCD_GetHeight() - 1;
    if (b.y >= LCD_GetHeight())
        b.y = LCD_GetHeight() - 1;
    color |= 0xFF000000ul;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DrawLine(a.x, a.y, b.x, b.y);
} //LCD_Line

void LCD_Circle(LCDPoint center, uint16_t r, LCDColor color)
{
    color |= 0xFF000000;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DrawCircle(center.x, center.y, r);
}

void LCD_FillCircle(LCDPoint center, uint16_t r, LCDColor color)
{
    color |= 0xFF000000;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillCircle(center.x, center.y, r);
}
