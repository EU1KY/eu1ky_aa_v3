/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "match.h"
#include "dsp.h"
#include "config.h"

static void quadratic_equation(float a, float b, float c, float *pResult)
{
    float d = b * b - 4 * a * c;
    if (d < 0)
    {
        pResult[0] = 0.f;
        pResult[1] = 0.f;
        return;
    }
    float sd = sqrtf(d);
    pResult[0] = (-b + sd) / (2.f * a);
    pResult[1] = (-b - sd) / (2.f * a);
}

// Calculate two solutions for ZL where R + X * X / R > R0
static void calc_hi(float R0, float complex ZL, MATCH_S *pResult)
{
    float Rl = crealf(ZL);
    float Xl = cimagf(ZL);
    float a = R0 - Rl;
    float b = 2 * Xl * R0;
    float c = R0 * (Xl * Xl + Rl * Rl);
    float xp[2];
    quadratic_equation(a, b, c, xp);
    //Found two impedances parallel to load

    //Now calculate serial impedances
    float complex ZZ1 = ZL * (0.f + xp[0] * I) / (ZL + (0.f + xp[0] * I));
    pResult[0].XS = -cimagf(ZZ1);
    pResult[0].XPS = NAN;
    pResult[0].XPL = xp[0];

    float complex ZZ2 = ZL * (0.f + xp[1] * I) / (ZL + (0.f + xp[1] * I));
    pResult[1].XS = -cimagf(ZZ2);
    pResult[1].XPS = NAN;
    pResult[1].XPL = xp[1];
}

// Calculate two solutions for ZL where R < R0
static void calc_lo(float R0, float complex ZL, MATCH_S *pResult)
{
    float Rl = crealf(ZL);
    float Xl = cimagf(ZL);
    // Calculate Xs
    float a = 1.f;
    float b = 2.f * Xl;
    float c = Rl * Rl + Xl * Xl - R0 * Rl;
    float xs[2];
    quadratic_equation(a, b, c, xs);
    //Got two serial impedances that change ZL to the Y.real = 1/R0

    float complex ZZ1 = ZL + xs[0] * I;
    float complex ZZ2 = ZL + xs[1] * I;

    //Now calculate impedances parallel to source
    float xp1 = cimagf(ZZ1 * R0 / (ZZ1 - R0));
    float xp2 = cimagf(ZZ2 * R0 / (ZZ2 - R0));

    pResult[0].XS = xs[0];
    pResult[0].XPS = xp1;
    pResult[0].XPL = NAN;

    pResult[1].XS = xs[1];
    pResult[1].XPS = xp2;
    pResult[1].XPL = NAN;
}

uint32_t MATCH_Calc(float complex ZL, MATCH_S *pResult)
{
    float vswr = DSP_CalcVSWR(ZL);
    float R0 = (float)CFG_GetParam(CFG_PARAM_R0);

    if ((vswr <= 1.1f) || (crealf(ZL) < 0.5) || ((cimagf(ZL) / crealf(ZL)) > 100.f))
        return 0; //Don't calculate for low VSWR, too low R, or for Q > 100

    if ((crealf(ZL) > (0.91f * R0)) && (crealf(ZL) < (1.1f * R0)))
    {//Only one solution is enough: just a serial reactance, this gives SWR < 1.1 if R is within the range 0.91 .. 1.1 of R0
        pResult[0].XPL = NAN;
        pResult[0].XPS = NAN;
        pResult[0].XS = -cimagf(ZL);
        return 1;
    }

    if (crealf(ZL) < R0)
    {
        //Calc Lo-Z solutions here
        calc_lo(R0, ZL, pResult);

        if ((crealf(ZL) + cimagf(ZL) * cimagf(ZL) / crealf(ZL)) > R0)
        {// Two more Hi-Z solutions exist
            calc_hi(R0, ZL, &pResult[2]);
            return 4;
        }
        return 2;
    }

    // Only two Hi-Z solutions
    calc_hi(R0, ZL, pResult);
    return 2;
}

void MATCH_XtoStr(uint32_t FHz, float X, char* str)
{
    if (isnanf(X))
    {
        strcpy(str, " --- ");
        return;
    }
    if (0.f == X || -0.f == X)
    {
        strcpy(str, "0");
        return;
    }
    if (X < 0.f)
    {
        float CpF = -1e12f / (2.f * M_PI * FHz * X);
        sprintf(str, "%.1f pF", CpF);
    }
    else
    {
        float LuH = (1e6f * X) / (2.f * M_PI * FHz);
        sprintf(str, "%.2f uH",  LuH);
    }
}
