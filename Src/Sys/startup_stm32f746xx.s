/**
  ******************************************************************************
  * @file      startup_stm32f746xx.s
  * @author    MCD Application Team
  * Version    V1.0.0
  * Date       25-June-2015
  * @brief     STM32F746xx Devices vector table for GCC based toolchain.
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR address
  *                - Branches to main in the C library (which eventually
  *                  calls main()).
  *            After Reset the Cortex-M7 processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

    .syntax unified
    .cpu cortex-m7
    .fpu softvfp
    .thumb

.global  g_pfnVectors
.global  Default_Handler

/* start address for the initialization values of the .data section.
defined in linker script */
.word  _sidata
/* start address for the .data section. defined in linker script */
.word  _sdata
/* end address for the .data section. defined in linker script */
.word  _edata
/* start address for the .bss section. defined in linker script */
.word  _sbss
/* end address for the .bss section. defined in linker script */
.word  _ebss
/* stack used for SystemInit_ExtMemCtl; always internal RAM used */

/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called.
 * @param  None
 * @retval : None
*/

    .section  .text.Reset_Handler
    .weak  Reset_Handler
    .type  Reset_Handler, %function
    .thumb_func
Reset_Handler:
    ldr   sp, =_estack      /* set stack pointer */

/* Copy the data segment initializers from flash to SRAM */
    movs  r1, #0
    b  LoopCopyDataInit

CopyDataInit:
    ldr  r3, =_sidata
    ldr  r3, [r3, r1]
    str  r3, [r0, r1]
    adds  r1, r1, #4

LoopCopyDataInit:
    ldr  r0, =_sdata
    ldr  r3, =_edata
    adds  r2, r0, r1
    cmp  r2, r3
    bcc  CopyDataInit
    ldr  r2, =_sbss
    b  LoopFillZerobss

/* Zero fill the bss segment. */
FillZerobss:
    movs  r3, #0
    str  r3, [r2], #4

LoopFillZerobss:
    ldr  r3, = _ebss
    cmp  r2, r3
    bcc  FillZerobss

/* Call the clock system initialization function.*/
    bl  SystemInit
/* Call static constructors */
    bl __libc_init_array
/* Call the application's entry point.*/
    bl  main
    bx  lr
    .size  Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 * @param  None
 * @retval None
*/
    .section  .text.Default_Handler,"ax",%progbits
    .thumb_func
Default_Handler:
    // Load the address of the interrupt control register into r3.
    ldr r3,=0xe000ed04 //The address of the NVIC interrupt control register
    // Load the value of the interrupt control register into r2 from the address held in r3.
    ldr r2, [r3, #0]
    // The interrupt number is in the least significant byte - clear all other bits.
    uxtb r2, r2
    // Now sit in an infinite loop - the number of the executing interrupt is held in r2.
    b  .
    .size  Default_Handler, .-Default_Handler

/************************************************************************************
 *  HardFault_Handler()
 ************************************************************************************/
    .text
    .thumb
    .thumb_func
    .align 2
    .global    HardFault_Handler
    .type    HardFault_Handler, %function
HardFault_Handler:
    tst lr, #4
    ite eq
    mrseq r0, msp
    mrsne r0, psp
    ldr r1, [r0, #24] //R1 points to array of 8 registers? r0 r1 r2 r3 r12 lr pc psr
    //ldr r2,=GetRegistersFromStack
    //bx r2
    b .
    .pool
    .size  HardFault_Handler, .-HardFault_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex M7. Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
*******************************************************************************/
   .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object
  .size  g_pfnVectors, .-g_pfnVectors


g_pfnVectors:
    .word  _estack
    .word  Reset_Handler

    .word  NMI_Handler
    .word  HardFault_Handler
    .word  MemManage_Handler
    .word  BusFault_Handler
    .word  UsageFault_Handler
    .word  0
    .word  0
    .word  0
    .word  0
    .word  SVC_Handler
    .word  DebugMon_Handler
    .word  0
    .word  PendSV_Handler
    .word  SysTick_Handler

    /* External Interrupts */
    .word     WWDG_IRQHandler                   /* Window WatchDog              */
    .word     PVD_IRQHandler                    /* PVD through EXTI Line detection */
    .word     TAMP_STAMP_IRQHandler             /* Tamper and TimeStamps through the EXTI line */
    .word     RTC_WKUP_IRQHandler               /* RTC Wakeup through the EXTI line */
    .word     FLASH_IRQHandler                  /* FLASH                        */
    .word     RCC_IRQHandler                    /* RCC                          */
    .word     EXTI0_IRQHandler                  /* EXTI Line0                   */
    .word     EXTI1_IRQHandler                  /* EXTI Line1                   */
    .word     EXTI2_IRQHandler                  /* EXTI Line2                   */
    .word     EXTI3_IRQHandler                  /* EXTI Line3                   */
    .word     EXTI4_IRQHandler                  /* EXTI Line4                   */
    .word     DMA1_Stream0_IRQHandler           /* DMA1 Stream 0                */
    .word     DMA1_Stream1_IRQHandler           /* DMA1 Stream 1                */
    .word     DMA1_Stream2_IRQHandler           /* DMA1 Stream 2                */
    .word     DMA1_Stream3_IRQHandler           /* DMA1 Stream 3                */
    .word     DMA1_Stream4_IRQHandler           /* DMA1 Stream 4                */
    .word     DMA1_Stream5_IRQHandler           /* DMA1 Stream 5                */
    .word     DMA1_Stream6_IRQHandler           /* DMA1 Stream 6                */
    .word     ADC_IRQHandler                    /* ADC1, ADC2 and ADC3s         */
    .word     CAN1_TX_IRQHandler                /* CAN1 TX                      */
    .word     CAN1_RX0_IRQHandler               /* CAN1 RX0                     */
    .word     CAN1_RX1_IRQHandler               /* CAN1 RX1                     */
    .word     CAN1_SCE_IRQHandler               /* CAN1 SCE                     */
    .word     EXTI9_5_IRQHandler                /* External Line[9:5]s          */
    .word     TIM1_BRK_TIM9_IRQHandler          /* TIM1 Break and TIM9          */
    .word     TIM1_UP_TIM10_IRQHandler          /* TIM1 Update and TIM10        */
    .word     TIM1_TRG_COM_TIM11_IRQHandler     /* TIM1 Trigger and Commutation and TIM11 */
    .word     TIM1_CC_IRQHandler                /* TIM1 Capture Compare         */
    .word     TIM2_IRQHandler                   /* TIM2                         */
    .word     TIM3_IRQHandler                   /* TIM3                         */
    .word     TIM4_IRQHandler                   /* TIM4                         */
    .word     I2C1_EV_IRQHandler                /* I2C1 Event                   */
    .word     I2C1_ER_IRQHandler                /* I2C1 Error                   */
    .word     I2C2_EV_IRQHandler                /* I2C2 Event                   */
    .word     I2C2_ER_IRQHandler                /* I2C2 Error                   */
    .word     SPI1_IRQHandler                   /* SPI1                         */
    .word     SPI2_IRQHandler                   /* SPI2                         */
    .word     USART1_IRQHandler                 /* USART1                       */
    .word     USART2_IRQHandler                 /* USART2                       */
    .word     USART3_IRQHandler                 /* USART3                       */
    .word     EXTI15_10_IRQHandler              /* External Line[15:10]s        */
    .word     RTC_Alarm_IRQHandler              /* RTC Alarm (A and B) through EXTI Line */
    .word     OTG_FS_WKUP_IRQHandler            /* USB OTG FS Wakeup through EXTI line */
    .word     TIM8_BRK_TIM12_IRQHandler         /* TIM8 Break and TIM12         */
    .word     TIM8_UP_TIM13_IRQHandler          /* TIM8 Update and TIM13        */
    .word     TIM8_TRG_COM_TIM14_IRQHandler     /* TIM8 Trigger and Commutation and TIM14 */
    .word     TIM8_CC_IRQHandler                /* TIM8 Capture Compare         */
    .word     DMA1_Stream7_IRQHandler           /* DMA1 Stream7                 */
    .word     FMC_IRQHandler                    /* FMC                          */
    .word     SDMMC1_IRQHandler                 /* SDMMC1                       */
    .word     TIM5_IRQHandler                   /* TIM5                         */
    .word     SPI3_IRQHandler                   /* SPI3                         */
    .word     UART4_IRQHandler                  /* UART4                        */
    .word     UART5_IRQHandler                  /* UART5                        */
    .word     TIM6_DAC_IRQHandler               /* TIM6 and DAC1&2 underrun errors */
    .word     TIM7_IRQHandler                   /* TIM7                         */
    .word     DMA2_Stream0_IRQHandler           /* DMA2 Stream 0                */
    .word     DMA2_Stream1_IRQHandler           /* DMA2 Stream 1                */
    .word     DMA2_Stream2_IRQHandler           /* DMA2 Stream 2                */
    .word     DMA2_Stream3_IRQHandler           /* DMA2 Stream 3                */
    .word     DMA2_Stream4_IRQHandler           /* DMA2 Stream 4                */
    .word     ETH_IRQHandler                    /* Ethernet                     */
    .word     ETH_WKUP_IRQHandler               /* Ethernet Wakeup through EXTI line */
    .word     CAN2_TX_IRQHandler                /* CAN2 TX                      */
    .word     CAN2_RX0_IRQHandler               /* CAN2 RX0                     */
    .word     CAN2_RX1_IRQHandler               /* CAN2 RX1                     */
    .word     CAN2_SCE_IRQHandler               /* CAN2 SCE                     */
    .word     OTG_FS_IRQHandler                 /* USB OTG FS                   */
    .word     DMA2_Stream5_IRQHandler           /* DMA2 Stream 5                */
    .word     DMA2_Stream6_IRQHandler           /* DMA2 Stream 6                */
    .word     DMA2_Stream7_IRQHandler           /* DMA2 Stream 7                */
    .word     USART6_IRQHandler                 /* USART6                       */
    .word     I2C3_EV_IRQHandler                /* I2C3 event                   */
    .word     I2C3_ER_IRQHandler                /* I2C3 error                   */
    .word     OTG_HS_EP1_OUT_IRQHandler         /* USB OTG HS End Point 1 Out   */
    .word     OTG_HS_EP1_IN_IRQHandler          /* USB OTG HS End Point 1 In    */
    .word     OTG_HS_WKUP_IRQHandler            /* USB OTG HS Wakeup through EXTI */
    .word     OTG_HS_IRQHandler                 /* USB OTG HS                   */
    .word     DCMI_IRQHandler                   /* DCMI                         */
    .word     0                                 /* Reserved                     */
    .word     RNG_IRQHandler                    /* Rng                          */
    .word     FPU_IRQHandler                    /* FPU                          */
    .word     UART7_IRQHandler                  /* UART7                        */
    .word     UART8_IRQHandler                  /* UART8                        */
    .word     SPI4_IRQHandler                   /* SPI4                         */
    .word     SPI5_IRQHandler                   /* SPI5	 		              */
    .word     SPI6_IRQHandler                   /* SPI6   			          */
    .word     SAI1_IRQHandler                   /* SAI1						  */
    .word     LTDC_IRQHandler                   /* LTDC					      */
    .word     LTDC_ER_IRQHandler                /* LTDC error					  */
    .word     DMA2D_IRQHandler                  /* DMA2D    				      */
    .word     SAI2_IRQHandler                   /* SAI2                         */
    .word     QUADSPI_IRQHandler                /* QUADSPI                      */
    .word     LPTIM1_IRQHandler                 /* LPTIM1                       */
    .word     CEC_IRQHandler                    /* HDMI_CEC                     */
    .word     I2C4_EV_IRQHandler                /* I2C4 Event                   */
    .word     I2C4_ER_IRQHandler                /* I2C4 Error                   */
    .word     SPDIF_RX_IRQHandler               /* SPDIF_RX                     */

/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler.
* As they are weak aliases, any function with the same name will override
* this definition.
*
*******************************************************************************/
   .weak      NMI_Handler
   .thumb_set NMI_Handler,Default_Handler

   //.weak      HardFault_Handler
   //.thumb_set HardFault_Handler,Default_Handler

   .weak      MemManage_Handler
   .thumb_set MemManage_Handler,Default_Handler

   .weak      BusFault_Handler
   .thumb_set BusFault_Handler,Default_Handler

   .weak      UsageFault_Handler
   .thumb_set UsageFault_Handler,Default_Handler

   .weak      SVC_Handler
   .thumb_set SVC_Handler,Default_Handler

   .weak      DebugMon_Handler
   .thumb_set DebugMon_Handler,Default_Handler

   .weak      PendSV_Handler
   .thumb_set PendSV_Handler,Default_Handler

   .weak      SysTick_Handler
   .thumb_set SysTick_Handler,Default_Handler

   .weak      WWDG_IRQHandler
   .thumb_func
   .type    WWDG_IRQHandler, %function
WWDG_IRQHandler: b  .

   .weak      PVD_IRQHandler
   .thumb_func
   .type    PVD_IRQHandler, %function
PVD_IRQHandler:   b   .

   .weak      TAMP_STAMP_IRQHandler
   .thumb_func
   .type    TAMP_STAMP_IRQHandler, %function
TAMP_STAMP_IRQHandler: b .

   .weak      RTC_WKUP_IRQHandler
   .thumb_func
   .type    RTC_WKUP_IRQHandler, %function
RTC_WKUP_IRQHandler: b .

   .weak      FLASH_IRQHandler
   .thumb_func
   .type    FLASH_IRQHandler, %function
FLASH_IRQHandler: b .

   .weak      RCC_IRQHandler
   .thumb_func
   .type    RCC_IRQHandler, %function
RCC_IRQHandler: b .

   .weak      EXTI0_IRQHandler
   .thumb_func
   .type    EXTI0_IRQHandler, %function
EXTI0_IRQHandler: b .

   .weak      EXTI1_IRQHandler
   .thumb_func
   .type    EXTI1_IRQHandler, %function
EXTI1_IRQHandler: b .

   .weak      EXTI2_IRQHandler
   .thumb_func
   .type    EXTI2_IRQHandler, %function
EXTI2_IRQHandler: b .

   .weak      EXTI3_IRQHandler
   .thumb_func
   .type    EXTI3_IRQHandler, %function
EXTI3_IRQHandler: b .

   .weak      EXTI4_IRQHandler
   .thumb_func
   .type    EXTI4_IRQHandler, %function
EXTI4_IRQHandler: b .

   .weak      DMA1_Stream0_IRQHandler
   .thumb_func
   .type    DMA1_Stream0_IRQHandler, %function
DMA1_Stream0_IRQHandler: b .

   .weak      DMA1_Stream1_IRQHandler
   .thumb_func
   .type    DMA1_Stream1_IRQHandler, %function
DMA1_Stream1_IRQHandler: b .

   .weak      DMA1_Stream2_IRQHandler
   .thumb_func
   .type    DMA1_Stream2_IRQHandler, %function
DMA1_Stream2_IRQHandler: b .

   .weak      DMA1_Stream3_IRQHandler
   .thumb_func
   .type    DMA1_Stream3_IRQHandler, %function
DMA1_Stream3_IRQHandler: b .

   .weak      DMA1_Stream4_IRQHandler
   .thumb_func
   .type    DMA1_Stream4_IRQHandler, %function
DMA1_Stream4_IRQHandler: b .

   .weak      DMA1_Stream5_IRQHandler
   .thumb_func
   .type    DMA1_Stream5_IRQHandler, %function
DMA1_Stream5_IRQHandler: b .

   .weak      DMA1_Stream6_IRQHandler
   .thumb_func
   .type    DMA1_Stream6_IRQHandler, %function
DMA1_Stream6_IRQHandler: b .

   .weak      ADC_IRQHandler
   .thumb_func
   .type    ADC_IRQHandler, %function
ADC_IRQHandler: b .

   .weak      CAN1_TX_IRQHandler
   .thumb_func
   .type    CAN1_TX_IRQHandler, %function
CAN1_TX_IRQHandler: b .

   .weak      CAN1_RX0_IRQHandler
   .thumb_func
   .type    CAN1_RX0_IRQHandler, %function
CAN1_RX0_IRQHandler: b .

   .weak      CAN1_RX1_IRQHandler
   .thumb_func
   .type    CAN1_RX1_IRQHandler, %function
CAN1_RX1_IRQHandler: b .

   .weak      CAN1_SCE_IRQHandler
   .thumb_func
   .type    CAN1_SCE_IRQHandler, %function
CAN1_SCE_IRQHandler: b .

   .weak      EXTI9_5_IRQHandler
   .thumb_func
   .type    EXTI9_5_IRQHandler, %function
EXTI9_5_IRQHandler: b .

   .weak      TIM1_BRK_TIM9_IRQHandler
   .thumb_func
   .type    TIM1_BRK_TIM9_IRQHandler, %function
TIM1_BRK_TIM9_IRQHandler: b .

   .weak      TIM1_UP_TIM10_IRQHandler
   .thumb_func
   .type    TIM1_UP_TIM10_IRQHandler, %function
TIM1_UP_TIM10_IRQHandler: b .

   .weak      TIM1_TRG_COM_TIM11_IRQHandler
   .thumb_func
   .type    TIM1_TRG_COM_TIM11_IRQHandler, %function
TIM1_TRG_COM_TIM11_IRQHandler: b .

   .weak      TIM1_CC_IRQHandler
   .thumb_func
   .type    TIM1_CC_IRQHandler, %function
TIM1_CC_IRQHandler: b .

   .weak      TIM2_IRQHandler
   .thumb_func
   .type    TIM2_IRQHandler, %function
TIM2_IRQHandler: b .

   .weak      TIM3_IRQHandler
   .thumb_func
   .type    TIM3_IRQHandler, %function
TIM3_IRQHandler: b .

   .weak      TIM4_IRQHandler
   .thumb_func
   .type    TIM4_IRQHandler, %function
TIM4_IRQHandler: b .

   .weak      I2C1_EV_IRQHandler
   .thumb_func
   .type    I2C1_EV_IRQHandler, %function
I2C1_EV_IRQHandler: b .

   .weak      I2C1_ER_IRQHandler
   .thumb_func
   .type    I2C1_ER_IRQHandler, %function
I2C1_ER_IRQHandler: b .

   .weak      I2C2_EV_IRQHandler
   .thumb_func
   .type    I2C2_EV_IRQHandler, %function
I2C2_EV_IRQHandler: b .

   .weak      I2C2_ER_IRQHandler
   .thumb_func
   .type    I2C2_ER_IRQHandler, %function
I2C2_ER_IRQHandler: b .

   .weak      SPI1_IRQHandler
   .thumb_func
   .type    SPI1_IRQHandler, %function
SPI1_IRQHandler: b .

   .weak      SPI2_IRQHandler
   .thumb_func
   .type    SPI2_IRQHandler, %function
SPI2_IRQHandler: b .

   .weak      USART1_IRQHandler
   .thumb_func
   .type    USART1_IRQHandler, %function
USART1_IRQHandler: b .

   .weak      USART2_IRQHandler
   .thumb_func
   .type    USART2_IRQHandler, %function
USART2_IRQHandler: b .

   .weak      USART3_IRQHandler
   .thumb_func
   .type    USART3_IRQHandler, %function
USART3_IRQHandler: b .

   .weak      EXTI15_10_IRQHandler
   .thumb_func
   .type    EXTI15_10_IRQHandler, %function
EXTI15_10_IRQHandler: b .

   .weak      RTC_Alarm_IRQHandler
   .thumb_func
   .type    RTC_Alarm_IRQHandler, %function
RTC_Alarm_IRQHandler: b .

   .weak      OTG_FS_WKUP_IRQHandler
   .thumb_func
   .type    OTG_FS_WKUP_IRQHandler, %function
OTG_FS_WKUP_IRQHandler: b .

   .weak      TIM8_BRK_TIM12_IRQHandler
   .thumb_func
   .type    TIM8_BRK_TIM12_IRQHandler, %function
TIM8_BRK_TIM12_IRQHandler: b .

   .weak      TIM8_UP_TIM13_IRQHandler
   .thumb_func
   .type    TIM8_UP_TIM13_IRQHandler, %function
TIM8_UP_TIM13_IRQHandler: b .

   .weak      TIM8_TRG_COM_TIM14_IRQHandler
   .thumb_func
   .type    TIM8_TRG_COM_TIM14_IRQHandler, %function
TIM8_TRG_COM_TIM14_IRQHandler: b .

   .weak      TIM8_CC_IRQHandler
   .thumb_func
   .type    TIM8_CC_IRQHandler, %function
TIM8_CC_IRQHandler: b .

   .weak      DMA1_Stream7_IRQHandler
   .thumb_func
   .type    DMA1_Stream7_IRQHandler, %function
DMA1_Stream7_IRQHandler: b .

   .weak      FMC_IRQHandler
   .thumb_func
   .type    FMC_IRQHandler, %function
FMC_IRQHandler: b .

   .weak      SDMMC1_IRQHandler
   .thumb_func
   .type    SDMMC1_IRQHandler, %function
SDMMC1_IRQHandler: b .

   .weak      TIM5_IRQHandler
   .thumb_func
   .type    TIM5_IRQHandler, %function
TIM5_IRQHandler: b .

   .weak      SPI3_IRQHandler
   .thumb_func
   .type    SPI3_IRQHandler, %function
SPI3_IRQHandler: b .

   .weak      UART4_IRQHandler
   .thumb_func
   .type    UART4_IRQHandler, %function
UART4_IRQHandler: b .

   .weak      UART5_IRQHandler
   .thumb_func
   .type    UART5_IRQHandler, %function
UART5_IRQHandler: b .

   .weak      TIM6_DAC_IRQHandler
   .thumb_func
   .type    TIM6_DAC_IRQHandler, %function
TIM6_DAC_IRQHandler: b .

   .weak      TIM7_IRQHandler
   .thumb_func
   .type    TIM7_IRQHandler, %function
TIM7_IRQHandler: b .

   .weak      DMA2_Stream0_IRQHandler
   .thumb_func
   .type    DMA2_Stream0_IRQHandler, %function
DMA2_Stream0_IRQHandler: b .

   .weak      DMA2_Stream1_IRQHandler
   .thumb_func
   .type    DMA2_Stream1_IRQHandler, %function
DMA2_Stream1_IRQHandler: b .

   .weak      DMA2_Stream2_IRQHandler
   .thumb_func
   .type    DMA2_Stream2_IRQHandler, %function
DMA2_Stream2_IRQHandler: b .

   .weak      DMA2_Stream3_IRQHandler
   .thumb_func
   .type    DMA2_Stream3_IRQHandler, %function
DMA2_Stream3_IRQHandler: b .

   .weak      DMA2_Stream4_IRQHandler
   .thumb_func
   .type    DMA2_Stream4_IRQHandler, %function
DMA2_Stream4_IRQHandler: b .

   .weak      ETH_IRQHandler
   .thumb_func
   .type    ETH_IRQHandler, %function
ETH_IRQHandler: b .

   .weak      ETH_WKUP_IRQHandler
   .thumb_func
   .type    ETH_WKUP_IRQHandler, %function
ETH_WKUP_IRQHandler: b .

   .weak      CAN2_TX_IRQHandler
   .thumb_func
   .type    CAN2_TX_IRQHandler, %function
CAN2_TX_IRQHandler: b .

   .weak      CAN2_RX0_IRQHandler
   .thumb_func
   .type    CAN2_RX0_IRQHandler, %function
CAN2_RX0_IRQHandler: b .

   .weak      CAN2_RX1_IRQHandler
   .thumb_func
   .type    CAN2_RX1_IRQHandler, %function
CAN2_RX1_IRQHandler: b .

   .weak      CAN2_SCE_IRQHandler
   .thumb_func
   .type    CAN2_SCE_IRQHandler, %function
CAN2_SCE_IRQHandler: b .

   .weak      OTG_FS_IRQHandler
   .thumb_func
   .type    OTG_FS_IRQHandler, %function
OTG_FS_IRQHandler: b .

   .weak      DMA2_Stream5_IRQHandler
   .thumb_func
   .type    DMA2_Stream5_IRQHandler, %function
DMA2_Stream5_IRQHandler: b .

   .weak      DMA2_Stream6_IRQHandler
   .thumb_func
   .type    DMA2_Stream6_IRQHandler, %function
DMA2_Stream6_IRQHandler: b .

   .weak      DMA2_Stream7_IRQHandler
   .thumb_func
   .type    DMA2_Stream7_IRQHandler, %function
DMA2_Stream7_IRQHandler: b .

   .weak      USART6_IRQHandler
   .thumb_func
   .type    USART6_IRQHandler, %function
USART6_IRQHandler:    b   .

   .weak      I2C3_EV_IRQHandler
   .thumb_func
   .type    I2C3_EV_IRQHandler, %function
I2C3_EV_IRQHandler:    b .

   .weak      I2C3_ER_IRQHandler
   .thumb_func
   .type    I2C3_ER_IRQHandler, %function
I2C3_ER_IRQHandler:    b   .

   .weak      OTG_HS_EP1_OUT_IRQHandler
   .thumb_func
   .type    OTG_HS_EP1_OUT_IRQHandler, %function
OTG_HS_EP1_OUT_IRQHandler:    b   .

   .weak      OTG_HS_EP1_IN_IRQHandler
   .thumb_func
   .type    OTG_HS_EP1_IN_IRQHandler, %function
OTG_HS_EP1_IN_IRQHandler: b .

   .weak      OTG_HS_WKUP_IRQHandler
   .thumb_func
   .type    OTG_HS_WKUP_IRQHandler, %function
OTG_HS_WKUP_IRQHandler: b .

   .weak      OTG_HS_IRQHandler
   .thumb_func
   .type    OTG_HS_IRQHandler, %function
OTG_HS_IRQHandler: b .

   .weak      DCMI_IRQHandler
   .thumb_func
   .type    DCMI_IRQHandler, %function
DCMI_IRQHandler: b .

   .weak      RNG_IRQHandler
   .thumb_func
   .type    RNG_IRQHandler, %function
RNG_IRQHandler: b .

   .weak      FPU_IRQHandler
   .thumb_func
   .type    FPU_IRQHandler, %function
FPU_IRQHandler: b .

   .weak      UART7_IRQHandler
   .thumb_func
   .type    UART7_IRQHandler, %function
UART7_IRQHandler: b .

   .weak      UART8_IRQHandler
   .thumb_func
   .type    UART8_IRQHandler, %function
UART8_IRQHandler: b .

   .weak      SPI4_IRQHandler
   .thumb_func
   .type    SPI4_IRQHandler, %function
SPI4_IRQHandler: b .

   .weak      SPI5_IRQHandler
   .thumb_func
   .type    SPI5_IRQHandler, %function
SPI5_IRQHandler: b .

   .weak      SPI6_IRQHandler
   .thumb_func
   .type    SPI6_IRQHandler, %function
SPI6_IRQHandler: b .

   .weak      SAI1_IRQHandler
   .thumb_func
   .type    SAI1_IRQHandler, %function
SAI1_IRQHandler: b .

   .weak      LTDC_IRQHandler
   .thumb_func
   .type    LTDC_IRQHandler, %function
LTDC_IRQHandler: b .

   .weak      LTDC_ER_IRQHandler
   .thumb_func
   .type    LTDC_ER_IRQHandler, %function
LTDC_ER_IRQHandler: b .

   .weak      DMA2D_IRQHandler
   .thumb_func
   .type    DMA2D_IRQHandler, %function
DMA2D_IRQHandler: b .

   .weak      SAI2_IRQHandler
   .thumb_func
   .type    SAI2_IRQHandler, %function
SAI2_IRQHandler: b .

   .weak      QUADSPI_IRQHandler
   .thumb_func
   .type    QUADSPI_IRQHandler, %function
QUADSPI_IRQHandler: b .

   .weak      LPTIM1_IRQHandler
   .thumb_func
   .type    LPTIM1_IRQHandler, %function
LPTIM1_IRQHandler: b .

   .weak      CEC_IRQHandler
   .thumb_func
   .type    CEC_IRQHandler, %function
CEC_IRQHandler: b .

   .weak      I2C4_EV_IRQHandler
   .thumb_func
   .type    I2C4_EV_IRQHandler, %function
I2C4_EV_IRQHandler: b .

   .weak      I2C4_ER_IRQHandler
   .thumb_func
   .type    I2C4_ER_IRQHandler, %function
I2C4_ER_IRQHandler: b .

   .weak      SPDIF_RX_IRQHandler
   .thumb_func
   .type    SPDIF_RX_IRQHandler, %function
SPDIF_RX_IRQHandler: b .

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

