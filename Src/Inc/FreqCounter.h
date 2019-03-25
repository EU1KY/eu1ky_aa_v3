#ifndef FREQCOUNTER_H_
#define FREQCOUNTER_H_

void UB_TIMER2_Init_FRQ(uint32_t frq_hz);
void UB_TIMER2_Start();
void UB_TIMER2_Stop();
void InitTimer2_4_5(void);
uint16_t TimeFlag;

void UB_TIMER4_Init(uint16_t prescaler, uint16_t periode);
void UB_TIMER4_Init_FRQ(uint32_t frq_hz);
void UB_TIMER4_Start(void);
void UB_TIMER4_Stop(void);
void UB_TIMER4_ISR_CallBack(void);
#endif
