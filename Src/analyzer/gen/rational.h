#ifndef _RATIONAL_H_
#define _RATIONAL_H_

#include <stdint.h>

void rational_best_approximation(
    uint64_t given_numerator, uint64_t given_denominator,
    uint32_t max_numerator, uint32_t max_denominator,
    uint32_t *best_numerator, uint32_t *best_denominator);

#endif //_RATIONAL_H_
