#ifndef BQ796XX_PROTOCOL_H
#define BQ796XX_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

typedef enum
{
    SINGLE_READ,
    STACK_READ
} read_mode_t;

typedef enum
{
    SINGLE_WRITE,
    STACK_WRITE,
    BROADCAST_WRITE
} write_mode_t;

int bq796xx_read_reg(read_mode_t mode, uint8_t dev_addr, uint16_t reg_addr, uint8_t *rx_buf, size_t rx_len, uint8_t num_bytes);
int bq796xx_write_reg(write_mode_t mode, uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t num_bytes);

#endif
