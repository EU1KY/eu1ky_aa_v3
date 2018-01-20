//#define __GNUC__
#include <stdio.h>
#include <string.h>
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery.h"
#include "stm32f7xx_hal_def.h"
#include "stm32f7xx_hal_tim.h"

#define preScaler 8;

static void MX_TIM5_Init(void);
void P_TIM2_TIMER(uint16_t prescaler, uint16_t periode);
void P_TIM2_NVIC(void);
void UB_TIMER2_Init(uint16_t prescaler, uint16_t periode);
void UB_TIMER2_Start();

uint32_t Timer5Value;
uint32_t MeasFrequency;
uint16_t TimeFlag=0;
uint8_t IsInit=0;
uint8_t uhCaptureIndex;

TIM_HandleTypeDef htim5;
TIM_ClockConfigTypeDef Tim5Clockdef;


/*                Measure Frequency          */
/* Author: Wolfgang Kiefer  DH1AKF           */
/* 02.12.2017                                */

void InitMeasFrq(void){
TimeFlag=0;
MX_TIM5_Init();
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





/**************************************************Init ************************************************************/
/*static void MX_TIM5_Init1(void)
{
TIM_MasterConfigTypeDef sMasterConfig;
TIM_IC_InitTypeDef sConfigIC;
GPIO_InitTypeDef    sConfigGPIO;

sConfigGPIO.Alternate=GPIO_AF2_TIM5;
sConfigGPIO.Mode=GPIO_MODE_INPUT;
sConfigGPIO.Pin=GPIO_PIN_0;
sConfigGPIO.Pull=GPIO_NOPULL;
sConfigGPIO.Speed=GPIO_SPEED_FAST;

HAL_GPIO_Init(GPIOI,&sConfigGPIO);// GPIO I Pin 4 Alternate fkt.
__HAL_RCC_GPIOI_CLK_ENABLE();

htim5.Instance = TIM5;
htim5.Init.Prescaler = 0;
htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
htim5.Init.Period = 0xffffffff;
htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
//htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
if (HAL_TIM_IC_Init(&htim5) != HAL_OK)
{
//Error_Handler(__FILE__, __LINE__);
}
TIM5->OR=TIM5->OR & 0xFF3F;// Tim5 Ch4 = GPIO;
Tim5Clockdef.ClockSource=TIM_CLOCKSOURCE_INTERNAL;//TIM_CLOCKSOURCE_ETRMODE1
Tim5Clockdef.ClockPolarity=TIM_CLOCKPOLARITY_RISING;
Tim5Clockdef.ClockPrescaler=0;
Tim5Clockdef.ClockFilter=0;
HAL_TIM_ConfigClockSource(&htim5,&Tim5Clockdef);
*/
/*sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
{
//Error_Handler(__FILE__, __LINE__);
}*/
/*
__HAL_RCC_TIM5_CLK_ENABLE();
sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
sConfigIC.ICPrescaler = TIM_ICPSC_DIV8;
sConfigIC.ICFilter = 15;

if (HAL_TIM_IC_ConfigChannel(&htim5, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
{
//Error_Handler(__FILE__, __LINE__);
}
HAL_TIM_IC_Start(&htim5,4);
 __enable_irq();
}
*/
/**********************************************call back function****************************************************************/
/*
void UB_TIMER2_ISR_CallBack()// frequency 1 kHz
{
    // Get the Input Capture value
    Timer5Value = TIM5->CNT;//Timer3Value
    TIM5->CNT=0;
    //* Frequency computation: for this example TIMx (TIM5) is clocked by
    //2xAPB1Clk
    TimeFlag += 1;


}
/**************************************************************************************************************/
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
/*
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
*/
