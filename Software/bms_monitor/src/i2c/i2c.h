#ifndef I2C_H
#define I2C_H

#include <stddef.h>
#include <stdint.h>

int i2c_init(void);
int i2c_transaction(uint8_t dev_addr, const uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf, size_t rx_len);

#endif
