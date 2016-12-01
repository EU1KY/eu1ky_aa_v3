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
#include "libnsbmp.h"
#include "crash.h"
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
    /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 Mhz */ //Modified to 192/6 = 32 MHz th prevent flickering (EU1KY)
    /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6Mhz */ //Now 32/4 = 8MHz (EU1KY)
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 5; //Initially 5, but slowed it down to prevent LCD flickering due to AHB bus overload // Must be 2..7.
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

//===================================================================
//Drawing bitmap file based on libnsbmp
//===================================================================
#define BYTES_PER_PIXEL 4
#define TRANSPARENT_COLOR 0xFFFFFFFF

static void *bitmap_create(int width, int height, unsigned int state)
{
    (void) state;  /* unused */
    return (void*)0x2003F000; // Fake: the bitmap buffer is not used
}

static unsigned char *bitmap_get_buffer(void *bitmap)
{
    return (unsigned char*)bitmap;
}

static size_t bitmap_get_bpp(void *bitmap)
{
    (void) bitmap;  /* unused */
    return BYTES_PER_PIXEL;
}

static void bitmap_destroy(void *bitmap)
{
}

static LCDPoint _bmpOrigin;

static void bitmap_putcolor(unsigned int color32, unsigned int x, unsigned int y)
{
    //DBGPRINT("LCD bmp color: 0x%08x, x: %d, y: %d\n", color32, x, y);
    uint16_t r, g, b;
    r = (uint16_t)(color32 & 0xFF);
    g = (uint16_t)((color32 & 0xFF00) >> 8);
    b = (uint16_t)((color32 & 0xFF0000) >> 16);
    LCDColor clr = LCD_RGB(r, g, b);
    LCD_SetPixel(LCD_MakePoint(_bmpOrigin.x + x, _bmpOrigin.y + y), clr);
}

static bmp_bitmap_callback_vt bitmap_callbacks =
{
    bitmap_create,
    bitmap_destroy,
    bitmap_get_buffer,
    bitmap_get_bpp,
    bitmap_putcolor
};

void LCD_DrawBitmap(LCDPoint origin, const uint8_t *bmpData, uint32_t bmpDataSize)
{
    bmp_image bmp;
    bmp_result code;
    _bmpOrigin = origin;

    bmp_create(&bmp, &bitmap_callbacks);
    code = bmp_analyse(&bmp, bmpDataSize, bmpData);
    if(code != BMP_OK)
    {
        CRASHF("bmp_analyse failed, %d\n", code);
        goto cleanup;
    }
    code = bmp_decode(&bmp);
    if(code != BMP_OK)
    {
        CRASHF("bmp_decode failed, %d\n", code);
        if(code != BMP_INSUFFICIENT_DATA)
        {
            goto cleanup;
        }
    }
cleanup:
    bmp_finalise(&bmp);
}

//========================================================================================================
// Display backup (to avoid redraws after running child window)
// to be used in temporary windows and pop-ups
// Uses a stack organized in the unused SDRAM as a temporary storage
//========================================================================================================
#define LCD_BUF_STACK_DEPTH 4
static uint8_t __attribute__((section (".user_sdram"))) scrBufferStack[LCD_BUF_STACK_DEPTH][480 * 272 * 4];
static uint32_t lcd_scr_stack_num = 0;

uint8_t* LCD_Push(void)
{
    if (lcd_scr_stack_num >= LCD_BUF_STACK_DEPTH)
        return 0;
    BSP_LCD_CopyActiveLayerTo(&scrBufferStack[lcd_scr_stack_num][0]);
    return &scrBufferStack[lcd_scr_stack_num++][0];
}

void LCD_Pop(void)
{
    if (lcd_scr_stack_num == 0)
        return;
    BSP_LCD_CopyToActiveLayer(&scrBufferStack[--lcd_scr_stack_num][0]);
}
