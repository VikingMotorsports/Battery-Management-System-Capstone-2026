#include "i2c.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(i2c_bus, LOG_LEVEL_INF);

#define I2C_NODE DT_NODELABEL(i2c1)

static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

int i2c_init(void)
{
    if (!device_is_ready(i2c_dev))
    {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    return 0;
}

int i2c_transaction(uint8_t dev_addr, const uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf, size_t rx_len)
{
    int ret;

    if (((tx_buf == NULL) && (tx_len > 0U)) || ((rx_buf == NULL) && (rx_len > 0U)))
    {
        LOG_ERR("invalid I2C transaction args");
        return -EINVAL;
    }

    if ((tx_len == 0U) && (rx_len == 0U))
    {
        LOG_ERR("empty I2C transaction");
        return -EINVAL;
    }

    if ((tx_len > 0U) && (rx_len > 0U))
    {
        ret = i2c_write_read(i2c_dev, dev_addr, tx_buf, tx_len, rx_buf, rx_len);
    }
    else if (tx_len > 0U)
    {
        ret = i2c_write(i2c_dev, tx_buf, tx_len, dev_addr);
    }
    else
    {
        ret = i2c_read(i2c_dev, rx_buf, rx_len, dev_addr);
    }

    if (ret < 0)
    {
        LOG_ERR("I2C transaction failed: dev=0x%02X tx_len=%u rx_len=%u err=%d",
                dev_addr, (unsigned int)tx_len, (unsigned int)rx_len, ret);
        return ret;
    }

    return 0;
}
