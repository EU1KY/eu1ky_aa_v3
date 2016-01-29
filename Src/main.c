#include "main.h"
#include "stm32f7xx_hal_uart.h"
#include <stdio.h>
#include <string.h>
#include "arm_math.h"
#include <complex.h>
#include <math.h>

static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);

void Sleep(uint32_t nms)
{
    uint32_t tstart = HAL_GetTick();
    while ((HAL_GetTick() - tstart) < nms);
}

#define NSAMPLES 2048
#define NDUMMY 64

static volatile int audioReady = 0;
static int16_t audioBuf[(NSAMPLES + NDUMMY) * 2];
void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
    audioReady = 1;
    BSP_AUDIO_IN_Pause();
}

UART_HandleTypeDef UartHandle = {0};


#define FSAMPLE I2S_AUDIOFREQ_16K

static float rfft_input[NSAMPLES];
static float rfft_output[NSAMPLES];
static float rfft_mags[NSAMPLES/2];
float complex *prfft   = (float complex*)rfft_output;
static const float binwidth = ((float)(FSAMPLE)) / (NSAMPLES);
static float wnd[NSAMPLES];

static void fillwnd(void)
{
    int32_t i;
    for(i = 0; i < NSAMPLES; i++)
    {
        //Blackman window, >66 dB OOB rejection
        wnd[i]= 0.426591f - .496561f * cosf( (2 * M_PI * i) / 1024) + .076848f * cosf((4 * M_PI * i) / 1024);
    }
}
static void do_fft_audiobuf()
{
    int i;
    arm_rfft_fast_instance_f32 S;

    for(i = 0; i < NSAMPLES; i++)
    {
        rfft_input[i] = (float)audioBuf[(i + NDUMMY) * 2];
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


int main(void)
{
    RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

    /* Enable the CPU Cache */
    CPU_CACHE_Enable();

    /* STM32F7xx HAL library initialization:
         - Configure the Flash ART accelerator on ITCM interface
         - Systick timer is configured by default as source of time base, but user
           can eventually implement his proper time base source (a general purpose
           timer for example or other time source), keeping in mind that Time base
           duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
           handled in milliseconds basis.
         - Set NVIC Group Priority to 4
         - Low Level Initialization
       */
    HAL_Init();

    /* Configure the system clock to 216 MHz */
    SystemClock_Config();

    /* LCD clock configuration */
    /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
    /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 Mhz */
    /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6Mhz */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_4;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    memset(&UartHandle, 0, sizeof(UartHandle));
    UartHandle.Instance        = USART1;
    UartHandle.Init.BaudRate   = 9600;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = UART_STOPBITS_1;
    UartHandle.Init.Parity     = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;
    UartHandle.Init.OverSampling = UART_OVERSAMPLING_8;
    UartHandle.Init.OneBitSampling = UART_ONEBIT_SAMPLING_ENABLED;
    HAL_UART_Init(&UartHandle);

    /* Configure LED1 */
    BSP_LED_Init(LED1);

    /* Configure LCD : Only one layer is used */
    BSP_LCD_Init();

    //Touchscreen
    BSP_TS_Init(FT5336_MAX_WIDTH, FT5336_MAX_HEIGHT);

    /* LCD Initialization */
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));

    /* Enable the LCD */
    BSP_LCD_DisplayOn();

    /* Select the LCD Background Layer  */
    BSP_LCD_SelectLayer(0);

    /* Clear the Background Layer */
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    /* Select the LCD Foreground Layer  */
    BSP_LCD_SelectLayer(1);

    /* Clear the Foreground Layer */
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    /* Configure the transparency for foreground and background :
     Increase the transparency */
    BSP_LCD_SetTransparency(0, 255);
    BSP_LCD_SetTransparency(1, 240);

    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_DisplayStringAt(0, 0, (uint8_t*)"STM32F746G Discovery", CENTER_MODE);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_DisplayStringAt(0, 40, (uint8_t*)"sample project", CENTER_MODE);

    BSP_LCD_SelectLayer(1);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetFont(&Font16);

    uint8_t ret;
    ret = BSP_AUDIO_IN_Init(INPUT_DEVICE_INPUT_LINE_1, 100, FSAMPLE);
    if (ret != AUDIO_OK)
    {
        BSP_LCD_SetTextColor(LCD_COLOR_RED);
        BSP_LCD_DisplayStringAtLine(2, "BSP_AUDIO_IN_Init failed");
    }
    audioReady = 2;
    BSP_AUDIO_IN_Record(audioBuf, (NSAMPLES + NDUMMY) * 2);

    uint32_t ctr = 0;
    char buf[100];
    int paused = 0;
    TS_StateTypeDef ts;

    fillwnd();

    while (1)
    {
        (HAL_GetTick() & 0x100 ? BSP_LED_On : BSP_LED_Off)(LED1);
        BSP_TS_GetState(&ts);
        if (ts.touchDetected)
        {
            sprintf(buf, "x %d, y %d, wt %d", ts.touchX[0], ts.touchY[0], ts.touchWeight[0]);
            BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGREEN);
            BSP_LCD_ClearStringLine(3);
            BSP_LCD_DisplayStringAtLine(3, buf);
            continue;
        }
        else
        {
            BSP_LCD_ClearStringLine(3);
        }

        if (audioReady == 0)
        {
            BSP_AUDIO_IN_Resume();
        }
        else if (audioReady == 1)
        {
            uint32_t tstart = HAL_GetTick();
            do_fft_audiobuf();
            tstart = HAL_GetTick() - tstart;

            //Draw oscilloscope
            BSP_LCD_SetTextColor(0xFF000020);
            BSP_LCD_FillRect(0, 140, 480, 140);
            BSP_LCD_SetTextColor(0xFF000040);
            int i;
            for (i = 279; i > 140; i-=10)
            {
                BSP_LCD_DrawLine(0, i, 479, i);
            }

            float maxmag = 0.;
            int idxmax = -1;
            for (i = 0; i < NSAMPLES/2; i++)
            {
                if (rfft_mags[i] > maxmag)
                {
                    maxmag = rfft_mags[i];
                    idxmax = i;
                }
            }
            sprintf(buf,"RFFT: %d ms Mag %.0f @ %.0f Hz", tstart, maxmag, binwidth * idxmax);
            BSP_LCD_SetTextColor(LCD_COLOR_RED);
            BSP_LCD_ClearStringLine(4);
            BSP_LCD_DisplayStringAtLine(4, buf);

            for (int x = 0; x < 480; x++)
            {
                int y = (int)(279 - 20 * log10f(rfft_mags[x]));
                BSP_LCD_DrawLine(x, 279, x, y);
            }
/*
            uint16_t lasty_left = 210;
            uint16_t lasty_right = 210;
            for (i = 32; i < 1024; i += 2)
            {
                if (i == 0)
                {
                    lasty_left = 210 - audioBuf[(i + 32) * 2]/ 500;
                    BSP_LCD_DrawPixel(i/2, lasty_left, LCD_COLOR_GREEN);
                    lasty_right = 210 - audioBuf[(i + 32) * 2 + 1 ]/ 500;
                    BSP_LCD_DrawPixel(i/2, lasty_right, LCD_COLOR_RED);
                }
                else
                {
                    uint16_t y_left = 210 - audioBuf[(i + 32) * 2]/ 500;
                    uint16_t y_right = 210 - audioBuf[(i + 32) * 2 + 1]/ 500;
                    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
                    BSP_LCD_DrawLine(i/2-1, lasty_left, i/2, y_left);
                    BSP_LCD_SetTextColor(LCD_COLOR_RED);
                    BSP_LCD_DrawLine(i/2-1, lasty_right, i/2, y_right);
                    lasty_left = y_left;
                    lasty_right = y_right;
                }
            }
*/
            HAL_Delay(20);
            audioReady = 0;

            HAL_UART_Transmit(&UartHandle, "ABCDEF", 6, 5);
        }
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

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif
