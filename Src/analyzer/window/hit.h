/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef _HIT_H_
#define _HIT_H_

#include <stdint.h>

struct HitRect
{
    uint32_t x1;
    uint32_t y1;
    uint32_t x2;
    uint32_t y2;
    void (*HitCallback)(void);
};

#define HITRECT(x0, y0, width, height, callback) \
    {(x0), (y0),  (x0) + (width) - 1, (y0) + (height) - 1, (callback)}

#define HITEND { 0xFFFFFFFFul, 0, 0xFFFFFFFFul, 0, 0 }

int HitTest(const struct HitRect* r, uint32_t x, uint32_t y);

#endif
