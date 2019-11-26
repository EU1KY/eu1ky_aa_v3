/*********************************************************************
----------------------------------------------------------------------
File    : syscalls.c
Purpose : Reentrant functions for newlib to enable printf.
          printf will write to the SWO Stimulus port 0
          on Cortex-M3/4 targets.
Notes   : SWO Output should work on every Cortex-M. In case of errors
          check the defines for the debug unit.
          __heap_start__ and __heap_end__ have to be defined
          in the linker file.
          printf via Newlib needs about 30KB extra memory space.
--------- END-OF-HEADER --------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <reent.h>
#include <sys/stat.h>

/*********************************************************************
*
*       Defines for Cortex-M debug unit
*/
#define ITM_STIM_U32 (*(volatile uint32_t *)0xE0000000)    // Stimulus Port Register word acces
#define ITM_STIM_U8  (*(volatile uint8_t  *)0xE0000000)    // Stimulus Port Register byte acces
#define ITM_ENA      (*(volatile uint32_t *)0xE0000E00)    // Control Register
#define ITM_TCR      (*(volatile uint32_t *)0xE0000E80)    // Trace control register
#define DHCSR        (*(volatile uint32_t *)0xE000EDF0)    // Debug Halting Control Status Register
#define DEMCR        (*(volatile uint32_t *)0xE000EDFC)    // Debug Exception Monitor Control Register

char *heap_end = NULL;

/*********************************************************************
*       _write_r()
* Purpose
*   Reentrant write function. Outputs the data to Stimulus port 0.
*/
_ssize_t _write_r (struct _reent *r, int file, const void *ptr, size_t len)
{
    size_t i;
    const uint8_t *p;

    p = (const uint8_t*) ptr;

    for (i = 0; i < len; i++)
    {
        //
        // Check if SWO is enabled / Debugger is connected
        //
        if (   (DHCSR & 1) != 1
            || (DEMCR & (1 << 24)) == 0
            || (ITM_TCR & (1u << 22)) != 0
            || (ITM_ENA & 1) == 0)
        {
            return len;
        }
        while ((ITM_STIM_U8 & 1) == 0)
        {
            ;
        }
        ITM_STIM_U8 = *p++;
    }
    return len;
}

/*********************************************************************
*       _sbrk_r()
* Purpose
*   Allocates memory on the heap.
*/
void * _sbrk_r(struct _reent *_s_r, ptrdiff_t nbytes)
{
    extern char  __heap_start__;
    extern char  __heap_end__;
    char* base;

    if (heap_end == NULL)
    {
        heap_end = &__heap_start__;
    }
    base = heap_end;
    if (heap_end + nbytes > &__heap_end__)
    {
        return 0; // Heap overflow
    }
    heap_end += nbytes;
    return base;
}

/*********************************************************************
*
*       Function dummies.
*       Return a default value.
*
*/

_ssize_t _read_r(struct _reent *r, int file, void *ptr, size_t len)
{
    return 0;
}

int _close_r(struct _reent *r, int file)
{
    return -1;
}

_off_t _lseek_r(struct _reent *r, int file, _off_t ptr, int dir)
{
    return (_off_t)0;
}

int _fstat_r(struct _reent *r, int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

int isatty(int file)
{
    return 1;
}

__attribute__((weak)) void _exit (int a)
{
    while(1) {};
}

int _kill (int a, int b)
{
    return 0;
}

int _getpid(int a)
{
    return 0;
}

int _isatty(int file)
{
    return 0;
}



