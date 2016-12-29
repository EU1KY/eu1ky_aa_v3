/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

/*
    This code implements a simple memory heap utilizing a pool of equal size memory blocks.
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sdram_heap.h"

#define SDRH_BLKSIZE  128
#define SDRH_HEAPSIZE 0x100000  // Must be agreed with linker scrpt's _SDRAM_HEAP_SIZE, and must be multiple of SDRH_BLKSIZE !!!
#define SDRH_NBLOCKS (SDRH_HEAPSIZE / SDRH_BLKSIZE)
#define SDRH_ADDR(block) (SDRH_START + block * SDRH_BLKSIZE)

#if ((SDRH_BLKSIZE == 0) || (SDRH_HEAPSIZE % SDRH_BLKSIZE) != 0)
#error SDRAM heap size is incorrect
#endif

#if (SDRH_NBLOCKS >= 0x10000) // Must be less than 65536 to fit into uint16_t
#error SDRAM heap: too many blocks
#endif

extern uint8_t __sdram_heap_start__;
extern uint8_t __sdram_heap_end__;

static void* const SDRH_START = &__sdram_heap_start__;
static void* const SDRH_END = &__sdram_heap_end__;

static uint16_t __attribute__((section (".user_sdram"))) SDRH_descr[SDRH_NBLOCKS] = {0}; //Heap descriptor

//It is expected that ptr belongs to SDRAM HEAP designated area, and is aligned at the block start
static bool _isValidPtr(void* ptr)
{
    if ((ptr < SDRH_START) || (ptr >= SDRH_END))
        return false;
    if (((uint32_t)(ptr - SDRH_START) % SDRH_BLKSIZE) != 0)
        return false;
    return true;
}

static uint32_t _find_area(uint32_t nblocks)
{
    uint32_t i = 0;
    while (i < SDRH_NBLOCKS)
    {
        if (0 == SDRH_descr[i]) //found free
        {
            if (1 == nblocks)
                return i;
            //count how many free blocks exist ahead
            uint32_t j = i + 1;
            uint32_t nfound = 1;
            while (j < SDRH_NBLOCKS)
            {
                if (0 == SDRH_descr[j])
                {
                    if (++nfound == nblocks)
                    {
                        return i; //End of search: enough blocks found
                    }
                    j++;
                }
                else
                    break;
            }
            //not found
            if (j >= SDRH_NBLOCKS)
                break; //Return not found
            i = j + SDRH_descr[j]; //Skip over too small area and next occupied area
        }
        else
        {
            i += SDRH_descr[i]; //Skip over occupied area
        }
    }
    return 0xFFFFFFFFul; //Not found
}

void* SDRH_malloc(size_t nbytes)
{
    if (0 == nbytes)
        return 0;

    uint32_t nblocks;
    if (0 == nbytes % SDRH_BLKSIZE)
        nblocks = (nbytes / SDRH_BLKSIZE) + 1;
    else
        nblocks = (nbytes / SDRH_BLKSIZE);

    uint32_t start_block = _find_area(nblocks);

    if (start_block >= SDRH_NBLOCKS)
        return 0;
    void* addr = SDRH_ADDR(start_block);
    while (nblocks)
    {
        SDRH_descr[start_block++] = nblocks--;
    }
    return addr;
}

void SDRH_free(void* ptr)
{
    if (!_isValidPtr(ptr))
        return;
    uint32_t block = (ptr - SDRH_START) / SDRH_BLKSIZE;
    uint32_t nblocks = SDRH_descr[block];
    if (block + nblocks > SDRH_NBLOCKS) //Should not happen, but let's take care of this
        nblocks = SDRH_NBLOCKS - block;
    while (nblocks--)
    {
        SDRH_descr[block++] = 0;
    }
}

void* SDRH_realloc(void* ptr, size_t nbytes)
{
    if (0 == nbytes)
    {// If size is zero, the memory previously allocated at ptr is deallocated as
     // if a call to free was made, and a null pointer is returned.
        SDRH_free(ptr);
        return 0;
    }
    if (0 == ptr) //In case that ptr is a null pointer, the function behaves like malloc
        return SDRH_malloc(nbytes);

    uint32_t nblocks;
    if (0 == nbytes % SDRH_BLKSIZE)
        nblocks = (nbytes / SDRH_BLKSIZE) + 1;
    else
        nblocks = (nbytes / SDRH_BLKSIZE);

    //Check if ptr already has enough memory to fit nbytes
    uint32_t block = (ptr - SDRH_START) / SDRH_BLKSIZE;
    if (SDRH_descr[block] >= nblocks)
        return ptr; //No need for realloc, the allocated block fits the requested number of bytes

    void *pnew = SDRH_malloc(nbytes); //Allocate new block
    if (0 == pnew)
        return pnew; //Failed to reallocate
    memcpy(pnew, ptr, SDRH_descr[block] * SDRH_BLKSIZE); //Copy old data to new block
    SDRH_free(ptr); //Free old block
    return pnew;
}

void* SDRH_calloc(size_t nbytes)
{
    void* ptr = SDRH_malloc(nbytes);
    if (0 != ptr)
    {
        memset(ptr, 0, nbytes);
    }
    return ptr;
}
