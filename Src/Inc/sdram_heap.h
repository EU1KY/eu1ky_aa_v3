/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef _SDRAM_HEAP_H_
#define _SDRAM_HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

void* SDRH_malloc(size_t nbytes);
void SDRH_free(void* ptr);
void* SDRH_realloc(void* ptr, size_t nbytes);
void* SDRH_calloc(size_t nbytes);

#ifdef __cplusplus
}
#endif

#endif //_SDRAM_HEAP_H_
