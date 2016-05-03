#include "main.h"
#include "stm32f7xx_hal_uart.h"
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "font.h"
#include "touch.h"
#include "panvswr2.h"
#include "measurement.h"
#include "fftwnd.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "config.h"
#include "crash.h"
#include "gen.h"
#include "si5351.h"

static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);

void Sleep(uint32_t nms)
{
    HAL_Delay(nms);
}

#define NSAMPLES 512
#define NDUMMY 32
#define FSAMPLE I2S_AUDIOFREQ_48K

static int step = 1;
static UART_HandleTypeDef UartHandle = {0};

void uart_init(void)
{
    memset(&UartHandle, 0, sizeof(UartHandle));
    UartHandle.Instance        = USART1;
    UartHandle.Init.BaudRate   = 9600;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = UART_STOPBITS_1;
    UartHandle.Init.Parity     = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;
    UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
    //UartHandle.Init.OneBitSampling = UART_ONEBIT_SAMPLING_ENABLED;
    HAL_UART_Init(&UartHandle);
}

void flash_example()
{
    BSP_QSPI_Init();

    QSPI_Info inf;
    BSP_QSPI_GetInfo(&inf);
    FONT_SetAttributes(FONT_FRAN, LCD_YELLOW, LCD_BLACK);
    FONT_Printf(0, 0, "FlashSize: %d", inf.FlashSize);
    FONT_Printf(0, 20, "EraseSectorSize: %d", inf.EraseSectorSize);
    FONT_Printf(0, 40, "EraseSectorsNumber: %d", inf.EraseSectorsNumber);
    FONT_Printf(0, 60, "ProgPageSize: %d", inf.ProgPageSize);
    FONT_Printf(0, 80, "ProgPagesNumber: %d", inf.ProgPagesNumber);

    uint8_t rdbuf[256] = {0};
    //BSP_QSPI_Erase_Chip();
    //BSP_QSPI_Erase_Block(0);

    //rdbuf[0] = 0xFF;
    //BSP_QSPI_Write(rdbuf, 100000, 1);

    BSP_QSPI_Read(rdbuf, 0, 256);
    FONT_SetAttributes(FONT_FRAN, LCD_YELLOW, LCD_BLACK);
    uint8_t* p = rdbuf;
    for (uint32_t i=0; i < 10; i++)
    {
        FONT_Printf(0, 100 + i * 14, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                    p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                    p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
        p += 16;
    }
}

static FATFS SDFatFs;  // File system object for SD card logical drive
static char SDPath[4];        // SD card logical drive path

int main(void)
{
    uint32_t oscilloscope = 1;
    CPU_CACHE_Enable();
    HAL_Init();
    SystemClock_Config();
    uart_init();
    BSP_LED_Init(LED1);
    LCD_Init();
    TOUCH_Init();

    HAL_Delay(500);
    if (FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
        CRASH("FATFS_LinkDriver failed");
    if (f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
        CRASH("f_mount failed");
    CFG_Init();

    CFG_ParamWnd();

    si5351_init();

    uint8_t ret;
    ret = BSP_AUDIO_IN_Init(INPUT_DEVICE_INPUT_LINE_1, 100 - CFG_GetParam(CFG_PARAM_LIN_ATTENUATION), FSAMPLE);
    if (ret != AUDIO_OK)
    {
        CRASH("BSP_AUDIO_IN_Init failed");
    }

    for(;;)
    {
        PANVSWR2_Proc();
        MEASUREMENT_Proc();
        FFTWND_Proc();
    }

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
    SCB_EnableICache();

    /* Enable D-Cache */
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
