#include "touch.h"
#include "stm32746g_discovery_ts.h"

void TOUCH_Init(void)
{
    BSP_TS_Init(FT5336_MAX_WIDTH, FT5336_MAX_HEIGHT);
}

uint8_t TOUCH_Poll(LCDPoint* pCoord)
{
    TS_StateTypeDef ts = { 0 };
    BSP_TS_GetState(&ts);
    if (ts.touchDetected)
    {
        pCoord->x = ts.touchX[0];
        pCoord->y = ts.touchY[0];
    }
    return ts.touchDetected;
}

uint8_t TOUCH_IsPressed(void)
{
    TS_StateTypeDef ts = { 0 };
    BSP_TS_GetState(&ts);
    return ts.touchDetected;
}
