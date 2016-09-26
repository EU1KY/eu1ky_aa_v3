#include "custom_spi2.h"
#include "stm32746g_discovery.h"

// Definition for SPIx
#define SPIx                             SPI2
#define SPIx_CLK_ENABLE()                __HAL_RCC_SPI2_CLK_ENABLE()
#define SPIx_SCK_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOI_CLK_ENABLE()
#define SPIx_MISO_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define SPIx_MOSI_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

#define SPIx_FORCE_RESET()               __HAL_RCC_SPI2_FORCE_RESET()
#define SPIx_RELEASE_RESET()             __HAL_RCC_SPI2_RELEASE_RESET()

// Definition for SPIx Pins
#define SPIx_SCK_PIN                     GPIO_PIN_1
#define SPIx_SCK_GPIO_PORT               GPIOI
#define SPIx_SCK_AF                      GPIO_AF5_SPI2
#define SPIx_MISO_PIN                    GPIO_PIN_14
#define SPIx_MISO_GPIO_PORT              GPIOB
#define SPIx_MISO_AF                     GPIO_AF5_SPI2
#define SPIx_MOSI_PIN                    GPIO_PIN_15
#define SPIx_MOSI_GPIO_PORT              GPIOB
#define SPIx_MOSI_AF                     GPIO_AF5_SPI2

// Definitions for Slave Select pins
#define SPIx_SLAVE0_PIN                  GPIO_PIN_1
#define SPIx_SLAVE0_GPIO_PORT            GPIOI
#define SPIx_SLAVE0_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOI_CLK_ENABLE()

#define SPIx_SLAVE1_PIN                  GPIO_PIN_15
#define SPIx_SLAVE1_GPIO_PORT            GPIOA
#define SPIx_SLAVE1_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOA_CLK_ENABLE()

static SPI_HandleTypeDef SpiHandle;

void SPI2_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    // Enable GPIO TX/RX clock
    SPIx_SCK_GPIO_CLK_ENABLE();
    SPIx_MISO_GPIO_CLK_ENABLE();
    SPIx_MOSI_GPIO_CLK_ENABLE();

    // Enable SPI clock
    SPIx_CLK_ENABLE();

    // SPI SCK GPIO pin configuration
    GPIO_InitStruct.Pin       = SPIx_SCK_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = SPIx_SCK_AF;
    HAL_GPIO_Init(SPIx_SCK_GPIO_PORT, &GPIO_InitStruct);

    // SPI MISO GPIO pin configuration
    GPIO_InitStruct.Pin = SPIx_MISO_PIN;
    GPIO_InitStruct.Alternate = SPIx_MISO_AF;
    HAL_GPIO_Init(SPIx_MISO_GPIO_PORT, &GPIO_InitStruct);

    // SPI MOSI GPIO pin configuration
    GPIO_InitStruct.Pin = SPIx_MOSI_PIN;
    GPIO_InitStruct.Alternate = SPIx_MOSI_AF;
    HAL_GPIO_Init(SPIx_MOSI_GPIO_PORT, &GPIO_InitStruct);

    SpiHandle.Instance               = SPIx;
    SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
    SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;         //Todo: set properly
    SpiHandle.Init.CLKPolarity       = SPI_POLARITY_HIGH;       //Todo: set properly
    SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
    SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;        //Todo: set properly
    SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
    SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    SpiHandle.Init.CRCPolynomial     = 7;
    SpiHandle.Init.NSS               = SPI_NSS_SOFT;
    SpiHandle.Init.Mode = SPI_MODE_MASTER;

    HAL_SPI_Init(&SpiHandle);

    SPIx_SLAVE0_GPIO_CLK_ENABLE();
    SPIx_SLAVE1_GPIO_CLK_ENABLE();
    GPIO_InitStruct.Pin       = SPIx_SLAVE0_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = 0;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    HAL_GPIO_Init(SPIx_SLAVE0_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = SPIx_SLAVE1_PIN;
    HAL_GPIO_Init(SPIx_SLAVE1_GPIO_PORT, &GPIO_InitStruct);

    SPI2_DeselectSlave();
}

void SPI2_SelectSlave(SPI2_Slave_t slave)
{
    assert_param(slave == SPI2_SLAVE_0 || slave == SPI2_SLAVE_1);
    SPI2_DeselectSlave();
    if (SPI2_SLAVE_0 == slave)
    {
        HAL_GPIO_WritePin(SPIx_SLAVE0_GPIO_PORT, SPIx_SLAVE0_PIN, GPIO_PIN_RESET);
    }
    else if (SPI2_SLAVE_1 == slave)
    {
        HAL_GPIO_WritePin(SPIx_SLAVE1_GPIO_PORT, SPIx_SLAVE1_PIN, GPIO_PIN_RESET);
    }
}

void SPI2_DeselectSlave(void)
{
    HAL_GPIO_WritePin(SPIx_SLAVE0_GPIO_PORT, SPIx_SLAVE0_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SPIx_SLAVE1_GPIO_PORT, SPIx_SLAVE1_PIN, GPIO_PIN_SET);
}

void SPI2_Transmit(uint8_t *pDataTx, uint32_t nBytes)
{
    HAL_SPI_Transmit(&SpiHandle, pDataTx, nBytes, 500);
}
