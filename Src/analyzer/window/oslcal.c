//OSL calibration window

#include "config.h"
#include "LCD.h"
#include "font.h"
#include "touch.h"
#include "hit.h"
#include "textbox.h"
#include "oslfile.h"

static uint32_t rqExit = 0;

void OSL_CalWnd(void)
{
    rqExit = 0;

    LCD_FillAll(LCD_BLACK);
    while (TOUCH_IsPressed());
}
