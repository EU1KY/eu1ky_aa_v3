


#ifndef DS3231_H_
#define DS3231_H_


#include <stdint.h>
#include "config.h"

#define DS3231_Address              0x68

#define DS3231_Read_addr            ((DS3231_Address << 1) | 0x01)
#define DS3231_Write_addr           ((DS3231_Address << 1) & 0xFE)

#define secondREG                   0x00
#define minuteREG                   0x01
#define hourREG                     0x02
#define dayREG                      0x03
#define dateREG                     0x04
#define monthREG                    0x05
#define yearREG                     0x06
#define alarm1secREG                0x07
#define alarm1minREG                0x08
#define alarm1hrREG                 0x09
#define alarm1dateREG               0x0A
#define alarm2minREG                0x0B
#define alarm2hrREG                 0x0C
#define alarm2dateREG               0x0D
#define controlREG                  0x0E
#define statusREG                   0x0F
#define ageoffsetREG                0x10
#define tempMSBREG                  0x11
#define tempLSBREG                  0x12

#define _24_hour_format             0
#define _12_hour_format             1
#define am                          0
#define pm                          1

//Ext I2C port used for camera is wired to Arduino UNO connector when SB4 and SB1 jumpers are set instead of SB5 and SB3.
extern void     CAMERA_IO_Init(void);
extern void     CAMERA_IO_Write(uint8_t addr, uint8_t reg, uint8_t value);
extern uint8_t  CAMERA_IO_Read(uint8_t addr, uint8_t reg);
extern void     CAMERA_Delay(uint32_t delay);
extern void     CAMERA_IO_WriteBulk(uint8_t addr, uint8_t reg, uint8_t* values, uint16_t nvalues);
extern void Sleep(uint32_t);

unsigned char bcd_to_decimal(unsigned char d);
unsigned char decimal_to_bcd(unsigned char d);
unsigned char DS3231_Read(unsigned char address);
void DS3231_Write(unsigned char address, unsigned char value);
void DS3231_init();
void getTime(uint32_t *rtctime, unsigned char *second, short *AMPM, short hour_format);
void getDate(uint32_t *date);
void setTime(uint32_t timeSet, unsigned char sSet, short am_pm_state, short hour_format);
void setDate(uint32_t dateSet);
float getTemp();

#endif
