#include "main.h"
#include "stm32f7xx_hal_uart.h"
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "font.h"
#include "touch.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "config.h"
#include "crash.h"
#include "si5351.h"
#include "dsp.h"
#include "mainwnd.h"

static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);

void Sleep(uint32_t nms)
{
    HAL_Delay(nms);
}

static int step = 1;
static UART_HandleTypeDef UartHandle = {0};

void uart_init(void)
{
    UartHandle.Init.BaudRate = 921600;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits = UART_STOPBITS_1;
    UartHandle.Init.Parity = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode = UART_MODE_TX_RX;

    BSP_COM_Init(COM1, &UartHandle);
}

_ssize_t _write_r (struct _reent *r, int file, const void *ptr, size_t len)
{
    HAL_UART_Transmit(&UartHandle, (uint8_t*)ptr, len, 0xFFFFFFFF);
    return len;
}


static FATFS SDFatFs;  // File system object for SD card logical drive
static char SDPath[4];        // SD card logical drive path

int main(void)
{
    CPU_CACHE_Enable();
    HAL_Init();
    SystemClock_Config();
    uart_init();
    BSP_LED_Init(LED1);
    LCD_Init();
    TOUCH_Init();

    HAL_Delay(300);

    //Mount SD card
    if (FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
        CRASH("FATFS_LinkDriver failed");
    if (f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
        CRASH("f_mount failed");

    CFG_Init(); //Load configuration

    si5351_init(); //Initialize frequency synthesizer

    DSP_Init(); //Initialize DSP module. Also loads calibration files inside.

    #if 0
    int i = 0;
    for(;;)
    {
        uint8_t rxb[4];
        if (HAL_OK == HAL_UART_Receive(&UartHandle, rxb, 1, 100))
        {
            printf("%02X\r\n", rxb[0]);
        }
        else
        {
            printf("%f\r\n", ((float)i++)/ 100.f);
        }
    }
    #endif

    //Run main window function
    MainWnd(); //Never returns

    return 0;
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 432;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 9;

    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if(ret != HAL_OK)
    {
        while(1)
        {
            ;
        }
    }

    /* Activate the OverDrive to reach the 216 MHz Frequency */
    ret = HAL_PWREx_EnableOverDrive();
    if(ret != HAL_OK)
    {
        while(1)
        {
            ;
        }
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
    if(ret != HAL_OK)
    {
        while(1)
        {
            ;
        }
    }
}


//CPU L1-Cache enable.
static void CPU_CACHE_Enable(void)
{
    /* Enable I-Cache */
    SCB_InvalidateICache();
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_InvalidateDCache();
    SCB_EnableDCache();
}

void CRASH(const char *text)
{
    FONT_Write(FONT_FRAN, LCD_RED, LCD_BLACK, 0, 0, text);
    FONT_Write(FONT_FRAN, LCD_RED, LCD_BLACK, 0, 14, "SYSTEM HALTED ");
    for(;;);
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
    static uint32_t in_assert = 0;
    if (in_assert) return;
    in_assert = 1;
    CRASHF("ASSERT @ %s:%d", file, line);
}
#endif
