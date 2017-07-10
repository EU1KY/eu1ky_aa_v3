# This makefile to build firmware from command line in Windows
# Before running make, ensure that path to arm-none-eabi-gcc is added to the system path, e.g. like this:
# set PATH=d:\EmBitz\share\em_armgcc\bin\;%PATH%

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

CFLAGS  = -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -fgcse -fexpensive-optimizations -fomit-frame-pointer \
          -fdata-sections -ffunction-sections -Os -g -mfpu=fpv5-sp-d16 -flto -MMD

ASFLAGS = -mcpu=cortex-m7 -mthumb -Wa,--gdwarf-2

LDFLAGS = -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -fgcse -fexpensive-optimizations -fomit-frame-pointer \
          -fdata-sections -ffunction-sections -Os -g -mthumb -mfpu=fpv5-sp-d16 -Wl,-Map=bin/Release/F7Discovery.map \
          -u _printf_float -specs=nano.specs -Wl,--gc-sections -flto -TSrc/Sys/STM32F746NGHx_FLASH.ld -lm

DEFINE = -DSTM32F746xx \
         -DSTM32F746G_DISCO \
         -DSTM32F746NGHx \
         -DSTM32F7 \
         -DSTM32 \
         -DUSE_HAL_DRIVER \
         -DARM_MATH_CM7 \
         -D__FPU_PRESENT \
         -DUSE_USB_HS \
         -DLODEPNG_NO_COMPILE_DISK \
         -DLODEPNG_NO_COMPILE_ERROR_TEXT \
         -DLODEPNG_NO_COMPILE_ALLOCATORS \
         -DLODEPNG_NO_COMPILE_ANCILLARY_CHUNKS \
         -DLODEPNG_NO_COMPILE_DECODER

INCLUDE = -ISrc/BSP/Components/ampire480272 \
          -ISrc/BSP/Components/ampire640480 \
          -ISrc/BSP/Components/Common \
          -ISrc/BSP/Components/exc7200 \
          -ISrc/BSP/Components/ft5336 \
          -ISrc/BSP/Components/mfxstm32l152 \
          -ISrc/BSP/Components/n25q128a \
          -ISrc/BSP/Components/n25q512a \
          -ISrc/BSP/Components/ov9655 \
          -ISrc/BSP/Components/rk043fn48h \
          -ISrc/BSP/Components/s5k5cag \
          -ISrc/BSP/Components/stmpe811 \
          -ISrc/BSP/Components/ts3510 \
          -ISrc/BSP/Components/wm8994 \
          -ISrc/BSP/STM32746G-Discovery \
          -ISrc/CMSIS/Device/ST/STM32F7xx/Include \
          -ISrc/CMSIS/Include -ISrc/Inc \
          -ISrc/STM32F7xx_HAL_Driver/Inc \
          -ISrc/Utilities/Fonts \
          -ISrc/STM32F7xx_HAL_Driver/Inc/Legacy \
          -ISrc/fatfs -ISrc/fatfs/drivers \
          -ISrc/config \
          -ISrc/gen \
          -ISrc/dsp \
          -ISrc/USBD \
          -ISrc/uartcomm \
          -I./Src/analyzer/config \
          -I./Src/analyzer/dsp \
          -I./Src/analyzer/gen \
          -I./Src/analyzer/lcd/bitmaps \
          -I./Src/analyzer/lcd \
          -I./Src/analyzer/osl \
          -I./Src/analyzer/uartcomm \
          -I./Src/analyzer/window \
          -I./Src/BSP/Components/ampire480272 \
          -I./Src/BSP/Components/ampire640480 \
          -I./Src/BSP/Components/Common \
          -I./Src/BSP/Components/ft5336 \
          -I./Src/BSP/Components/n25q128a \
          -I./Src/BSP/Components/n25q512a \
          -I./Src/BSP/Components/ov9655 \
          -I./Src/BSP/Components/rk043fn48h \
          -I./Src/BSP/Components/s5k5cag \
          -I./Src/BSP/Components/wm8994 \
          -I./Src/BSP/STM32746G-Discovery \
          -I./Src/CMSIS/Device/ST/STM32F7xx/Include \
          -I./Src/CMSIS/Include \
          -I./Src/CMSIS/RTOS/Template \
          -I./Src/fatfs \
          -I./Src/fatfs/drivers \
          -I./Src/Inc \
          -I./Src/STM32F7xx_HAL_Driver/Inc/Legacy \
          -I./Src/STM32F7xx_HAL_Driver/Inc \
          -I./Src/USBD

SRC :=   Src/analyzer/config/config.c \
        Src/analyzer/dsp/dsp.c \
        Src/analyzer/gen/adf4350.c \
        Src/analyzer/gen/adf4351.c \
        Src/analyzer/gen/si5338a.c \
        Src/analyzer/gen/gen.c \
        Src/analyzer/gen/rational.c \
        Src/analyzer/gen/si5351.c \
        Src/analyzer/lcd/bitmaps/bitmaps.c \
        Src/analyzer/lcd/consbig.c \
        Src/analyzer/lcd/font.c \
        Src/analyzer/lcd/fran.c \
        Src/analyzer/lcd/franbig.c \
        Src/analyzer/lcd/hit.c \
        Src/analyzer/lcd/LCD.c \
        Src/analyzer/lcd/libnsbmp.c \
        Src/analyzer/lcd/lodepng.c \
        Src/analyzer/lcd/sdigits.c \
        Src/analyzer/lcd/smith.c \
        Src/analyzer/lcd/textbox.c \
        Src/analyzer/lcd/touch.c \
        Src/analyzer/osl/match.c \
        Src/analyzer/osl/oslfile.c \
        Src/analyzer/uartcomm/aauart.c \
        Src/analyzer/uartcomm/fifo.c \
        Src/analyzer/window/fftwnd.c \
        Src/analyzer/window/generator.c \
        Src/analyzer/window/keyboard.c \
        Src/analyzer/window/mainwnd.c \
        Src/analyzer/window/measurement.c \
        Src/analyzer/window/num_keypad.c \
        Src/analyzer/window/oslcal.c \
        Src/analyzer/window/panfreq.c \
        Src/analyzer/window/panvswr2.c \
        Src/analyzer/window/screenshot.c \
        Src/analyzer/window/tdr.c \
        Src/BSP/Components/ft5336/ft5336.c \
        Src/BSP/Components/ov9655/ov9655.c \
        Src/BSP/Components/s5k5cag/s5k5cag.c \
        Src/BSP/Components/wm8994/wm8994.c \
        Src/BSP/STM32746G-Discovery/custom_spi2.c \
        Src/BSP/STM32746G-Discovery/stm32746g_discovery.c \
        Src/BSP/STM32746G-Discovery/stm32746g_discovery_audio.c \
        Src/BSP/STM32746G-Discovery/stm32746g_discovery_eeprom.c \
        Src/BSP/STM32746G-Discovery/stm32746g_discovery_lcd.c \
        Src/BSP/STM32746G-Discovery/stm32746g_discovery_qspi.c \
        Src/BSP/STM32746G-Discovery/stm32746g_discovery_sd.c \
        Src/BSP/STM32746G-Discovery/stm32746g_discovery_sdram.c \
        Src/BSP/STM32746G-Discovery/stm32746g_discovery_ts.c \
        Src/CMSIS/DSP_Lib/Source/CommonTables/arm_common_tables.c \
        Src/CMSIS/DSP_Lib/Source/CommonTables/arm_const_structs.c \
        Src/CMSIS/DSP_Lib/Source/TransformFunctions/arm_cfft_f32.c \
        Src/CMSIS/DSP_Lib/Source/TransformFunctions/arm_cfft_radix8_f32.c \
        Src/CMSIS/DSP_Lib/Source/TransformFunctions/arm_rfft_fast_f32.c \
        Src/CMSIS/DSP_Lib/Source/TransformFunctions/arm_rfft_fast_init_f32.c \
        Src/fatfs/diskio.c \
        Src/fatfs/drivers/sd_diskio.c \
        Src/fatfs/ff.c \
        Src/fatfs/ff_gen_drv.c \
        Src/fatfs/option/syscall.c \
        Src/fatfs/option/unicode.c \
        Src/main.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_can.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cec.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_crc.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_crc_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cryp.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cryp_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dac.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dac_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dcmi.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dcmi_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma2d.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_eth.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_hash.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_hash_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_hcd.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_irda.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_iwdg.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_lptim.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_ltdc.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_nand.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_nor.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_qspi.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rtc.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rtc_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sai.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sai_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sdram.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_smartcard.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_smartcard_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spdifrx.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sram.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_usart.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_wwdg.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_fmc.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c \
        Src/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c \
        Src/Sys/sdram_heap.c \
        Src/Sys/stm32f7xx_hal_msp.c \
        Src/Sys/stm32f7xx_it.c \
        Src/Sys/syscalls.c \
        Src/Sys/system_stm32f7xx.c \
        Src/USBD/usbd_conf.c \
        Src/USBD/usbd_core.c \
        Src/USBD/usbd_ctlreq.c \
        Src/USBD/usbd_desc.c \
        Src/USBD/usbd_ioreq.c \
        Src/USBD/usbd_msc.c \
        Src/USBD/usbd_msc_bot.c \
        Src/USBD/usbd_msc_data.c \
        Src/USBD/usbd_msc_scsi.c \
        Src/USBD/usbd_storage.c

ASRC := Src/CMSIS/DSP_Lib/Source/TransformFunctions/arm_bitreversal2.S

ASRC2 := Src/Sys/startup_stm32f746xx.s

OBJS := $(SRC:%.c=obj/release/%.c.o)
OBJS += $(ASRC:%.S=obj/release/%.S.o)
OBJS += $(ASRC2:%.s=obj/release/%.s.o)

DEPS = $(OBJS:.o=.d)

all: gen bin/Release/F7Discovery.elf

clean:
	@if exist "Src/Inc/build_timestamp.h" del "Src\Inc\build_timestamp.h"
	@del /F /S /Q bin > nul
	@del /F /S /Q obj > nul
	@gen_timestamp.bat

gen: Src\Inc\build_timestamp.h
	@gen_timestamp.bat

bin/Release/F7Discovery.elf : $(OBJS)
	@if not exist "bin/Release" mkdir "bin/Release"
	$(CC) $(OBJS) -o bin/Release/F7Discovery.elf $(LDFLAGS)
	@$(OBJCOPY) -O ihex $@ $@.hex
	@$(OBJCOPY) -O binary $@ $@.bin
	@$(SIZE) bin\Release\F7Discovery.elf

obj/release/%.c.o : %.c
	@if not exist "$(@D)" mkdir "$(@D)"
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFINE) -c $< -o $@

obj/release/%.S.o : %.S
	@if not exist "$(@D)" mkdir "$(@D)"
	$(CC) $(CFLAGS) -c $< -o $@

obj/release/%.s.o : %.s
	@if not exist "$(@D)" mkdir "$(@D)"
	$(CC) $(ASFLAGS) -MMD -c $< -o $@

-include $(DEPS)
