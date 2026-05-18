#include "ina238_protocol.h"
#include "i2c.h"

#include <zephyr/logging/log.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

LOG_MODULE_REGISTER(ina238_protocol, LOG_LEVEL_INF);

#define REGISTER_ADDR_BYTES 1U
#define MAX_DATA_BYTES 5U
#define MAX_FRAME_BYTES (REGISTER_ADDR_BYTES + MAX_DATA_BYTES)

static size_t build_read_frame(uint8_t reg_addr, uint8_t *frame);
static size_t build_write_frame(uint8_t reg_addr, const uint8_t *data, uint8_t num_bytes, uint8_t *frame);

int ina238_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, size_t rx_len, uint8_t num_bytes)
{
    uint8_t frame[REGISTER_ADDR_BYTES];
    size_t frame_len;

    if ((rx_buf == NULL) || (num_bytes == 0U))
    {
        LOG_ERR("invalid INA238 read args");
        return -EINVAL;
    }

    if ((num_bytes > MAX_DATA_BYTES) || (rx_len < num_bytes))
    {
        LOG_ERR("INA238 read size invalid: rx_len=%u num_bytes=%u",
                (unsigned int)rx_len, (unsigned int)num_bytes);
        return -EMSGSIZE;
    }

    frame_len = build_read_frame(reg_addr, frame);

    return i2c_transaction(dev_addr, frame, frame_len, rx_buf, num_bytes);
}

int ina238_write_reg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint8_t num_bytes)
{
    uint8_t frame[MAX_FRAME_BYTES];
    size_t frame_len;

    if ((data == NULL) || (num_bytes == 0U))
    {
        LOG_ERR("invalid INA238 write args");
        return -EINVAL;
    }

    if (num_bytes > MAX_DATA_BYTES)
    {
        LOG_ERR("INA238 write size invalid: num_bytes=%u", (unsigned int)num_bytes);
        return -EMSGSIZE;
    }

    frame_len = build_write_frame(reg_addr, data, num_bytes, frame);

    return i2c_transaction(dev_addr, frame, frame_len, NULL, 0U);
}

static size_t build_read_frame(uint8_t reg_addr, uint8_t *frame)
{
    frame[0] = reg_addr;

    return REGISTER_ADDR_BYTES;
}

static size_t build_write_frame(uint8_t reg_addr, const uint8_t *data, uint8_t num_bytes, uint8_t *frame)
{
    frame[0] = reg_addr;

    for (uint8_t i = 0; i < num_bytes; i++)
    {
        frame[REGISTER_ADDR_BYTES + i] = data[i];
    }

    return (size_t)(REGISTER_ADDR_BYTES + num_bytes);
}
