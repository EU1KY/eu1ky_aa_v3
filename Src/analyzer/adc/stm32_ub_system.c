//--------------------------------------------------------------
// File     : stm32_ub_system.c
// Datum    : 12.07.2015
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F746
// IDE      : OpenSTM32
// GCC      : 4.9 2015q2
// Module   : CubeHAL
// Funktion : System Funktionen
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_system.h"



//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
static void P_MPU_Config(void);
static void P_CPU_CACHE_ENABLE(void);
static void P_SystemClock_Config(void);


//--------------------------------------------------------------
// init vom System
//--------------------------------------------------------------
void UB_System_Init(void)
{
  P_MPU_Config();

  P_CPU_CACHE_ENABLE();
  HAL_Init();
  P_SystemClock_Config();
}


//--------------------------------------------------------------
// einschalten vom Clock eines Ports
// LED_PORT : [GPIOA bis GPIOK]
//--------------------------------------------------------------
void UB_System_ClockEnable(GPIO_TypeDef* LED_PORT)
{
  if(LED_PORT==GPIOA) __GPIOA_CLK_ENABLE();
  if(LED_PORT==GPIOB) __GPIOB_CLK_ENABLE();
  if(LED_PORT==GPIOC) __GPIOC_CLK_ENABLE();
  if(LED_PORT==GPIOD) __GPIOD_CLK_ENABLE();
  if(LED_PORT==GPIOE) __GPIOE_CLK_ENABLE();
  if(LED_PORT==GPIOF) __GPIOF_CLK_ENABLE();
  if(LED_PORT==GPIOG) __GPIOG_CLK_ENABLE();
  if(LED_PORT==GPIOH) __GPIOH_CLK_ENABLE();
  if(LED_PORT==GPIOI) __GPIOI_CLK_ENABLE();
  if(LED_PORT==GPIOJ) __GPIOJ_CLK_ENABLE();
  if(LED_PORT==GPIOK) __GPIOK_CLK_ENABLE();
}

//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
static void P_MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as WT for SRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x20010000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
static void P_CPU_CACHE_ENABLE(void)
{
  /* Invalidate I-Cache : ICIALLU register*/
  SCB_InvalidateICache();

  /* Enable branch prediction */
  SCB->CCR |= (1 <<18);
  __DSB();

  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_InvalidateDCache();
  SCB_EnableDCache();
}

//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
static void P_SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);


  /* activate the OverDrive to reach the 180 Mhz Frequency */
   HAL_PWREx_ActivateOverDrive();

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}
/*
//--------------------------------------------------------------
void NMI_Handler(void)
{
}
//--------------------------------------------------------------
void HardFault_Handler(void)
{
 // Go to infinite loop when Hard Fault exception occurs
  while (1)
  {
  }
}
//--------------------------------------------------------------
void MemManage_Handler(void)
{
  // Go to infinite loop when Memory Manage exception occurs
  while (1)
  {
  }
}
//--------------------------------------------------------------
void BusFault_Handler(void)
{
  // Go to infinite loop when Bus Fault exception occurs
  while (1)
  {
  }
}
//--------------------------------------------------------------
void UsageFault_Handler(void)
{
  // Go to infinite loop when Usage Fault exception occurs
  while (1)
  {
  }
}
//--------------------------------------------------------------
void SVC_Handler(void)
{
}
//--------------------------------------------------------------
void DebugMon_Handler(void)
{
}
//--------------------------------------------------------------
void PendSV_Handler(void)
{
}
*/
