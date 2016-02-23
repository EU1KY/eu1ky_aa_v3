#include "main.h"
#include "stm32f7xx_hal_uart.h"
#include <stdio.h>
#include <string.h>
#include "arm_math.h"
#include <complex.h>
#include <math.h>
#include "lcd.h"
#include "font.h"
#include "touch.h"
#include "panvswr2.h"
#include "measurement.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "config.h"
#include "crash.h"

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
static int16_t audioBuf[(NSAMPLES + NDUMMY) * 2] = {0};
static UART_HandleTypeDef UartHandle = {0};
static float rfft_input[NSAMPLES];
static float rfft_output[NSAMPLES];
static float rfft_mags[NSAMPLES/2];
static const float complex *prfft   = (float complex*)rfft_output;
static const float binwidth = ((float)(FSAMPLE)) / (NSAMPLES);
static float wnd[NSAMPLES];
static enum {W_NONE, W_SINUS, W_HANN, W_HAMMING, W_BLACKMAN, W_FLATTOP} wndtype = W_NONE;
static const char* wndstr[] = {"None", "Sinus", "Hann", "Hamming", "Blackman", "Flattop"};

static void prepare_windowing(void)
{
    int32_t i;
    int ns = NSAMPLES - 1;
    for(i = 0; i < NSAMPLES; i++)
    {
        switch (wndtype)
        {
        case W_NONE:
        default:
            wnd[i] = 1.0f;
            wndtype = W_NONE;
            break;
        case W_SINUS:
            wnd[i] = sinf((M_PI * i)/ns);
            break;
        case W_HANN:
            wnd[i] = 0.5 - 0.5 * cosf((2 * M_PI * i)/ns);
            break;
        case W_HAMMING:
            wnd[i] = 0.54 - 0.46 * cosf((2 * M_PI * i)/ns);
            break;
        case W_BLACKMAN:
            //Blackman window, >66 dB OOB rejection
            wnd[i] = 0.426591f - .496561f * cosf( (2 * M_PI * i) / ns) + .076848f * cosf((4 * M_PI * i) / ns);
            break;
        case W_FLATTOP:
            //Flattop window
            wnd[i] = (1.0 - 1.93*cosf((2 * M_PI * i) / ns) + 1.29 * cosf((4 * M_PI * i) / ns)
                      -0.388*cosf((6 * M_PI * i) / ns) +0.0322*cosf((8 * M_PI * i) / ns)) / 3.f;
            break;
        }
    }
}
static void do_fft_audiobuf()
{
    //Only one channel
    int i;
    arm_rfft_fast_instance_f32 S;

    int16_t* pBuf = &audioBuf[NDUMMY];
    for(i = 0; i < NSAMPLES; i++)
    {
        rfft_input[i] = (float)*pBuf;
        pBuf += 2;
        rfft_input[i] *= wnd[i];
    }

    arm_rfft_fast_init_f32(&S, NSAMPLES);
    arm_rfft_fast_f32(&S, rfft_input, rfft_output, 0);

    for (i = 0; i < NSAMPLES/2; i++)
    {
        float complex binf = prfft[i];
        rfft_mags[i] = cabsf(binf) / (NSAMPLES/2);
    }
}

void measure(void)
{
    extern SAI_HandleTypeDef haudio_in_sai;
    HAL_SAI_Receive(&haudio_in_sai, (uint8_t*)audioBuf, (NSAMPLES + NDUMMY) * 2, HAL_MAX_DELAY);
}
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

    for(;;)
    {
        PANVSWR2_Proc();
        MEASUREMENT_Proc();
    }

    uint8_t ret;
    ret = BSP_AUDIO_IN_Init(INPUT_DEVICE_INPUT_LINE_1, 100, FSAMPLE);
    if (ret != AUDIO_OK)
    {
        CRASH("BSP_AUDIO_IN_Init failed");
    }

    uint32_t ctr = 0;
    int i;

    prepare_windowing();

    while (1)
    {
        (HAL_GetTick() & 0x100 ? BSP_LED_On : BSP_LED_Off)(LED1);
        LCDPoint pt;
        if (TOUCH_Poll(&pt))
        {
            FONT_SetAttributes(FONT_FRANBIG, LCD_GREEN, LCD_BLACK);
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 0);
            FONT_Printf(0, 0, "x %d, y %d", pt.x, pt.y);
            if (pt.y > 140)
                oscilloscope = !oscilloscope;
            else
            {
                wndtype++;
                prepare_windowing();
                FONT_SetAttributes(FONT_FRANBIG, LCD_WHITE, LCD_BLACK);
                FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 32);
                FONT_Printf(0, 32, "Windowing: %s", wndstr[wndtype]);
            }
            while(TOUCH_IsPressed());
            continue;
        }
        else
        {
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 0);
        }

        uint32_t tmstart = HAL_GetTick();
        measure();
        tmstart = HAL_GetTick() - tmstart;

        if (oscilloscope)
        {
            uint16_t lasty_left = 210;
            uint16_t lasty_right = 210;
            int16_t* pData;
            int16_t minMag = 32767;
            int16_t maxMag = -32767;
            int32_t magnitude = 0;

            LCD_FillRect(LCD_MakePoint(0, 140), LCD_MakePoint(LCD_GetWidth()-1, LCD_GetHeight()-1), 0xFF000020);

            //Calc magnitude for one channel only
            for (i = NDUMMY * 2; i < (NSAMPLES + NDUMMY) * 2; i += 2)
            {
                if (minMag > audioBuf[i])
                    minMag = audioBuf[i];
                if (maxMag < audioBuf[i])
                    maxMag = audioBuf[i];
            }
            magnitude = (maxMag - minMag) / 2;

            FONT_SetAttributes(FONT_FRANBIG, LCD_BLUE, LCD_BLACK);
            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 64);
            FONT_Printf(0, 64, "Sampling %d ms, Magnitude: %d", tmstart, magnitude);

            pData = &audioBuf[NDUMMY];
            for (i = 0; i < NSAMPLES; i ++)
            {
                if (i == 0)
                {
                    lasty_left = 210 - (int)(*pData++ * wnd[i*step]) / 500;
                    if (lasty_left > LCD_GetHeight()-1)
                        lasty_left = LCD_GetHeight()-1;
                    if (lasty_left < 0)
                        lasty_left = 0;
                    LCD_SetPixel(LCD_MakePoint(i, lasty_left), LCD_COLOR_GREEN);
                    lasty_right = 210 - (int)(*pData++ * wnd[i*step]) / 500;
                    if (lasty_right > LCD_GetHeight()-1)
                        lasty_right = LCD_GetHeight()-1;
                    if (lasty_right < 140)
                        lasty_right = 140;
                    LCD_SetPixel(LCD_MakePoint(i, lasty_right), LCD_COLOR_RED);
                }
                else
                {
                    uint16_t y_left = 210 - (int)(*pData++ * wnd[i*step]) / 500;
                    uint16_t y_right = 210 - (int)(*pData++ * wnd[i*step]) / 500;
                    if (y_left > LCD_GetHeight()-1)
                        y_left = LCD_GetHeight()-1;
                    if (y_left < 140)
                        y_left = 140;
                    if (y_right > LCD_GetHeight()-1)
                        y_right = LCD_GetHeight()-1;
                    if (y_right < 140)
                        y_right = 140;
                    LCD_Line(LCD_MakePoint(i-1, lasty_left), LCD_MakePoint(i, y_left), LCD_RED);
                    LCD_Line(LCD_MakePoint(i-1, lasty_right), LCD_MakePoint(i, y_right), LCD_GREEN);
                    lasty_left = y_left;
                    lasty_right = y_right;
                }
                pData += (step * 2 - 2);
                if (i >= LCD_GetWidth()-1)
                    break;
            }
        }
        else //Spectrum
        {
            uint32_t tstart = HAL_GetTick();
            do_fft_audiobuf();
            tstart = HAL_GetTick() - tstart;

            //Draw spectrum
            LCD_FillRect(LCD_MakePoint(0, 140), LCD_MakePoint(LCD_GetWidth()-1, LCD_GetHeight()-1), 0xFF000020);

            //Draw horizontal grid lines
            for (i = LCD_GetHeight()-1; i > 140; i-=10)
            {
                LCD_Line(LCD_MakePoint(0, i), LCD_MakePoint(LCD_GetWidth()-1, i), 0xFF303070);
            }
            //Calculate max magnitude bin
            float maxmag = 0.;
            int idxmax = -1;
            for (i = 3; i < NSAMPLES/2 - 3; i++)
            {
                if (rfft_mags[i] > maxmag)
                {
                    maxmag = rfft_mags[i];
                    idxmax = i;
                }
            }

            //Calculate magnitude value considering +/-3 bins from maximum

            float P = 0.f;
            for (i = idxmax-1; i <= idxmax + 1; i++)
                P += powf(rfft_mags[i], 2);
            maxmag = sqrtf(P);

            FONT_ClearLine(FONT_FRANBIG, LCD_BLACK, 64);
            FONT_SetAttributes(FONT_FRANBIG, LCD_BLUE, LCD_BLACK);
            FONT_Printf(0, 64, "RFFT: %d ms Mag %.0f @ %.0f Hz", tstart, maxmag, binwidth * idxmax);

            //Draw spectrum
            for (int x = 0; x < NSAMPLES/2; x++)
            {
                int y = (int)(LCD_GetHeight()-1 - 20 * log10f(rfft_mags[x]));
                if (y <= LCD_GetHeight()-1)
                {
                    LCD_Line(LCD_MakePoint(x, LCD_GetHeight()-1), LCD_MakePoint(x, y), LCD_RED);
                }
                if (x >= LCD_GetWidth()-1)
                    break;
            }
        }
        HAL_Delay(100);
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
    char txt[256];
    sprintf(txt, "ASSERT @ %s:%d", file, line);
    CRASH(txt);
}
#endif
