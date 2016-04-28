#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_write(uint8_t);
uint8_t i2c_read_ack(void);
uint8_t i2c_read_nack(void);
uint8_t i2c_status(void);

#ifdef __cplusplus
}
#endif

#endif /* I2C_H_ */
