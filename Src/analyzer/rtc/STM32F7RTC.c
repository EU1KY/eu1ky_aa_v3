
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "stm32f7xx_hal_rtc.h"
#include "stm32f7xx_hal_rtc_ex.h"
#include "STM32F7RTC.h"
// Initialize RTC, setup interrupt
static RTC_HandleTypeDef rtc1;
//extern HAL_StatusTypeDef HAL_RTC_Init(RTC_HandleTypeDef *hrtc);

void RTCInit(void) {
HAL_StatusTypeDef ret = HAL_OK;
    rtc1.Instance=RTC;
    rtc1.Init.HourFormat=RTC_HOURFORMAT_24;
    rtc1.Init.AsynchPrediv=255;
    rtc1.Init.SynchPrediv=127;
    rtc1.Init.OutPut=RTC_OUTPUT_WAKEUP;
    rtc1.Init.OutPutPolarity=0;
    rtc1.Init.OutPutType=0;
   // ret = HAL_RTC_Init(& rtc1);
   // HAL_RTCEx_SetWakeUpTimer_IT(&rtc1, 20480, RTC_WAKEUPCLOCK_RTCCLK_DIV16 );

}


 void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *rtc1){
    if (Sleeping==1){
        Sleeping=0;
        secondsCounter++;
        if(secondsCounter>=10){
            secondsCounter=0;
            BSP_LCD_DisplayOn();
            Sleep(1000);
            //Beep(1);
            BSP_LCD_DisplayOff();
        }
    }

 }

