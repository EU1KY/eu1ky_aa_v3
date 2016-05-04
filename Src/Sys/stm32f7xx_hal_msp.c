/**
  ******************************************************************************
  * @file    LTDC/LTDC_Display_1Layer/Src/stm32f7xx_hal_msp.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    25-June-2015
  * @brief   HAL MSP module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32F7xx_HAL_Examples
  * @{
  */

/** @defgroup LTDC_Display_1Layer
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup HAL_MSP_Private_Functions
  * @{
  */

/**
  * @brief LTDC MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param hltdc: LTDC handle pointer
  * @retval None
  */
void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
//  GPIO_InitTypeDef GPIO_Init_Structure;
//
//  /*##-1- Enable peripherals and GPIO Clocks #################################*/
//  /*##-1- Enable peripherals and GPIO Clocks #################################*/
//  /* Enable the LTDC Clock */
//  __HAL_RCC_LTDC_CLK_ENABLE();
//
//  /*##-2- Configure peripheral GPIO ##########################################*/
//  /******************** LTDC Pins configuration *************************/
//  /* Enable GPIOs clock */
//  __HAL_RCC_GPIOE_CLK_ENABLE();
//  __HAL_RCC_GPIOG_CLK_ENABLE();
//  __HAL_RCC_GPIOI_CLK_ENABLE();
//  __HAL_RCC_GPIOJ_CLK_ENABLE();
//  __HAL_RCC_GPIOK_CLK_ENABLE();
//
//  /*** LTDC Pins configuration ***/
//  /* GPIOE configuration */
//  GPIO_Init_Structure.Pin       = GPIO_PIN_4;
//  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
//  GPIO_Init_Structure.Pull      = GPIO_NOPULL;
//  GPIO_Init_Structure.Speed     = GPIO_SPEED_FAST;
//  GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;
//  HAL_GPIO_Init(GPIOE, &GPIO_Init_Structure);
//
//  /* GPIOG configuration */
//  GPIO_Init_Structure.Pin       = GPIO_PIN_12;
//  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
//  GPIO_Init_Structure.Alternate = GPIO_AF9_LTDC;
//  HAL_GPIO_Init(GPIOG, &GPIO_Init_Structure);
//
//  /* GPIOI LTDC alternate configuration */
//  GPIO_Init_Structure.Pin       = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
//  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
//  GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;
//  HAL_GPIO_Init(GPIOI, &GPIO_Init_Structure);
//
//  /* GPIOJ configuration */
//  GPIO_Init_Structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
//  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
//  GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;
//  HAL_GPIO_Init(GPIOJ, &GPIO_Init_Structure);
//
//  /* GPIOK configuration */
//  GPIO_Init_Structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
//  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
//  GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;
//  HAL_GPIO_Init(GPIOK, &GPIO_Init_Structure);
//
//  /* LCD_DISP GPIO configuration */
//  GPIO_Init_Structure.Pin       = GPIO_PIN_12;     /* LCD_DISP pin has to be manually controlled */
//  GPIO_Init_Structure.Mode      = GPIO_MODE_OUTPUT_PP;
//  HAL_GPIO_Init(GPIOI, &GPIO_Init_Structure);
//
//  /* LCD_BL_CTRL GPIO configuration */
//  GPIO_Init_Structure.Pin       = GPIO_PIN_3;  /* LCD_BL_CTRL pin has to be manually controlled */
//  GPIO_Init_Structure.Mode      = GPIO_MODE_OUTPUT_PP;
//  HAL_GPIO_Init(GPIOK, &GPIO_Init_Structure);
//
//  /* Assert display enable LCD_DISP pin */
//  HAL_GPIO_WritePin(LCD_DISP_GPIO_PORT, LCD_DISP_PIN, GPIO_PIN_SET);
//
//  /* Assert backlight LCD_BL_CTRL pin */
//  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
//
}

/**
  * @brief LTDC MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  * @param hltdc: LTDC handle pointer
  * @retval None
  */
void HAL_LTDC_MspDeInit(LTDC_HandleTypeDef *hltdc)
{

  /*##-1- Reset peripherals ##################################################*/
  /* Enable LTDC reset state */
  __HAL_RCC_LTDC_FORCE_RESET();

  /* Release LTDC from reset state */
  __HAL_RCC_LTDC_RELEASE_RESET();
}

void _HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO TX/RX clock */
    USARTx_TX_GPIO_CLK_ENABLE();
    USARTx_RX_GPIO_CLK_ENABLE();


    /* Enable USARTx clock */
    USARTx_CLK_ENABLE();

    /*##-2- Configure peripheral GPIO ##########################################*/
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = USARTx_TX_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = USARTx_TX_AF;

    HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStruct);

    /* UART RX GPIO pin configuration  */
    GPIO_InitStruct.Pin = USARTx_RX_PIN;
    GPIO_InitStruct.Alternate = USARTx_RX_AF;

    HAL_GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
