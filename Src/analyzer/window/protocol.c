/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

// This file implements remote control protocol for the EU1KY antenna analyzer
// The device pretends a RigExpert AA-30 (emulates a subset of commands supported by it).


#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include "stm32f10x.h"
#include "touch.h"
#include "gen.h"
#include "protocol.h"
#include "btusart.h"
#include "dsp.h"
#include "LCD.h"
#include "touch.h"
#include "font.h"
#include "dbgprint.h"

void Sleep(uint32_t nms);

#define RXB_SIZE 64
static char rxstr[RXB_SIZE];
static char txstr[128];
static uint32_t rx_ctr = 0;
static char* rxstr_p;

static uint32_t _fCenter = 10000000ul;
static uint32_t _fSweep = 100000ul;

static char* _trim(char *str)
{
    char *end;
    while(isspace((int)*str))
        str++;
    if (*str == '\0')
        return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((int)*end))
        end--;
    *(end + 1) = '\0';
    return str;
}


static int PROTOCOL_RxCmd(void)
{
    int ch = BTUSART_Getchar();
    if (ch == EOF)
        return 0;
    else if (ch == '\r' || ch == '\n')
    {
        // command delimiter
        if (!rx_ctr)
            rxstr[0] = '\0';
        rx_ctr = 0;
        return 1;
    }
    else if (ch == '\0') //ignore it
        return 0;
    else if (ch == '\t')
        ch = ' ';
    else if (rx_ctr >= (RXB_SIZE - 1)) //skip all characters that do not fit in the rx buffer
        return 0;
    //Store character to buffer
    rxstr[rx_ctr++] = (char)(ch & 0xFF);
    rxstr[rx_ctr] = '\0';
    return 0;
}


void PROTOCOL_Proc(void)
{
    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 25, 60, "Remote control mode");
    FONT_Write(FONT_FRANBIG, LCD_WHITE, LCD_BLACK, 25, 100, "(emulates AA-170)");
    while(TOUCH_IsPressed());
    Sleep(250);

    //Purge contents of RX buffer
    while (EOF != BTUSART_Getchar());

    for (;;)
    {
        //Wait while command is received
        rx_ctr = 0;
        while(!PROTOCOL_RxCmd())
        {
            if (TOUCH_IsPressed())
            {
                Sleep(100);
                return;
            }
        }

        rxstr_p = _trim(rxstr);
        DBGPRINT("Protocol command: %s\n", rxstr_p);
        strlwr(rxstr_p);
        if (rxstr_p[0] == '\0') //empty command
        {
            BTUSART_PutString("ERROR\r");
            continue;
        }

        if (0 == strcmp("ver", rxstr_p))
        {
            BTUSART_PutString("AA-170 201\r"); //OK is needed?
            continue;
        }

        if (0 == strcmp("on", rxstr_p))
        {
            GEN_SetMeasurementFreq(GEN_GetLastFreq());
            BTUSART_PutString("OK\r");
            continue;
        }

        if (0 == strcmp("off", rxstr_p))
        {
            GEN_SetMeasurementFreq(0);
            BTUSART_PutString("OK\r");
            continue;
        }
        if (rxstr_p == strstr(rxstr_p, "am"))
        {//Amplitude setting
            BTUSART_PutString("OK\r");
            continue;
        }
        if (rxstr_p == strstr(rxstr_p, "ph"))
        {//Phase setting
            BTUSART_PutString("OK\r");
            continue;
        }
        if (rxstr_p == strstr(rxstr_p, "de"))
        {//Set delay
            BTUSART_PutString("OK\r");
            continue;
        }
        if (rxstr_p == strstr(rxstr_p, "fq"))
        {
            uint32_t FHz = 0;
            if(isdigit((int)rxstr_p[2]))
            {
                FHz = (uint32_t)atoi(&rxstr_p[2]);
            }
            else
            {
                BTUSART_PutString("ERROR\r");
                continue;
            }
            if (FHz < BAND_FMIN)
            {
                BTUSART_PutString("ERROR\r");
            }
            else
            {
                _fCenter = FHz;
                BTUSART_PutString("OK\r");
            }
            continue;
        }

        if (rxstr_p == strstr(rxstr_p, "sw"))
        {
            uint32_t sw = 0;
            if(isdigit((int)rxstr_p[2]))
            {
                sw = (uint32_t)atoi(&rxstr_p[2]);
            }
            else
            {
                BTUSART_PutString("ERROR\r");
                continue;
            }
            _fSweep = sw;
            BTUSART_PutString("OK\r");
            continue;
        }

        if (rxstr_p == strstr(rxstr_p, "frx"))
        {
            static uint32_t steps = 0; //protothreads require local vars to be static
            static uint32_t fint;
            static uint32_t fstep;

            if(isdigit((int)rxstr_p[3]))
            {
                steps = (uint32_t)atoi(&rxstr_p[3]);
            }
            else
            {
                BTUSART_PutString("ERROR\r");
                continue;
            }
            if (steps == 0)
            {
                BTUSART_PutString("ERROR\r");
                continue;
            }
            if (_fSweep / 2 > _fCenter)
                fint = 10;
            else
                fint = _fCenter - _fSweep / 2;
            fstep = _fSweep / steps;
            steps += 1;
            while (steps--)
            {
                static DSP_RX rx;
                DSP_Measure(fint, 1, 1, 10);
                rx = DSP_MeasuredZ();
                sprintf(txstr, "%.6f,%.2f,%.2f\r", ((float)fint) / 1000000., crealf(rx), cimagf(rx));

                BTUSART_PutString(txstr);
                while(BTUSART_IsTxBusy()) //To let the touch screen remain responsive
                {
                    if (TOUCH_IsPressed())
                    {
                        Sleep(100);
                        return;
                    }
                }
                fint += fstep;
            }
            BTUSART_PutString("OK\r");
            //FONT_Write(FONT_FRAN, LCD_BLACK, LCD_BLACK, 0, 0, "                           ");
            continue;
        }
        BTUSART_PutString("ERROR\r");
    }
}
