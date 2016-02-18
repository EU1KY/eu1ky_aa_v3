#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

#define AAVERSION "3.0d"

typedef enum
{
    CFG_PARAM_PAN_F1, //Initial frequency for panoramic window
    CFG_PARAM_SPAN,   //Span for panoramic window
    CFG_PARAM_3,

    //---------------------
    CFG_NUM_PARAMS
} CFG_PARAM_t;

void CFG_Init(void);
uint32_t CFG_GetParam(CFG_PARAM_t param);
void CFG_SetParam(CFG_PARAM_t param, uint32_t value);
void CFG_Flush(void);

#endif // _CONFIG_H_
