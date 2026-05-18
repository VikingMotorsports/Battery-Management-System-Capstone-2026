#include "current.h"
#include "ina238_protocol.h"
#include "../register_maps/ina238_regs.h"

#include <zephyr/logging/log.h>
#include <errno.h>
#include <stdint.h>

LOG_MODULE_REGISTER(current, LOG_LEVEL_INF);

#define INA238_I2C_ADDR 0x40U
#define INA238_U16_NUM_BYTES 2U
#define INA238_SHUNT_CAL_VALUE 1250U
#define INA238_VSHUNT_LSB_UV 5.0f
#define INA238_CURRENT_LSB_A (50.0f / 32768.0f)

static void parse_current_measurements(const uint8_t *vshunt_rx_buf, const uint8_t *current_rx_buf, current_data_t *current);

int current_init(void)
{
    uint8_t data[INA238_U16_NUM_BYTES];
    int ret;

    data[0] = (uint8_t)(INA238_SHUNT_CAL_VALUE >> 8);
    data[1] = (uint8_t)(INA238_SHUNT_CAL_VALUE & 0xFFU);

    ret = ina238_write_reg(INA238_I2C_ADDR, INA238_REG_SHUNT_CAL, data, INA238_U16_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("INA238 SHUNT_CAL write failed: reg=0x%02X err=%d", INA238_REG_SHUNT_CAL, ret);
        return ret;
    }

    return 0;
}

int read_current(current_data_t *current)
{
    uint8_t vshunt_rx_buf[INA238_U16_NUM_BYTES];
    uint8_t current_rx_buf[INA238_U16_NUM_BYTES];
    int ret;

    if (current == NULL)
    {
        LOG_ERR("read_current got NULL output pointer");
        return -EINVAL;
    }

    ret = ina238_read_reg(INA238_I2C_ADDR, INA238_REG_VSHUNT, vshunt_rx_buf, sizeof(vshunt_rx_buf), INA238_U16_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("INA238 VSHUNT read failed: reg=0x%02X err=%d", INA238_REG_VSHUNT, ret);
        return ret;
    }

    ret = ina238_read_reg(INA238_I2C_ADDR, INA238_REG_CURRENT, current_rx_buf, sizeof(current_rx_buf), INA238_U16_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("INA238 CURRENT read failed: reg=0x%02X err=%d", INA238_REG_CURRENT, ret);
        return ret;
    }

    parse_current_measurements(vshunt_rx_buf, current_rx_buf, current);

    return 0;
}

static void parse_current_measurements(const uint8_t *vshunt_rx_buf, const uint8_t *current_rx_buf, current_data_t *current)
{
    current->raw_shunt_voltage = (int16_t)(((uint16_t)vshunt_rx_buf[0] << 8) | (uint16_t)vshunt_rx_buf[1]);
    current->raw_current = (int16_t)(((uint16_t)current_rx_buf[0] << 8) | (uint16_t)current_rx_buf[1]);

    current->shunt_voltage = ((float)current->raw_shunt_voltage * INA238_VSHUNT_LSB_UV) / 1000000.0f;
    current->current = (float)current->raw_current * INA238_CURRENT_LSB_A;
}
