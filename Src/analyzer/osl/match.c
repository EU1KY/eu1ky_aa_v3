/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <math.h>
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
static void calc_hi(float R0, float complex ZL, float *pResult)
{
    float Rl = crealf(ZL);
    float Xl = cimagf(ZL);
    float a = R0 - Rl;
    float b = 2 * Xl * R0;
    float c = R0 * (Xl * Xl + Rl * Rl);
    float xp[2];
    quadratic_equation(a, b, c, xp);
}

// Calculate two solutions for ZL where R + X * X / R < R0
static void calc_lo(float R0, float complex ZL)
{
    float Rl = crealf(ZL);
    float Xl = cimagf(ZL);
    // Calculate Xs
    float a = 1.f / Rl;
    float b = 2.f * Xl / Rl;
    float c = Rl + Xl * Xl / Rl - R0;
    float xs[2];
    quadratic_equation(a, b, c, xs);
    //Got two serial impedances that correct ZL to the Y.real = 1/Rs
    //Now calculate parallel impedances
    float b1 = -1. / (1. / (ZL + xs[0] * I) - 1. / R0);
    float b2 = -1. / (1. / (ZL + xs[1] * I) - 1. / R0);
    //return xs1, xs2, b1, b2
}

uint32_t MATCH_Calc(float complex ZL, MATCH_S *pResult)
{
    float vswr = DSP_CalcVSWR(ZL);
    if (vswr <= 1.1f || vswr >= 50.f)
        return 0; //Don't calculate for low and too high VSWR

    float R0 = (float)CFG_GetParam(CFG_PARAM_R0);

    if (fabsf(crealf(ZL) - R0) < R0 / 100.f)
    {//Only one solution: compensate with serial reactance
        pResult[0].XPL = NAN;
        pResult[0].XPS = NAN;
        pResult[0].XS = -cimagf(ZL);
        return 1;
    }

    if (fabsf(crealf(1.f / ZL) - 1.f / R0) <  0.01f / R0)
    {//Only one solution: compensate with parallel reactance
        float complex Z0 = R0 + 0.fi;
        pResult[0].XPL = cimagf(Z0 * ZL / (Z0 - ZL));
        pResult[0].XPS = NAN;
        pResult[0].XS = 0.f;
        return 1;
    }

    if (crealf(1.f / ZL) < 1.f / R0)
    {//Only two Lo-Z solutions
        //TODO
        return 2;
    }

    if (crealf(ZL) > R0)
    {// Only two Hi-Z solutions
        //TODO
        return 2;
    }

    //Four solutions exist
    //TODO
    return 4;
}
