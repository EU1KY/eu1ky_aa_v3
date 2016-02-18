#ifndef _CRASH_H_
#define _CRASH_H
#include <stdio.h>

void CRASH(const char* txt);
#define CRASHF(fmt, ...) do {char __str[256]; sprintf(__str, fmt, __VA_ARGS__); CRASH(__str); } while ((void)0, 0)
#endif
