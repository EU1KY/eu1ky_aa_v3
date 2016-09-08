/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef _MATCH_H_
#define _MATCH_H_

#include <stdint.h>
#include <complex.h>

/// L-Network solution structure
typedef struct
{
    float XPS; ///< Reactance parallel to source (can be NAN if not applicable)
    float XS;  ///< Serial reactance (can be 0.0 if not applicable)
    float XPL; ///< Reactance parallel to load (can be NAN if not applicable)
} MATCH_S;

/**
    @brief   Calculate L-Networks on ideal lumped elements for given load impedance
    @param   ZL Load impedance
    @param   pResult array of 4 MATCH_S structures
    @return  0 if no solution available (or not needed), 1..4 for number of solutions found and filled into pResult
*/
uint32_t MATCH_Calc(float complex ZL, MATCH_S *pResult);


#endif // _MATCH_H_
