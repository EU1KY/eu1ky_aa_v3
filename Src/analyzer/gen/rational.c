#include "rational.h"

/*
 * Calculate best rational approximation for a given fraction
 * taking into account restricted register size, e.g. to find
 * appropriate values for a pll with 5 bit denominator and
 * 8 bit numerator register fields, trying to set up with a
 * frequency ratio of 3.1415, one would say:
 *
 * rational_best_approximation(31415, 10000,
 *              (1 << 8) - 1, (1 << 5) - 1, &n, &d);
 *
 * you may look at given_numerator as a fixed point number,
 * with the fractional part size described in given_denominator.
 *
 * for theoretical background, see:
 * http://en.wikipedia.org/wiki/Continued_fraction
 */

void rational_best_approximation(
    uint64_t given_numerator, uint64_t given_denominator,
    uint32_t max_numerator, uint32_t max_denominator,
    uint32_t *best_numerator, uint32_t *best_denominator)
{
    uint64_t n, d, n0, d0, n1, d1;
    n = given_numerator;
    d = given_denominator;
    n0 = d1 = 0;
    n1 = d0 = 1;
    for (;;)
    {
        uint64_t t, a;
        if ((n1 > max_numerator) || (d1 > max_denominator))
        {
            n1 = n0;
            d1 = d0;
            break;
        }
        if (d == 0)
            break;
        t = d;
        a = n / d;
        d = n % d;
        n = t;
        t = n0 + a * n1;
        n0 = n1;
        n1 = t;
        t = d0 + a * d1;
        d0 = d1;
        d1 = t;
    }
    *best_numerator = (uint32_t)n1;
    *best_denominator = (uint32_t)d1;
}
