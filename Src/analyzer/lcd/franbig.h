#ifndef franbigH
#define franbigH

#ifndef SMfgTypes
#define SMfgTypes
#include <stdint.h>
#define b2b(b7u,b6u,b5u,b4u,b3u,b2u,b1u,b0u) ((uint8_t)((b7u)*128u + (b6u)*64u + (b5u)*32u + (b4u)*16u + (b3u)*8u + (b2u)*4u + (b1u)*2u + (b0u)))

#define TCDATA const uint8_t
typedef const TCDATA* const * const TCLISTP;

#endif

/*======= Character pointers table =======*/
extern const uint8_t* const franbig[256];
extern const uint8_t franbig_height;
extern const uint8_t franbig_spacing;

#endif
