/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef PANVSWR2_H_
#define PANVSWR2_H_

typedef enum
{
    BS200, BS400, BS800, BS1600, BS2M, BS4M, BS8M, BS16M, BS20M, BS40M
} BANDSPAN;

extern const char* BSSTR[];
extern const char* BSSTR_HALF[];
extern const uint32_t BSVALUES[];

void PANVSWR2_Proc(void);

#endif
