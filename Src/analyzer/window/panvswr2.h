/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef PANVSWR2_H_
#define PANVSWR2_H_
// ** WK ** :
typedef enum
{
    BS2, BS4, BS10, BS20, BS40, BS100, BS200, BS400, BS1000, BS2M, BS4M, BS10M, BS20M, BS40M, BS100M
} BANDSPAN;

extern const char* BSSTR[];
extern const char* BSSTR_HALF[];
extern const uint32_t BSVALUES[];
static uint32_t multi_fr[5];
void PANVSWR2_Proc(void);
typedef struct
{
    uint32_t flo;
    uint32_t fhi;
} HAM_BANDS;
unsigned long GetUpper(int i);
unsigned long GetLower(int i);
int GetBandNr(unsigned long frequ);
void Sleep(uint32_t nms);
void Tune_SWR_Proc(void);
uint8_t AUDIO;
static uint8_t loudness;


#endif
