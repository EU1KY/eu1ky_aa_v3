//#define __GNUC__
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery.h"
#include "stm32f7xx_hal_def.h"
#include "stm32f7xx_hal_tim.h"
#include "panvswr2.h"
#include "FreqCounter.h"
#include "main.h"

#define preScaler 8;

static void MX_TIM5_Init(void);
extern int Sleeping;
void P_TIM2_TIMER(uint16_t prescaler, uint16_t periode);
void P_TIM2_NVIC(void);
void UB_TIMER2_Init(uint16_t prescaler, uint16_t periode);
void UB_TIMER2_Start();
//void Init_Tim4(void);

uint32_t Timer5Value;
uint32_t MeasFrequency;
uint16_t TimeFlag=0;
uint8_t IsInit=0;
uint8_t uhCaptureIndex;
//extern HAL_StatusTypeDef HAL_RTC_Init(RTC_HandleTypeDef *hrtc);
//extern HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT();

TIM_HandleTypeDef htim5;
TIM_ClockConfigTypeDef Tim5Clockdef;

/*                Measure Frequency          */
/* Author: Wolfgang Kiefer  DH1AKF           */
/* 02.12.2017                                */

void InitTimer2_4_5(void){
TimeFlag=0;
MX_TIM5_Init();
UB_TIMER2_Init_FRQ(1000); //1000 Hz
UB_TIMER2_Start();
}

static void Error_Handler(void)
{
  /* Turn LED3 on */
  //BSP_LED_On(LED3);
  while (1)
  {
  }
}

/**************************************************Init ************************************************************/
static void MX_TIM5_Init(void)
{
TIM_MasterConfigTypeDef sMasterConfig;
TIM_IC_InitTypeDef sConfigIC;
GPIO_InitTypeDef    sConfigGPIO;

sConfigGPIO.Alternate=GPIO_AF2_TIM5;
sConfigGPIO.Mode=GPIO_MODE_INPUT;
sConfigGPIO.Pin=GPIO_PIN_0;
sConfigGPIO.Pull=GPIO_NOPULL;
sConfigGPIO.Speed=GPIO_SPEED_FAST;

HAL_GPIO_Init(GPIOI,&sConfigGPIO);// GPIO I Pin 0 Alternate fkt.
__HAL_RCC_GPIOI_CLK_ENABLE();

htim5.Instance = TIM5;
htim5.Init.Prescaler = 0;
htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
htim5.Init.Period = 0xffff;
htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
if (HAL_TIM_IC_Init(&htim5) != HAL_OK)
{
//_Error_Handler(__FILE__, __LINE__);
}

sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
{
//_Error_Handler(__FILE__, __LINE__);
}
sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
sConfigIC.ICFilter = 15;
if (HAL_TIM_IC_ConfigChannel(&htim5, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
{
//_Error_Handler(__FILE__, __LINE__);
}
TIM5->OR=TIM5->OR & 0xFF3F;// Tim5 Ch4 = GPIO;
Tim5Clockdef.ClockSource=TIM_CLOCKSOURCE_INTERNAL;//TIM_CLOCKSOURCE_ETRMODE1
Tim5Clockdef.ClockPolarity=TIM_CLOCKPOLARITY_RISING;
Tim5Clockdef.ClockPrescaler=0;
Tim5Clockdef.ClockFilter=0;
HAL_TIM_ConfigClockSource(&htim5,&Tim5Clockdef);
HAL_NVIC_SetPriority(TIM5_IRQn, 3, 0);
HAL_NVIC_EnableIRQ(TIM5_IRQn);
HAL_TIM_Base_Start_IT(&htim5);
}

/*
//void MX_TIM4_Init(void)// ************************************************ TIM 4
{
TIM_MasterConfigTypeDef sMasterConfig;
TIM_IC_InitTypeDef sConfigIC;

htim4.Instance = TIM4;
htim4.Init.Prescaler = 0;
htim4.Init.CounterMode = TIM_COUNTERMODE_DOWN;
htim4.Init.Period = 0xffff;
htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
if (HAL_TIM_IC_Init(&htim4) != HAL_OK)
{
//_Error_Handler(__FILE__, __LINE__);
}

sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
{
//_Error_Handler(__FILE__, __LINE__);
}
sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
sConfigIC.ICFilter = 15;
if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
{
//_Error_Handler(__FILE__, __LINE__);
}
TIM4->OR=TIM4->OR & 0xFF3F;// Tim5 Ch4 = GPIO;
Tim4Clockdef.ClockSource=TIM_CLOCKSOURCE_INTERNAL;//TIM_CLOCKSOURCE_ETRMODE1
Tim4Clockdef.ClockPolarity=TIM_CLOCKPOLARITY_RISING;
Tim4Clockdef.ClockPrescaler=0;
Tim4Clockdef.ClockFilter=0;
HAL_TIM_ConfigClockSource(&htim4,&Tim4Clockdef);
HAL_NVIC_SetPriority(TIM4_IRQn, 6, 0);
HAL_NVIC_EnableIRQ(TIM4_IRQn);
HAL_TIM_Base_Start_IT(&htim4);
}

*/

/**********************************************call back function****************************************************************/
uint32_t uwIC2Value1, uwIC2Value2, uwDiffCapture;

void UB_TIMER5_ISR_CallBack(void)
{



if(uhCaptureIndex == 0)
{
/* Get the 1st Input Capture value */
uwIC2Value1 = HAL_TIM_ReadCapturedValue(&htim5, TIM_CHANNEL_4);
uhCaptureIndex = 1;
}
else if(uhCaptureIndex == 1)
{
/* Get the 2nd Input Capture value */
uwIC2Value2 = HAL_TIM_ReadCapturedValue(&htim5, TIM_CHANNEL_4);
/* Capture computation */
if (uwIC2Value2 > uwIC2Value1)
{
uwDiffCapture = (uwIC2Value2 - uwIC2Value1);
}
else if (uwIC2Value2 < uwIC2Value1)
{
/* 0xFFFF is max TIM3_CCRx value */
uwDiffCapture = ((0xFFFF - uwIC2Value1) + uwIC2Value2) + 1;
}
else
{
/* If capture values are equal, we have reached the limit of frequency
measures */
Error_Handler();
}
/* Frequency computation: for this example TIMx (TIM3) is clocked by
2xAPB1Clk */
Timer5Value = (2*HAL_RCC_GetPCLK1Freq()) / uwDiffCapture;
  uhCaptureIndex = 0;
}
}

void TIM5_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim5);

  // Callback Funktion aufrufen
  // Diese Funktion muss extern vom User benutzt werden !!
 // if(tim2_enable_flag!=0) {
    // nur wenn Timer aktiviert ist
    UB_TIMER5_ISR_CallBack();
 // }
}




/**********************************************call back function****************************************************************/
extern uint8_t SWRTone;
extern int BeepIsActive;

void UB_TIMER2_ISR_CallBack()// frequency 1 kHz || AUDIO: 200 Hz ..2000Hz
{
    if((SWRTone==1)||(BeepIsActive==1)){
         HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_2);
    }
    else{
        HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, 0);
        // Get the Input Capture value
        Timer5Value = TIM5->CNT;//Timer3Value
        TIM5->CNT=0;
        //* Frequency computation: for this example TIMx (TIM5) is clocked by
        //2xAPB1Clk
        TimeFlag += 1;
    }
}
// *************************************************************************************************************
//--------------------------------------------------------------
// File     : stm32_ub_tim2.c
// Datum    : 17.01.2016
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : http://mikrocontroller.bplaced.net/wordpress/?page_id=182
// CPU      : STM32F746
// IDE      : OpenSTM32
// GCC      : 4.9 2015q2
// Module   : CubeHAL
// Funktion : Timer-Funktionen per Timer2
//            (mit Callback-Funktion für externe ISR)
//
// Hinweis  : beim Timerevent wird die Funktion :
//            "UB_TIMER2_ISR_CallBack()" aufgerufen
//            diese Funktion muss vom User programmiert werden
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
//#include "stm32_ub_tim2.h"

//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------

//--------------------------------------------------------------

TIM_HandleTypeDef TIM2_Handle;
uint32_t tim2_enable_flag=0;



//--------------------------------------------------------------
// Init und Stop vom Timer mit Vorteiler und Counterwert
// prescaler : [0...65535]
// periode   : [0...65535]
//
// F_TIM = 100 MHz / (prescaler+1) / (periode+1)
//--------------------------------------------------------------
void UB_TIMER2_Init(uint16_t prescaler, uint16_t periode)
{
  // Timer flag löschen
  tim2_enable_flag=0;
  // Timer einstellen
  P_TIM2_TIMER(prescaler, periode);
  P_TIM2_NVIC();
}

//--------------------------------------------------------------
// Init und Stop vom Timer mit einem FRQ-Wert (in Hz)
// frq_hz : [1...50000000]
//
// Hinweis : die tatsächliche Frq weicht wegen Rundungsfehlern
//           etwas vom Sollwert ab (Bitte nachrechnen falls Wichtig)
//--------------------------------------------------------------
void UB_TIMER2_Init_FRQ(uint32_t frq_hz)
{
  uint32_t clk_frq;
  uint16_t prescaler, periode;
  uint32_t u_temp;
  float teiler,f_temp;
  SWRTone=1;
  // Clock-Frequenzen (PCLK1) auslesen
  clk_frq = HAL_RCC_GetPCLK1Freq();

  // check der werte
  if(frq_hz==0) frq_hz=1;
  if(frq_hz>clk_frq) frq_hz=clk_frq;

  // berechne teiler
  teiler=(float)(clk_frq<<1)/(float)(frq_hz);

  // berechne prescaler
  u_temp=(uint32_t)(teiler);
  prescaler=(u_temp>>16);

  // berechne periode
  f_temp=(float)(teiler)/(float)(prescaler+1);
  periode=(uint16_t)(f_temp-1);

  // werte einstellen
  UB_TIMER2_Init(prescaler, periode);
}

//--------------------------------------------------------------
// Timer starten
//--------------------------------------------------------------
void UB_TIMER2_Start(void)
{
  // Timer enable
  HAL_TIM_Base_Start_IT(&TIM2_Handle);
  // Timer flag setzen
  tim2_enable_flag=1;
}

//--------------------------------------------------------------
// Timer anhalten
//--------------------------------------------------------------
void UB_TIMER2_Stop(void)
{
  // Timer flag löschen
  tim2_enable_flag=0;
  // Timer disable
  HAL_TIM_Base_Stop_IT(&TIM2_Handle);
}

//--------------------------------------------------------------
// interne Funktion
// init vom Timer
//--------------------------------------------------------------
void P_TIM2_TIMER(uint16_t prescaler, uint16_t periode)
{
  // Clock enable
  __HAL_RCC_TIM2_CLK_ENABLE();

  TIM2_Handle.Instance = TIM2;
  TIM2_Handle.Init.Period            = periode;
  TIM2_Handle.Init.Prescaler         = prescaler;
  TIM2_Handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  TIM2_Handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
  TIM2_Handle.Init.RepetitionCounter = 0;
  HAL_TIM_Base_Init(&TIM2_Handle);

}

//--------------------------------------------------------------
// interne Funktion
// init vom Interrupt
//--------------------------------------------------------------
void P_TIM2_NVIC(void)
{
  // NVIC konfig
  HAL_NVIC_SetPriority(TIM2_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

//--------------------------------------------------------------
// Timer-Interrupt
// wird beim erreichen vom Counterwert aufgerufen
// die Callback funktion muss extern benutzt werden
//--------------------------------------------------------------
void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&TIM2_Handle);

  // Callback Funktion aufrufen
  // Diese Funktion muss extern vom User benutzt werden !!
  if(tim2_enable_flag!=0) {
    // nur wenn Timer aktiviert ist
    UB_TIMER2_ISR_CallBack();
  }
}
/*
// *******************************Timer4 ***********************************
// Clock pulse: 1 second
//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
void P_TIM4_TIMER(uint16_t prescaler, uint16_t periode);
void P_TIM4_NVIC(void);
//--------------------------------------------------------------
TIM_HandleTypeDef TIM4_Handle;
uint32_t tim4_enable_flag=0;



//--------------------------------------------------------------
// Init und Stop vom Timer mit Vorteiler und Counterwert
// prescaler : [0...65535]
// periode   : [0...65535]
//
// F_TIM = 100 MHz / (prescaler+1) / (periode+1)
//--------------------------------------------------------------
void UB_TIMER4_Init(uint16_t prescaler, uint16_t periode)
{
  // Timer flag löschen
  tim4_enable_flag=0;
  // Timer einstellen
  P_TIM4_TIMER(prescaler, periode);
  P_TIM4_NVIC();
}

//--------------------------------------------------------------
// Init und Stop vom Timer mit einem FRQ-Wert (in Hz)
// frq_hz : [1...50000000]
//
// Hinweis : die tatsächliche Frq weicht wegen Rundungsfehlern
//           etwas vom Sollwert ab (Bitte nachrechnen falls Wichtig)
//--------------------------------------------------------------
void UB_TIMER4_Init_FRQ(uint32_t frq_hz)
{
  uint32_t clk_frq;
  uint16_t prescaler, periode;
  uint32_t u_temp;
  float teiler,f_temp;

  // Clock-Frequenzen (PCLK1) auslesen
  clk_frq = HAL_RCC_GetPCLK1Freq();

  // check der werte
  if(frq_hz==0) frq_hz=1;
  if(frq_hz>clk_frq) frq_hz=clk_frq;

  // berechne teiler
  teiler=(float)(clk_frq<<1)/(float)(frq_hz);

  // berechne prescaler
  u_temp=(uint32_t)(teiler);
  prescaler=(u_temp>>16);

  // berechne periode
  f_temp=(float)(teiler)/(float)(prescaler+1);
  periode=(uint16_t)(f_temp-1);

  // werte einstellen
  UB_TIMER4_Init(prescaler, periode);
}

//--------------------------------------------------------------
// Timer starten
//--------------------------------------------------------------

volatile uint32_t secondsCounter;
void UB_TIMER4_Start(void)
{
  // Timer enable
  secondsCounter=0;
  HAL_TIM_Base_Start_IT(&TIM4_Handle);
  // Timer flag setzen
  tim4_enable_flag=1;
}

//--------------------------------------------------------------
// Timer anhalten
//--------------------------------------------------------------
void UB_TIMER4_Stop(void)
{
  // Timer flag löschen
  tim4_enable_flag=0;
  // Timer disable
  HAL_TIM_Base_Stop_IT(&TIM4_Handle);
}

//--------------------------------------------------------------
// interne Funktion
// init vom Timer
//--------------------------------------------------------------
void P_TIM4_TIMER(uint16_t prescaler, uint16_t periode)
{
  // Clock enable
  __HAL_RCC_TIM4_CLK_ENABLE();

  TIM4_Handle.Instance = TIM4;
  TIM4_Handle.Init.Period            = periode;
  TIM4_Handle.Init.Prescaler         = prescaler;
  TIM4_Handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  TIM4_Handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
  TIM4_Handle.Init.RepetitionCounter = 0;
  HAL_TIM_Base_Init(&TIM4_Handle);

}

//--------------------------------------------------------------
// interne Funktion
// init vom Interrupt
//--------------------------------------------------------------
void P_TIM4_NVIC(void)
{
  // NVIC konfig
  HAL_NVIC_SetPriority(TIM4_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

//--------------------------------------------------------------
// Timer-Interrupt
// wird beim erreichen vom Counterwert aufgerufen
// die Callback funktion muss extern benutzt werden
//--------------------------------------------------------------

//extern

void TIM4_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&TIM4_Handle);

  // Callback Funktion aufrufen
  // Diese Funktion muss extern vom User benutzt werden !!
  if(tim4_enable_flag!=0) {
    // nur wenn Timer aktiviert ist
    //UB_TIMER4_ISR_CallBack();
    if(Sleeping==1){
        secondsCounter++;
        if(secondsCounter>=10){
            secondsCounter=0;
            BSP_LCD_DisplayOn();
            Sleep(2000);
            Beep(1);
            BSP_LCD_DisplayOff();
        }
    }
  }
}
// **************************************************************************
* This example shows how you can use a timer interrupt.
 * In this case, the timer4 count up to 1000 and generate an IRQ which toggles a LED every second.
 * The clock frequency of the internal oscillator = 16MHz



void TIM4_IRQHandler(void)          // IRQ-Handler timer4
{
   // if (TIM4->SR & TIM_SR_UIF)       // flash on update event
    //    GPIOD->ODR ^= (1 << 15);   // Turn GPIOD pin15 (blue LED) ON in GPIO port output data register

    TIM4->SR = 0x0;              // reset the status register
    secondsCounter++;
}

void Init_Tim4(void)
{
    RCC -> AHB1ENR |= RCC_AHB1ENR_GPIODEN;       // Enable CLK for PortD in peripheral clock register (RCC_AHB1ENR)
    RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;        // enable TIM2 clock

  //  GPIOD -> MODER |= (1<<30);                 // Set pin 15 (blue LED)to be general purpose output in GPIO port mode register

    NVIC -> ISER[0] |= 1<< (TIM4_IRQn);        // enable the TIM2 IRQ

    TIM4 -> PSC = 21600;                         // prescaler = 16000 (timer2 clock = 16MHz clock / 16000 = 1000Hz)
    TIM4 -> DIER |= TIM_DIER_UIE;                // enable update interrupt
    TIM4 -> ARR = 10000;                      // count to 1000 (autoreload value 1000) --> overflow every 1 second
    TIM4 -> CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;   // autoreload on, counter enabled
    TIM4 -> EGR = 1;                             // trigger update event to reload timer registers

}
*/

    /*



  RCC_APB1ENR.PWREN = 1; // Enable RTC clock
  PWR_CR.DBP = 1; // Allow access to RTC
  RTC_WPR = 0xCA; // Unlock write protection
  RTC_WPR = 0x53;
  if (RCC_BDCR.RTCEN==0) { // if RTC is disabled,
    RCC_BDCR = 0x00010000; // Reset the backup domain
    RCC_BDCR = 0x00008101; // Set RTCEN, select LSE, set LSEON
  }
  while (RTC_ISR.RSF!=1) // Wait for RTC APB registers synchronization
    ;
  while (RCC_BDCR.LSERDY!=1) // Wait till LSE is ready
    ;

  EXTI_IMR.MR22 = 1; // Set EXTI22 for wake-up timer
  EXTI_RTSR.TR22 = 1;
  RTC_CR.WUTE = 0; // Stop wake-up timer, to access it
  while (RTC_ISR.WUTWF!=1) // Wait for wake-up timer access
    ;
  RTC_CR.WUTIE = 1; // Enable wake-up timer interrupt
  RTC_WUTR = 2047; // Set timer period
  RTC_CR &= ~(0x00000007); // Clear WUCKSEL, to select LSE as clock
  RTC_CR.WUTE = 10; // Enable wake-up timer

  RTC_ISR |= 0x00000080; // Enter initialization mode, bit 7
  while (RTC_ISR.INITF!=1) // Confirm status, bit 6
    ;
//  RTC_PRER = 0x7f00ff; // Set SynchPrediv to FF and AsynchPrediv to 7F
//  RTC_PRER = 0x7f00ff; // Write twice, for SyncPre and AsyncPre.
// Set time and date here if needed
  RTC_CRbits.FMT = 0; // Set FMT 24H format
  RTC_ISR &= ~0x00000080; // Exit initialization mode

  RTC_WPR = 0xFF; // Lock write protect
  PWR_CR.DBP = 0; // Inhibit RTC access
  NVIC_IntEnable(IVT_INT_RTC_WKUP); // Enable RTC wake up interrupt
}

// RTC interrupt service. Read out date and time to global variables.
void vRTCWakeupISR(void) iv IVT_INT_RTC_WKUP ics ICS_AUTO {
  PWR_CR.DBP = 1;
  RTC_ISR.WUTF = 0; // Clear wake-up event flag
  PWR_CR.DBP = 0;
  EXTI_PR.PR22 = 1; // Clear wake-up interrupt flag
  while (RTC_ISR.RSF!=1) // Wait for RTC APB registers synchronization
    ;


  u32RTCTime = RTC_TR; // Read time and date
  u32RTCDate = RTC_DR;
}

// Set time and date to RTC hardware.
void vSetRTC(void) {
  PWR_CR.DBP = 1;
  RTC_WPR = 0xCA; // Unlock write protection
  RTC_WPR = 0x53;
  RTC_ISR |= 0x00000080; // Enter initialization mode, bit 7
  while (RTC_ISR.INITF != 1) // Confirm status, bit 6
    ;
  RTC_TR = u32RTCTime;
  RTC_DR = u32RTCDate;
  RTC_CRbits.FMT = 0; // Set FMT 24H format
  RTC_ISR &= ~0x00000080; // Exit initialization mode
  RTC_WPR = 0xFF; // Enable the write protection for RTC registers

  */


