/*                Definitons for GPIOs                */ 

                                                                                                 
//I/O Modes

#define output_mode_medium_speed                                                         0x1
#define output_mode_low_speed                                                            0x2
#define output_mode_high_speed                                                           0x3

//I/O Configurations

#define GPIO_PP_output                                                                   0x0
#define GPIO_open_drain_output                                                           0x4
#define AFIO_PP_output                                                                   0x8
#define AFIO_open_drain_output                                                           0xC

#define analog_input                                                                     0x0
#define floating_input                                                                   0x4
#define input_without_pull_resistors                                                     0x4
#define reset_state                                                                      0x4
#define digital_input                                                                    0x8

//Pull Resistor Configurations

#define pull_down                                                                        0x0
#define pull_up                                                                          0x1

//Miscellaneous                                                                                                                                           

#define enable                                                                           0x1
#define disable                                                                          0x0

#define true                                                                             0x1
#define false                                                                            0x0

#define high                                                                             0x1
#define low                                                                              0x0

//CR Register Configuration Macro

#define pin_configure_low(reg, pin, type)                                                do{reg &= (~(0xF << (pin << 2))); reg |= (type << (pin << 2));}while(0)
#define pin_configure_high(reg, pin, type)                                               do{reg &= (~(0xF << ((pin - 8) << 2))); reg |= (type << ((pin - 8) << 2));}while(0)

//GPIO Setups 
 
#define setup_GPIOA(pin_no, io_type)                                                     do{if((pin_no >= 0) && (pin_no < 8)){pin_configure_low(GPIOA_CRL, pin_no, io_type);}else{pin_configure_high(GPIOA_CRH, pin_no, io_type);}}while(0)
#define setup_GPIOB(pin_no, io_type)                                                     do{if((pin_no >= 0) && (pin_no < 8)){pin_configure_low(GPIOB_CRL, pin_no, io_type);}else{pin_configure_high(GPIOB_CRH, pin_no, io_type);}}while(0)
#define setup_GPIOC(pin_no, io_type)                                                     do{if((pin_no >= 0) && (pin_no < 8)){pin_configure_low(GPIOC_CRL, pin_no, io_type);}else{pin_configure_high(GPIOC_CRH, pin_no, io_type);}}while(0)
#define setup_GPIOD(pin_no, io_type)                                                     do{if((pin_no >= 0) && (pin_no < 8)){pin_configure_low(GPIOD_CRL, pin_no, io_type);}else{pin_configure_high(GPIOD_CRH, pin_no, io_type);}}while(0)
#define setup_GPIOE(pin_no, io_type)                                                     do{if((pin_no >= 0) && (pin_no < 8)){pin_configure_low(GPIOE_CRL, pin_no, io_type);}else{pin_configure_high(GPIOE_CRH, pin_no, io_type);}}while(0)
#define setup_GPIOF(pin_no, io_type)                                                     do{if((pin_no >= 0) && (pin_no < 8)){pin_configure_low(GPIOF_CRL, pin_no, io_type);}else{pin_configure_high(GPIOF_CRH, pin_no, io_type);}}while(0)
#define setup_GPIOG(pin_no, io_type)                                                     do{if((pin_no >= 0) && (pin_no < 8)){pin_configure_low(GPIOG_CRL, pin_no, io_type);}else{pin_configure_high(GPIOG_CRH, pin_no, io_type);}}while(0)
          
//Bitwise Operations for GPIOs

#define bit_set(reg, bit_val)                                                            reg |= (1 << bit_val)
#define bit_clr(reg, bit_val)                                                            reg &= (~(1 << bit_val))
#define bit_tgl(reg, bit_val)                                                            reg ^= (1 << bit_val)
#define get_bit(reg, bit_val)                                                            (reg & (1 << bit_val))
#define get_reg(reg, msk)                                                                (reg & msk)

//Port Pin Operations

#define GPIOA_pin_high(pin)                                                              bit_set(GPIOA_BSRR, pin)
#define GPIOB_pin_high(pin)                                                              bit_set(GPIOB_BSRR, pin)
#define GPIOC_pin_high(pin)                                                              bit_set(GPIOC_BSRR, pin)
#define GPIOD_pin_high(pin)                                                              bit_set(GPIOD_BSRR, pin)
#define GPIOE_pin_high(pin)                                                              bit_set(GPIOE_BSRR, pin)
#define GPIOF_pin_high(pin)                                                              bit_set(GPIOF_BSRR, pin)
#define GPIOG_pin_high(pin)                                                              bit_set(GPIOG_BSRR, pin)

#define GPIOA_pin_low(pin)                                                               bit_set(GPIOA_BRR, pin)
#define GPIOB_pin_low(pin)                                                               bit_set(GPIOB_BRR, pin)
#define GPIOC_pin_low(pin)                                                               bit_set(GPIOC_BRR, pin)
#define GPIOD_pin_low(pin)                                                               bit_set(GPIOD_BRR, pin)
#define GPIOE_pin_low(pin)                                                               bit_set(GPIOE_BRR, pin)
#define GPIOF_pin_low(pin)                                                               bit_set(GPIOF_BRR, pin)
#define GPIOG_pin_low(pin)                                                               bit_set(GPIOG_BRR, pin)

#define GPIOA_pin_toggle(pin)                                                            bit_tgl(GPIOA_ODR, pin)
#define GPIOB_pin_toggle(pin)                                                            bit_tgl(GPIOB_ODR, pin)
#define GPIOC_pin_toggle(pin)                                                            bit_tgl(GPIOC_ODR, pin)
#define GPIOD_pin_toggle(pin)                                                            bit_tgl(GPIOD_ODR, pin)
#define GPIOE_pin_toggle(pin)                                                            bit_tgl(GPIOE_ODR, pin)
#define GPIOF_pin_toggle(pin)                                                            bit_tgl(GPIOF_ODR, pin)
#define GPIOG_pin_toggle(pin)                                                            bit_tgl(GPIOG_ODR, pin)

#define get_GPIOA_pin_state(pin)                                                         get_bit(GPIOA_IDR, pin)
#define get_GPIOB_pin_state(pin)                                                         get_bit(GPIOB_IDR, pin)
#define get_GPIOC_pin_state(pin)                                                         get_bit(GPIOC_IDR, pin)
#define get_GPIOD_pin_state(pin)                                                         get_bit(GPIOD_IDR, pin)
#define get_GPIOE_pin_state(pin)                                                         get_bit(GPIOE_IDR, pin)
#define get_GPIOF_pin_state(pin)                                                         get_bit(GPIOF_IDR, pin)
#define get_GPIOG_pin_state(pin)                                                         get_bit(GPIOG_IDR, pin)

#define get_GPIOA_port(mask)                                                             get_reg(GPIOA_IDR, mask)
#define get_GPIOB_port(mask)                                                             get_reg(GPIOB_IDR, mask)
#define get_GPIOC_port(mask)                                                             get_reg(GPIOC_IDR, mask)
#define get_GPIOD_port(mask)                                                             get_reg(GPIOD_IDR, mask)
#define get_GPIOE_port(mask)                                                             get_reg(GPIOE_IDR, mask)
#define get_GPIOF_port(mask)                                                             get_reg(GPIOF_IDR, mask)
#define get_GPIOG_port(mask)                                                             get_reg(GPIOG_IDR, mask)

//Pull Resistor Functions

#define enable_pull_up_GPIOA(pin)                                                        bit_set(GPIOA_ODR, pin)
#define enable_pull_up_GPIOB(pin)                                                        bit_set(GPIOB_ODR, pin)
#define enable_pull_up_GPIOC(pin)                                                        bit_set(GPIOC_ODR, pin)
#define enable_pull_up_GPIOD(pin)                                                        bit_set(GPIOD_ODR, pin)
#define enable_pull_up_GPIOE(pin)                                                        bit_set(GPIOE_ODR, pin)
#define enable_pull_up_GPIOF(pin)                                                        bit_set(GPIOF_ODR, pin)
#define enable_pull_up_GPIOG(pin)                                                        bit_set(GPIOG_ODR, pin)

#define enable_pull_down_GPIOA(pin)                                                      bit_clr(GPIOA_ODR, pin)
#define enable_pull_down_GPIOB(pin)                                                      bit_clr(GPIOB_ODR, pin)
#define enable_pull_down_GPIOC(pin)                                                      bit_clr(GPIOC_ODR, pin)
#define enable_pull_down_GPIOD(pin)                                                      bit_clr(GPIOD_ODR, pin)
#define enable_pull_down_GPIOE(pin)                                                      bit_clr(GPIOE_ODR, pin)
#define enable_pull_down_GPIOF(pin)                                                      bit_clr(GPIOF_ODR, pin)
#define enable_pull_down_GPIOG(pin)                                                      bit_clr(GPIOG_ODR, pin)

//GPIO Enabling Functions

#define enable_GPIOA(mode)                                                               RCC_APB2ENRbits.IOPAEN = mode
#define enable_GPIOB(mode)                                                               RCC_APB2ENRbits.IOPBEN = mode
#define enable_GPIOC(mode)                                                               RCC_APB2ENRbits.IOPCEN = mode
#define enable_GPIOD(mode)                                                               RCC_APB2ENRbits.IOPDEN = mode
#define enable_GPIOE(mode)                                                               RCC_APB2ENRbits.IOPEEN = mode
#define enable_GPIOF(mode)                                                               RCC_APB2ENRbits.IOPFEN = mode
#define enable_GPIOG(mode)                                                               RCC_APB2ENRbits.IOPGEN = mode