/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef PANFREQ_H_
#define PANFREQ_H_

#include <stdint.h>
#include <stdbool.h>
#include "panvswr2.h"

bool PanFreqWindow(uint32_t *pFkhz, BANDSPAN *pBs);
void MultiSWR_Proc(void);
static uint8_t rqDel;

#endif
