#include "fftwnd.h"
#include "LCD.h"
#include "font.h"
#include "touch.h"
#include "hit.h"

static void FFTWND_SwitchWindowing(void)
{

}

static void FFTWND_ExitWnd(void)
{

}

static void FFTWND_SwitchDispMode(void)
{

}

static const struct HitRect hitArr[] =
{
    HITRECT(0, 0, 479, 140, FFTWND_SwitchWindowing),
    HITRECT(0, 200, 50, 279, FFTWND_ExitWnd),
    HITRECT(0, 140, 479, 279, FFTWND_SwitchDispMode),
    HITEND
};

void FFTWND_Proc(void)
{
}
