/*                Definitions for RTC Module                */


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


//Definitions for RTC Control Registers

#define enable_RTC_overflow_interrupt(mode)                                     RTC_CRHbits.OWIE = mode
#define enable_RTC_alarm_interrupt(mode)                                        RTC_CRHbits.ALRIE = mode
#define enable_RTC_second_interrupt(mode)                                       RTC_CRHbits.SECIE = mode

#define get_RTC_operation_state()                                               get_bit(RTC_CRL, 5)

#define get_RTC_configuration_state()                                           get_bit(RTC_CRL, 4)
#define set_RTC_configuration_flag(mode)                                        RTC_CRLbits.CNF = mode

#define get_RTC_register_sync_state()                                           get_bit(RTC_CRL, 3)
#define clear_RTC_register_sync_state_flag()                                    bit_clr(RTC_CRL, 3)

#define get_RTC_overflow_flag_state()                                           get_bit(RTC_CRL, 2)
#define clear_RTC_overflow_flag()                                               bit_clr(RTC_CRL, 2)

#define get_RTC_alarm_flag_state()                                              get_bit(RTC_CRL, 1)
#define clear_RTC_alarm_flag()                                                  bit_clr(RTC_CRL, 1)

#define get_RTC_second_flag_state()                                             get_bit(RTC_CRL, 0)
#define clear_RTC_second_flag()                                                 bit_clr(RTC_CRL, 0)


//Definitions for RTC Prescalar Load Registers

#define set_RTC_prescalar(value)                                                do{RTC_PRLL = 0; RTC_PRLH = 0; RTC_PRLL = (0xFFFF & value); RTC_PRLH = (value >> 16); }while(0)


//Definitions for RTC Prescalar Divider Registers

#define set_RTC_prescalar_divider(value)                                        do{RTC_DIVL = 0; RTC_DIVH = 0; RTC_DIVL = (0xFFFF & value); RTC_DIVH = (value >> 16); }while(0)


//Definitions for RTC Counter Registers

#define set_RTC_counter(value)                                                  do{RTC_CNTL = 0; RTC_CNTH = 0; RTC_CNTL = (0xFFFF & value); RTC_CNTH = (value >> 16); }while(0)


//Definitions for RTC Alarm Registers

#define set_RTC_alarm(value)                                                    do{RTC_ALRL = 0; RTC_ALRH = 0; RTC_ALRL = (0xFFFF & value); RTC_ALRH = (value >> 16); }while(0)


//Other defintions (related to RCC)

#define enable_LSI(mode)                                                        RCC_CSRbits.LSION = mode
#define LSI_ready()                                                             get_bit(RCC_CSR, 1)

#define set_backup_domain_software_reset(mode)                                  RCC_BDCRbits.BDRST = mode
#define enable_RTC_clock(mode)                                                  RCC_BDCRbits.RTCEN = mode

#define no_clock                                                                0x00
#define LSE_clock                                                               0x01
#define LSI_clock                                                               0x02
#define HSE_by_128_clock                                                        0x03

#define select_RTC_clock_source(value)                                          do{RCC_BDCR &= (~(0x3 << 8)); RCC_BDCR |= (value << 8);}while(0)
#define bypass_LSE_with_external_clock(mode)                                    RCC_BDCRbits.LSEBYP = mode
#define LSE_ready()                                                             get_bit(RCC_BDCR, 1)
#define enable_LSE(mode)                                                        RCC_BDCRbits.LSEON = mode