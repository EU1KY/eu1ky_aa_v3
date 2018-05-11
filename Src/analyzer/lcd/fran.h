#ifndef franH
#define franH

#ifndef SMfgTypes
#define SMfgTypes

/*======= binary input =======*/
#define b2b(b7u,b6u,b5u,b4u,b3u,b2u,b1u,b0u) ((uint8_t)((b7u)*128u + (b6u)*64u + (b5u)*32u + (b4u)*16u + (b3u)*8u + (b2u)*4u + (b1u)*2u + (b0u)))

#include <stdint.h>

#define TCDATA const uint8_t
#endif

extern const uint8_t* const fran[256];
extern const uint8_t fran_height;
extern const uint8_t fran_spacing;
#endif
