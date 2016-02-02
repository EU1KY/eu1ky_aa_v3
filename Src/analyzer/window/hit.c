/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include "hit.h"

int HitTest(const struct HitRect* r, uint32_t x, uint32_t y)
{
    while (r->x1 != 0xFFFFFFFFul)
    {
        if (x >= r->x1 && x <= r->x2 && y >= r->y1 && y <= r->y2)
        {
            if (r->HitCallback)
                r->HitCallback();
            return 1;
        }
        ++r;
    }
    return 0;
}
