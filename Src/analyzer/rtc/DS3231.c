#include "DS3231.h"


unsigned char bcd_to_decimal(unsigned char d)
{
         return ((d & 0x0F) + (((d & 0xF0) >> 4) * 10));
}


unsigned char decimal_to_bcd(unsigned char d)
{
         return (((d / 10) << 4) & 0xF0) | ((d % 10) & 0x0F);
}


//extern uint8_t  CAMERA_IO_Read(uint8_t addr, uint8_t reg);

unsigned char DS3231_Read(unsigned char reg)
{
 unsigned char value = 0;

        /* I2C_start();
         I2C_write(DS3231_Write_addr);
         I2C_write(address);
         I2C_start();
         I2C_write(DS3231_Read_addr);
         value = I2C_read(0);
         I2C_stop();*/
         value = CAMERA_IO_Read(DS3231_Write_addr, reg);
         return value;
}

//CAMERA_IO_Write(uint8_t addr, uint8_t reg, uint8_t value);

void DS3231_Write(unsigned char reg, unsigned char value)
{
         /*I2C_start();
         I2C_write(DS3231_Write_addr);
         I2C_write(address);
         I2C_write(value);
         I2C_stop();*/
         CAMERA_IO_Write(DS3231_Write_addr, reg, value);
}


void DS3231_init()
{
         DS3231_Write(controlREG, 0x00);
         DS3231_Write(statusREG, 0x08);
}


void getTime(uint32_t *rtctime, unsigned char *second, short *AMPM, short hour_format)
{
         unsigned char minute, hour, tmp = 0;
         //p1 = DS3231_Read(secondREG);
         *second = bcd_to_decimal(DS3231_Read(secondREG));
         //p2 = DS3231_Read(minuteREG);
         minute = bcd_to_decimal(DS3231_Read(minuteREG));
         switch(hour_format)
         {
                  case 1:// 12 h
                  {
                           tmp = DS3231_Read(hourREG);
                           tmp &= 0x20;
                           *AMPM = (short)(tmp >> 5);
                           //p3 = (0x1F & DS3231_Read(hourREG));
                           hour = bcd_to_decimal(0x1F & DS3231_Read(hourREG));
                           break;
                  }
                  default:// 24 h
                  {
                           //p3 = (0x3F & DS3231_Read(hourREG));
                           hour = bcd_to_decimal(0x3F & DS3231_Read(hourREG));
                           break;
                  }
         }
         *rtctime=100*hour + minute;
}


void getDate(uint32_t *date)
{
uint8_t p1,p2,p3,p4;
uint32_t date1;

         //p1 = DS3231_Read(yearREG);
         p1 = bcd_to_decimal(DS3231_Read(yearREG));
         //p2 = (0x1F & DS3231_Read(monthREG));
         p2 = bcd_to_decimal(0x1F & DS3231_Read(monthREG));
         p3 = (0x3F & DS3231_Read(dateREG));
         p3 = bcd_to_decimal(0x3F & DS3231_Read(dateREG));
         //p4 = (0x07 & DS3231_Read(dayREG));
         p4 = bcd_to_decimal(0x07 & DS3231_Read(dayREG));// day of week
         if(p1>=80) date1=19000000;// max year = 2079
         else       date1=20000000;
         *date=date1+10000*p1+100*p2+p3;
}


void setTime(uint32_t timeSet, unsigned char sSet, short am_pm_state, short hour_format)
{
         unsigned char hourset, tmp = 0;

         hourset=timeSet/100;
         DS3231_Write(secondREG, (decimal_to_bcd(sSet)));

         DS3231_Write(minuteREG, (decimal_to_bcd(timeSet%100)));

         switch(hour_format)
         {
                  case 1:// 12h
                  {
                           switch(am_pm_state)
                           {
                                    case 1:
                                    {
                                             tmp = 0x60;
                                             break;
                                    }
                                    default:
                                    {
                                             tmp = 0x40;
                                             break;
                                    }
                           }
                           DS3231_Write(hourREG, ((tmp | (0x1F & (decimal_to_bcd(hourset))))));
                           break;
                  }

                  default:// 0 24h
                  {
                           DS3231_Write(hourREG, (0x3F & (decimal_to_bcd(hourset))));
                  }
         }
}


void setDate(uint32_t dateSet)
{
uint32_t temp;

         temp=dateSet;
         //DS3231_Write(dayREG, (decimal_to_bcd(daySet)));
         DS3231_Write(dateREG, (decimal_to_bcd(temp%100)));//day
         temp=temp/100;
         DS3231_Write(monthREG, (decimal_to_bcd(temp%100)));//month
         temp=temp/100;
         DS3231_Write(yearREG, (decimal_to_bcd(temp%100)));//year 00..99
}


float getTemperature()
{
         register float t = 0.0;
         unsigned char lowByte = 0;
         signed char highByte = 0;
         lowByte = DS3231_Read(tempLSBREG);
         highByte = DS3231_Read(tempMSBREG);
         lowByte >>= 6;
         lowByte &= 0x03;
         t = ((float)lowByte);
         t *= 0.25;
         t += highByte;
         return t;

}
