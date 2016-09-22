#include "touch.h"
#include "stm32746g_discovery_ts.h"
#include "stm32746g_discovery_lcd.h"
#include "config.h"

void TOUCH_Init(void)
{
    BSP_TS_Init(FT5336_MAX_WIDTH, FT5336_MAX_HEIGHT);
}

static uint32_t wakeup_touch(void)
{
    extern volatile uint32_t autosleep_timer;
    extern volatile uint32_t lcd_brightness;
    autosleep_timer = CFG_GetParam(CFG_PARAM_LOWPWR_TIME);
    //if (0 == (LCD_BL_CTRL_GPIO_PORT->ODR & LCD_BL_CTRL_PIN))
    if (0 == lcd_brightness)
    {
        //BSP_LCD_DisplayOn();
        //HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
        lcd_brightness = 15;
        TS_StateTypeDef ts = { 0 };
        do
        {
            BSP_TS_GetState(&ts);
        } while (ts.touchDetected);
        return 1;
    }
    return 0;
}

uint8_t TOUCH_Poll(LCDPoint* pCoord)
{
    TS_StateTypeDef ts = { 0 };
    BSP_TS_GetState(&ts);
    if (ts.touchDetected)
    {
        if (wakeup_touch())
            return 0;
        pCoord->x = ts.touchX[0];
        pCoord->y = ts.touchY[0];
    }
    return ts.touchDetected;
}

uint8_t TOUCH_IsPressed(void)
{
    TS_StateTypeDef ts = { 0 };
    BSP_TS_GetState(&ts);
    if (ts.touchDetected)
    {
        if (wakeup_touch())
            return 0;
    }
    return ts.touchDetected;
}
