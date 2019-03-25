#include "RTC.h"
#include "GPIO.h"
#include "BACKUP.h"

#define rtc_access_code             0x9999

#define set_button_pin              0
#define inc_button_pin              1
#define dec_button_pin              2
#define esc_button_pin              3


const unsigned char month_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

unsigned char cal_hour = 0;
unsigned char cal_date = 1;
unsigned char cal_month = 1;
unsigned char cal_minute = 0;
unsigned char cal_second = 0;

unsigned int cal_year = 1970;



void setup_mcu();
void setup_GPIOs();
unsigned char RTC_init();
void get_RTC();
void set_RTC(unsigned int year, unsigned char month, unsigned char date, unsigned char hour, unsigned char minute, unsigned char second);
unsigned char check_for_leap_year(unsigned int year);
void show_value(unsigned char x_pos, unsigned char y_pos, unsigned char value);
void show_year(unsigned char x_pos, unsigned char y_pos, unsigned int value);
unsigned int change_value(unsigned char x_pos, unsigned char y_pos, signed int value, signed int value_min, signed int value_max, unsigned char value_type);
void settings();


void RTC_ISR()
iv IVT_INT_RTC
ics ICS_AUTO
{
    if(get_RTC_second_flag_state() == true)
    {
        update_time = 1;
        clear_RTC_second_flag();
    }

    clear_RTC_overflow_flag();
    while(get_RTC_operation_state() == false);
}


void main()
{
     setup_mcu();

     lcd_out(1, 7, ":  :");
     lcd_out(2, 6, "/  /");

     while(1)
     {
             settings();

             if(update_time)
             {
                 get_RTC();
                 show_value(5, 1, cal_hour);
                 show_value(8, 1, cal_minute);
                 show_value(11, 1, cal_second);
                 show_value(4, 2, cal_date);
                 show_value(7, 2, cal_month);
                 show_year(10, 2, cal_year);
                 update_time = 0;
             }
     };
}


void setup_mcu()
{
     unsigned char i = 0;

     setup_GPIOs();

     Lcd_Init();
     Lcd_Cmd(_LCD_CLEAR);
     Lcd_Cmd(_LCD_CURSOR_OFF);

     Lcd_Out(1, 4, "STM32 RTC.");
     i = RTC_init();

     switch(i)
     {
         case 1:
         {
              lcd_out(2, 1, "RTC init. failed");
              while(1);
         }
         default:
         {
              lcd_out(2, 1, "RTC init success");
              delay_ms(2000);
              break;
         }
     }

     Lcd_Cmd(_LCD_CLEAR);
}


void setup_GPIOs()
{
     enable_GPIOA(true);
     setup_GPIOA(set_button_pin, digital_input);
     enable_pull_up_GPIOA(set_button_pin);

     setup_GPIOA(inc_button_pin, digital_input);
     enable_pull_up_GPIOA(inc_button_pin);

     setup_GPIOA(dec_button_pin, digital_input);
     enable_pull_up_GPIOA(dec_button_pin);

     setup_GPIOA(esc_button_pin, digital_input);
     enable_pull_up_GPIOA(esc_button_pin);

     enable_GPIOB(true);
     setup_GPIOB(10, (GPIO_PP_output | output_mode_medium_speed));
     setup_GPIOB(11, (GPIO_PP_output | output_mode_medium_speed));
     setup_GPIOB(12, (GPIO_PP_output | output_mode_medium_speed));
     setup_GPIOB(13, (GPIO_PP_output | output_mode_medium_speed));
     setup_GPIOB(14, (GPIO_PP_output | output_mode_medium_speed));
     setup_GPIOB(15, (GPIO_PP_output | output_mode_medium_speed));
}


unsigned char RTC_init()
{
    unsigned char timeout = 0;

    if(BKP_DR1 != rtc_access_code)
    {
        enable_power_control_module(true);
        enable_backup_module(true);

        disable_backup_domain_write_protection(true);
        set_backup_domain_software_reset(true);
        set_backup_domain_software_reset(false);

        bypass_LSE_with_external_clock(false);
        enable_LSE(true);
        while((LSE_ready() == false) && (timeout < 250))
        {
            timeout++;
            delay_ms(10);
        };

        if(timeout > 250)
        {
            return 1;
        }

        select_RTC_clock_source(LSE_clock);
        enable_RTC_clock(true);

        while(get_RTC_operation_state() == false);
        while(get_RTC_register_sync_state() == false);
        enable_RTC_second_interrupt(true);
        while(get_RTC_operation_state() == false);

        set_RTC_configuration_flag(true);
        set_RTC_prescalar(32767);
        set_RTC_configuration_flag(false);

        while(get_RTC_operation_state() == false);
        BKP_DR1 = rtc_access_code;
        disable_backup_domain_write_protection(false);

        set_RTC(cal_year, cal_month, cal_date, cal_hour, cal_minute, cal_second);
    }

    else
    {
        while(get_RTC_register_sync_state() == false);
        enable_RTC_second_interrupt(true);
        while(get_RTC_operation_state() == false);
    }

    NVIC_IntEnable(IVT_INT_RTC);

    return 0;
}


void get_RTC()
{
     unsigned int temp1 = 0;
     static unsigned int day_count;

     unsigned long temp = 0;
     unsigned long counts = 0;

     counts = RTC_CNTH;
     counts <<= 16;
     counts += RTC_CNTL;

     temp = (counts / 86400);

     if(day_count != temp)
     {
         day_count = temp;
         temp1 = 1970;

         while(temp >= 365)
         {
             if(check_for_leap_year(temp1) == 1)
             {
                 if(temp >= 366)
                 {
                     temp -= 366;
                 }

                 else
                 {
                     break;
                 }
             }

             else
             {
                 temp -= 365;
             }

             temp1++;
         };

         cal_year = temp1;

         temp1 = 0;
         while(temp >= 28)
         {
             if((temp1 == 1) && (check_for_leap_year(cal_year) == 1))
             {
                 if(temp >= 29)
                 {
                     temp -= 29;
                 }

                 else
                 {
                     break;
                 }
             }

             else
             {
                 if(temp >= month_table[temp1])
                 {
                     temp -= ((unsigned long)month_table[temp1]);
                 }

                 else
                 {
                     break;
                 }
             }

             temp1++;
         };

         cal_month = (temp1 + 1);
         cal_date = (temp + 1);
     }

     temp = (counts % 86400);

     cal_hour = (temp / 3600);
     cal_minute = ((temp % 3600) / 60);
     cal_second = ((temp % 3600) % 60);
}


void set_RTC(unsigned int year, unsigned char month, unsigned char date, unsigned char hour, unsigned char minute, unsigned char second)
{
    unsigned int i = 0;
    unsigned long counts = 0;

    if(year > 2099)
    {
        year = 2099;
    }

    if(year < 1970)
    {
        year = 1970;
    }

    for(i = 1970; i < year; i++)
    {
          if(check_for_leap_year(i) == 1)
          {
              counts += 31622400;
          }

          else
          {
              counts += 31536000;
          }
    }

    month -= 1;

    for(i = 0; i < month; i++)
    {
          counts += (((unsigned long)month_table[i]) * 86400);
    }

    if(check_for_leap_year(cal_year) == 1)
    {
        counts += 86400;
    }

    counts += ((unsigned long)(date - 1) * 86400);
    counts += ((unsigned long)hour * 3600);
    counts += ((unsigned long)minute * 60);
    counts += second;

    enable_power_control_module(true);
    enable_backup_module(true);

    disable_backup_domain_write_protection(true);

    set_RTC_configuration_flag(true);
    set_RTC_counter(counts);
    set_RTC_configuration_flag(false);

    while(get_RTC_operation_state() == false);

    disable_backup_domain_write_protection(false);
}


unsigned char check_for_leap_year(unsigned int year)
{
    if(year % 4 == 0)
    {
        if(year % 100 == 0)
        {
            if(year % 400 == 0)
            {
                return 1;
            }

            else
            {
                return 0;
            }
        }

        else
        {
            return 1;
        }
    }

    else
    {
        return 0;
    }
}


void show_value(unsigned char x_pos, unsigned char y_pos, unsigned char value)
{
     unsigned char ch = 0;

     ch = (value / 10);
     lcd_chr(y_pos, x_pos, (ch + 0x30));
     ch = (value % 10);
     lcd_chr(y_pos, (x_pos + 1), (ch + 0x30));
}


void show_year(unsigned char x_pos, unsigned char y_pos, unsigned int value)
{
     unsigned char temp = 0;

     temp = (value / 100);
     show_value(x_pos, y_pos, temp);
     temp = (value % 100);
     show_value((x_pos + 2), y_pos, temp);
}


unsigned int change_value(unsigned char x_pos, unsigned char y_pos, signed int value, signed int value_min, signed int value_max, unsigned char value_type)
{
    while(1)
    {
        switch(value_type)
        {
            case 1:
            {
                 lcd_out(y_pos, x_pos, "    ");
                 break;
            }
            default:
            {
                 lcd_out(y_pos, x_pos, "  ");
                 break;
            }
        }
        delay_ms(60);

        if(get_GPIOA_pin_state(inc_button_pin) == low)
        {
            value++;
        }

        if(value > value_max)
        {
            value = value_min;
        }

        if(get_GPIOA_pin_state(dec_button_pin) == low)
        {
            value--;
        }

        if(value < value_min)
        {
            value = value_max;
        }

        switch(value_type)
        {
            case 1:
            {
                 show_year(x_pos, y_pos, ((unsigned int)value));
                 break;
            }
            default:
            {
                 show_value(x_pos, y_pos, ((unsigned char)value));
                 break;
            }
        }
        delay_ms(90);

        if(get_GPIOA_pin_state(esc_button_pin) == low)
        {
            while(get_GPIOA_pin_state(esc_button_pin) == low);
            delay_ms(200);
            return value;
        }
    };
}


void settings()
{
    if(get_GPIOA_pin_state(set_button_pin) == low)
    {
        while(get_GPIOA_pin_state(set_button_pin) == low);
        NVIC_IntDisable(IVT_INT_RTC);
        update_time = 0;

        cal_hour = change_value(5, 1, cal_hour, 0, 23, 0);
        cal_minute = change_value(8, 1, cal_minute, 0, 59, 0);
        cal_second = change_value(11, 1, cal_second, 0, 59, 0);
        cal_date = change_value(4, 2, cal_date, 1, 31, 0);
        cal_month = change_value(7, 2, cal_month, 1, 12, 0);
        cal_year = change_value(10, 2, cal_year, 1970, 2099, 1);

        set_RTC(cal_year, cal_month, cal_date, cal_hour, cal_minute, cal_second);
        NVIC_IntEnable(IVT_INT_RTC);
    }
}
