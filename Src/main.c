#include "main.h"
#include "stm32f7xx_hal_uart.h"
#include <stdio.h>
#include <string.h>
#include "arm_math.h"
#include <complex.h>
#include <math.h>
#include "lcd.h"
#include "touch.h"
#include "panvswr2.h"

static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);

void Sleep(uint32_t nms)
{
    HAL_Delay(nms);
}

#define NSAMPLES 2048
#define NDUMMY 2048
#define FSAMPLE I2S_AUDIOFREQ_48K

static volatile int audioReady = 0;
static int16_t audioBuf[(NSAMPLES + NDUMMY) * 2] __attribute__((aligned(0x100))) = {0};
void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
    audioReady = 1;
    //BSP_AUDIO_IN_Pause();
}

UART_HandleTypeDef UartHandle = {0};


static float rfft_input[NSAMPLES];
static float rfft_output[NSAMPLES];
static float rfft_mags[NSAMPLES/2];
float complex *prfft   = (float complex*)rfft_output;
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
                      -0.388*cosf((6 * M_PI * i) / ns) +0.0322*cosf((8 * M_PI * i) / ns)) / 4.;
            break;
        }
    }
}
static void do_fft_audiobuf()
{
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
    audioReady = 0;
    BSP_AUDIO_IN_Record(audioBuf, (NSAMPLES + NDUMMY) * 2);
}

int main(void)
{
    uint32_t oscilloscope = 1;
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

    /* Configure LED1 */
    BSP_LED_Init(LED1);

    //LCD
    LCD_Init();

    //Touchscreen
    TOUCH_Init();

    //for(;;)
    {
        //    PANVSWR2_Proc();
    }

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
    //audioReady = 2;
    //BSP_AUDIO_IN_Record(audioBuf, (NSAMPLES + NDUMMY) * 2);

    uint32_t ctr = 0;
    char buf[100];
    int paused = 0;
    TS_StateTypeDef ts;
    int i;

    prepare_windowing();

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
            if (ts.touchY[0] > 140)
                oscilloscope = !oscilloscope;
            else
            {
                wndtype++;
                prepare_windowing();
                sprintf(buf, "Windowing: %s", wndstr[wndtype]);
                BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGREEN);
                BSP_LCD_ClearStringLine(2);
                BSP_LCD_DisplayStringAtLine(2, buf);
            }
            while(TOUCH_IsPressed());
            continue;
        }
        else
        {
            BSP_LCD_ClearStringLine(3);
        }

        measure();
        while (0 == audioReady);
        if (oscilloscope)
        {
            uint16_t lasty_left = 210;
            uint16_t lasty_right = 210;
            int16_t* pData = &audioBuf[NDUMMY];
            BSP_LCD_SetTextColor(0xFF000020);
            BSP_LCD_FillRect(0, 140, 480, 140);

            for (i = 0; i < NSAMPLES; i ++)
            {
                if (i == 0)
                {
                    lasty_left = 210 - (int)(*pData++ * wnd[i*4]) / 500;
                    if (lasty_left > 279)
                        lasty_left = 279;
                    if (lasty_left < 0)
                        lasty_left = 0;
                    BSP_LCD_DrawPixel(i, lasty_left, LCD_COLOR_GREEN);
                    lasty_right = 210 - (int)(*pData++ * wnd[i*4]) / 500;
                    if (lasty_right > 279)
                        lasty_right = 279;
                    if (lasty_right < 140)
                        lasty_right = 140;
                    BSP_LCD_DrawPixel(i, lasty_right, LCD_COLOR_RED);
                }
                else
                {
                    uint16_t y_left = 210 - (int)(*pData++ * wnd[i*4]) / 500;
                    uint16_t y_right = 210 - (int)(*pData++ * wnd[i*4]) / 500;
                    if (y_left > 279)
                        y_left = 279;
                    if (y_left < 140)
                        y_left = 140;
                    if (y_right > 279)
                        y_right = 279;
                    if (y_right < 140)
                        y_right = 140;
                    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
                    BSP_LCD_DrawLine(i-1, lasty_left, i, y_left);
                    BSP_LCD_SetTextColor(LCD_COLOR_RED);
                    BSP_LCD_DrawLine(i-1, lasty_right, i, y_right);
                    lasty_left = y_left;
                    lasty_right = y_right;
                }
                if (NSAMPLES >= 2048)
                    pData += 6;
                else if (NSAMPLES == 1024)
                    pData += 2;
                if (i >= 479)
                    break;
            }
        }
        else //Spectrum
        {
            uint32_t tstart = HAL_GetTick();
            do_fft_audiobuf();
            tstart = HAL_GetTick() - tstart;

            //Draw spectrum
            BSP_LCD_SetTextColor(0xFF000020);
            BSP_LCD_FillRect(0, 140, 480, 140);
            BSP_LCD_SetTextColor(0xFF000040);
            //Draw horizontal grid lines
            for (i = 279; i > 140; i-=10)
            {
                BSP_LCD_DrawLine(0, i, 479, i);
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

            sprintf(buf,"RFFT: %d ms Mag %.0f @ %.0f Hz", tstart, maxmag, binwidth * idxmax);
            BSP_LCD_SetTextColor(LCD_COLOR_RED);
            BSP_LCD_ClearStringLine(4);
            BSP_LCD_DisplayStringAtLine(4, buf);

            //Draw spectrum
            for (int x = 0; x < NSAMPLES/2; x++)
            {
                int y = (int)(279 - 20 * log10f(rfft_mags[x]));
                if (y <= 279)
                {
                    BSP_LCD_DrawLine(x, 279, x, y);
                }
                if (x >= 479)
                    break;
            }
        }//spectrum

        HAL_Delay(50);
        audioReady = 0;

        //HAL_UART_Transmit(&UartHandle, "ABCDEF", 6, 5);
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
