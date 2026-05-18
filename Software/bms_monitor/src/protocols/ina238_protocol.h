#ifndef INA238_PROTOCOL_H
#define INA238_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

int ina238_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, size_t rx_len, uint8_t num_bytes);
int ina238_write_reg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint8_t num_bytes);

#endif
