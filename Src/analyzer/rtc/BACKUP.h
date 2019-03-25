/*                Definitions for Backup and Power Control Module               */


//Miscellaneous

#define enable                                                                  0x01
#define disable                                                                 0x00

#define true                                                                    0x01
#define false                                                                   0x00

#define high                                                                    0x01
#define low                                                                     0x00


//Bitwise Operations

#define bit_set(reg, bit_val)                                                   reg |= (1 << bit_val)
#define bit_clr(reg, bit_val)                                                   reg &= (~(1 << bit_val))
#define bit_tgl(reg, bit_val)                                                   reg ^= (1 << bit_val)
#define get_bit(reg, bit_val)                                                   (reg & (1 << bit_val))
#define get_reg(reg, msk)                                                       (reg & msk)


//Defintions for RTC clock calibration register (BKP_RTCCR)

#define alarm_output                                                            low
#define second_output                                                           high

#define set_RTC_tamper_pin_output(mode)                                         BKP_RTCCRbits.ASOS = mode
#define enable_RTC_tamper_pin_output(mode)                                      BKP_RTCCRbits.ASOE = mode
#define enable_RTC_clock_calibration_on_tamper_pin(mode)                        BKP_RTCCRbits.CCO = mode
#define set_RTC_clock_calibration_value(value)                                  do{BKP_RTCCR &= 0xFFFFFF80); BKP_RTCCR |= (value & 0x7F);}while(0)


//Definitions for Backup control register (BKP_CR)

#define set_tamper_pin_active_level(mode)                                       BKP_CRbits.TPAL = mode
#define enable_tamper_pin(mode)                                                 BKP_CRbits.TPE = mode


//Definition for Backup control/status register (BKP_CSR)

#define get_tamper_interrupt_flag()                                             get_bit(BKP_CSR, 9)
#define get_tamper_event_flag()                                                 get_bit(BKP_CSR, 8)
#define enable_tamper_pin_interrupt(mode)                                       BKP_CSRbits.TPIE = mode
#define clear_tamper_interrupt()                                                BKP_CSRbits.CTI = 1
#define clear_tamper_event()                                                    BKP_CSRbits.CTE = 1


//Definition for Power control register (PWR_CR)

#define disable_backup_domain_write_protection(mode)                            PWR_CRbits.DBP = mode

#define select_PVD_level(value)                                                 do{PWR_CR &= (~(0x7 << 5)); PWR_CR |= (value << 5);}while(0)

#define enable_PVD(mode)                                                        PWR_CRbits.PVDE = mode

#define clear_standby_flag()                                                    PWR_CRbits.CSBF = 1
#define get_standby_flag()                                                      get_bit(PWR_CR, 3)

#define clear_wakeup_flag()                                                     PWR_CRbits.CWUF = 1
#define get_wakeup_flag()                                                       get_bit(PWR_CR, 2)

#define set_power_down_deep_sleep(mode)                                         PWR_CRbits.PDDS = mode

#define set_low_power_deep_sleep(mode)                                          PWR_CRbits.LPDS = mode


//Definitions for Power control status register (PWR_CSR)

#define wakeup_pin_is_a_GPIO                                                    low
#define wakeup_pin_is_used                                                      high

#define use_wakeup_pin(mode)                                                    PWR_CSRbits.EWUP = mode

#define get_PVD_output()                                                        get_bit(PWR_CSR, 2)
#define get_standby_flag()                                                      get_bit(PWR_CSR, 1)
#define get_wakeup_flag()                                                       get_bit(PWR_CSR, 0)

//Other Definitions

#define enable_backup_module(mode)                                              RCC_APB1ENRbits.BKPEN = mode
#define enable_power_control_module(mode)                                       RCC_APB1ENRbits.PWREN = mode