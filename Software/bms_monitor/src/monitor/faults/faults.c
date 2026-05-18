#include "faults.h"
#include "bq796xx_protocol.h"
#include "../register_maps/bq796xx_regs.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

LOG_MODULE_REGISTER(faults, LOG_LEVEL_INF);

#define FAULT_GPIO_NODE DT_PATH(zephyr_user)

#define BRIDGE_ADDR 0x00U
#define ONE_BYTE_NUM_BYTES 1U
#define ONE_BYTE_FRAME_LEN (ONE_BYTE_NUM_BYTES + 6U)

#define BQ79600_NFAULT_EN_VALUE 0x04U
#define BQ796XX_FCOMM_EN_VALUE 0x10U

#define BQ79600_FAULT_SUMMARY_COMM_MASK 0x08U
#define BQ79600_FAULT_SUMMARY_REG_MASK 0x04U
#define BQ79600_FAULT_SUMMARY_SYS_MASK 0x02U
#define BQ79600_FAULT_SUMMARY_PWR_MASK 0x01U

#define BQ79616_FAULT_SUMMARY_PROT_MASK 0x80U
#define BQ79616_FAULT_SUMMARY_COMP_ADC_MASK 0x40U
#define BQ79616_FAULT_SUMMARY_OTP_MASK 0x20U
#define BQ79616_FAULT_SUMMARY_COMM_MASK 0x10U
#define BQ79616_FAULT_SUMMARY_OTUT_MASK 0x08U
#define BQ79616_FAULT_SUMMARY_OVUV_MASK 0x04U
#define BQ79616_FAULT_SUMMARY_SYS_MASK 0x02U
#define BQ79616_FAULT_SUMMARY_PWR_MASK 0x01U

static const struct gpio_dt_spec fault_gpio = GPIO_DT_SPEC_GET(FAULT_GPIO_NODE, fault_gpios);

static struct gpio_callback fault_gpio_callback_data;
static volatile bool fault_pending_flag = false;

static void fault_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins);
static int read_bridge_faults(bridge_fault_data_t *bridge_faults);
static int read_stack_faults(stack_fault_data_t *stack_faults);

int faults_init(void)
{
    int ret;
    uint8_t data;

    if (!gpio_is_ready_dt(&fault_gpio))
    {
        LOG_ERR("fault GPIO device not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&fault_gpio, GPIO_INPUT);
    if (ret < 0)
    {
        LOG_ERR("fault GPIO configure failed: %d", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&fault_gpio, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0)
    {
        LOG_ERR("fault GPIO interrupt configure failed: %d", ret);
        return ret;
    }

    gpio_init_callback(&fault_gpio_callback_data, fault_gpio_callback, BIT(fault_gpio.pin));
    gpio_add_callback(fault_gpio.port, &fault_gpio_callback_data);

    data = BQ79600_NFAULT_EN_VALUE | BQ796XX_FCOMM_EN_VALUE;
    ret = bq796xx_write_reg(SINGLE_WRITE, BRIDGE_ADDR, BQ79600_REG_DEV_CONF1, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 DEV_CONF1 write failed: reg=0x%04X err=%d", BQ79600_REG_DEV_CONF1, ret);
        return ret;
    }

    data = BQ796XX_FCOMM_EN_VALUE;
    ret = bq796xx_write_reg(STACK_WRITE, 0U, BQ79616_REG_DEV_CONF, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79616 DEV_CONF write failed: reg=0x%04X err=%d", BQ79616_REG_DEV_CONF, ret);
        return ret;
    }

    LOG_INF("fault interrupt and signaling initialized");
    return 0;
}

bool faults_pending(void)
{
    return fault_pending_flag;
}

void faults_clear_pending(void)
{
    fault_pending_flag = false;
}

int read_faults(fault_data_t *faults)
{
    int ret;

    if (faults == NULL)
    {
        LOG_ERR("read_faults got NULL output pointer");
        return -EINVAL;
    }

    memset(faults, 0, sizeof(*faults));

    ret = read_bridge_faults(&faults->bridge);
    if (ret < 0)
    {
        return ret;
    }

    ret = read_stack_faults(faults->stack);
    if (ret < 0)
    {
        return ret;
    }

    return 0;
}

static int read_bridge_faults(bridge_fault_data_t *bridge_faults)
{
    uint8_t rx_buf[10];
    int ret;

    ret = bq796xx_read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_SUMMARY, rx_buf, sizeof(rx_buf), 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 FAULT_SUMMARY read failed err=%d", ret);
        return ret;
    }
    bridge_faults->fault_summary = rx_buf[4];
    bridge_faults->active = (bridge_faults->fault_summary != 0U);

    if ((bridge_faults->fault_summary & BQ79600_FAULT_SUMMARY_COMM_MASK) != 0U)
    {
        ret = bq796xx_read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_COMM1, rx_buf, sizeof(rx_buf), 2U);
        if (ret < 0)
        {
            LOG_ERR("BQ79600 FAULT_COMM1..2 read failed err=%d", ret);
            return ret;
        }
        bridge_faults->fault_comm1 = rx_buf[4];
        bridge_faults->fault_comm2 = rx_buf[5];
    }

    if ((bridge_faults->fault_summary & BQ79600_FAULT_SUMMARY_REG_MASK) != 0U)
    {
        ret = bq796xx_read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_REG, rx_buf, sizeof(rx_buf), 1U);
        if (ret < 0)
        {
            LOG_ERR("BQ79600 FAULT_REG read failed err=%d", ret);
            return ret;
        }
        bridge_faults->fault_reg = rx_buf[4];
    }

    if ((bridge_faults->fault_summary & BQ79600_FAULT_SUMMARY_SYS_MASK) != 0U)
    {
        ret = bq796xx_read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_SYS, rx_buf, sizeof(rx_buf), 1U);
        if (ret < 0)
        {
            LOG_ERR("BQ79600 FAULT_SYS read failed err=%d", ret);
            return ret;
        }
        bridge_faults->fault_sys = rx_buf[4];
    }

    if ((bridge_faults->fault_summary & BQ79600_FAULT_SUMMARY_PWR_MASK) != 0U)
    {
        ret = bq796xx_read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_PWR, rx_buf, sizeof(rx_buf), 1U);
        if (ret < 0)
        {
            LOG_ERR("BQ79600 FAULT_PWR read failed err=%d", ret);
            return ret;
        }
        bridge_faults->fault_pwr = rx_buf[4];
    }

    return 0;
}

static int read_stack_faults(stack_fault_data_t *stack_faults)
{
    uint8_t rx_buf[12];
    int ret;

    for (uint8_t dev = 1U; dev <= NUM_STACK_DEVICES; dev++)
    {
        ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_SUMMARY, rx_buf, sizeof(rx_buf), 1U);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_SUMMARY read failed err=%d", dev, ret);
            return ret;
        }
        stack_faults[dev - 1U].fault_summary = rx_buf[4];
        stack_faults[dev - 1U].active = (stack_faults[dev - 1U].fault_summary != 0U);

        if ((stack_faults[dev - 1U].fault_summary & BQ79616_FAULT_SUMMARY_PROT_MASK) != 0U)
        {
            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_PROT1, rx_buf, sizeof(rx_buf), 2U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_PROT1..2 read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_prot1 = rx_buf[4];
            stack_faults[dev - 1U].fault_prot2 = rx_buf[5];
        }

        if ((stack_faults[dev - 1U].fault_summary & BQ79616_FAULT_SUMMARY_COMP_ADC_MASK) != 0U)
        {
            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_VCCB1, rx_buf, sizeof(rx_buf), 2U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_COMP_VCCB1..2 read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_comp_vccb1 = rx_buf[4];
            stack_faults[dev - 1U].fault_comp_vccb2 = rx_buf[5];

            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_VCOW1, rx_buf, sizeof(rx_buf), 2U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_COMP_VCOW1..2 read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_comp_vcow1 = rx_buf[4];
            stack_faults[dev - 1U].fault_comp_vcow2 = rx_buf[5];

            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_CBOW1, rx_buf, sizeof(rx_buf), 2U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_COMP_CBOW1..2 read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_comp_cbow1 = rx_buf[4];
            stack_faults[dev - 1U].fault_comp_cbow2 = rx_buf[5];

            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_CBFET1, rx_buf, sizeof(rx_buf), 2U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_COMP_CBFET1..2 read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_comp_cbfet1 = rx_buf[4];
            stack_faults[dev - 1U].fault_comp_cbfet2 = rx_buf[5];

            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_GPIO, rx_buf, sizeof(rx_buf), 1U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_COMP_GPIO read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_comp_gpio = rx_buf[4];

            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_MISC, rx_buf, sizeof(rx_buf), 1U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_COMP_MISC read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_comp_misc = rx_buf[4];
        }

        if ((stack_faults[dev - 1U].fault_summary & BQ79616_FAULT_SUMMARY_OTP_MASK) != 0U)
        {
            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_OTP, rx_buf, sizeof(rx_buf), 1U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_OTP read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_otp = rx_buf[4];
        }

        if ((stack_faults[dev - 1U].fault_summary & BQ79616_FAULT_SUMMARY_COMM_MASK) != 0U)
        {
            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMM1, rx_buf, sizeof(rx_buf), 3U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_COMM1..3 read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_comm1 = rx_buf[4];
            stack_faults[dev - 1U].fault_comm2 = rx_buf[5];
            stack_faults[dev - 1U].fault_comm3 = rx_buf[6];
        }

        if ((stack_faults[dev - 1U].fault_summary & BQ79616_FAULT_SUMMARY_OTUT_MASK) != 0U)
        {
            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_OT, rx_buf, sizeof(rx_buf), 2U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_OT..UT read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_ot = rx_buf[4];
            stack_faults[dev - 1U].fault_ut = rx_buf[5];
        }

        if ((stack_faults[dev - 1U].fault_summary & BQ79616_FAULT_SUMMARY_OVUV_MASK) != 0U)
        {
            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_OV1, rx_buf, sizeof(rx_buf), 4U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_OV1..UV2 read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_ov1 = rx_buf[4];
            stack_faults[dev - 1U].fault_ov2 = rx_buf[5];
            stack_faults[dev - 1U].fault_uv1 = rx_buf[6];
            stack_faults[dev - 1U].fault_uv2 = rx_buf[7];
        }

        if ((stack_faults[dev - 1U].fault_summary & BQ79616_FAULT_SUMMARY_SYS_MASK) != 0U)
        {
            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_SYS, rx_buf, sizeof(rx_buf), 1U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_SYS read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_sys = rx_buf[4];
        }

        if ((stack_faults[dev - 1U].fault_summary & BQ79616_FAULT_SUMMARY_PWR_MASK) != 0U)
        {
            ret = bq796xx_read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_PWR1, rx_buf, sizeof(rx_buf), 3U);
            if (ret < 0)
            {
                LOG_ERR("BQ79616 dev %u FAULT_PWR1..3 read failed: err=%d", dev, ret);
                return ret;
            }
            stack_faults[dev - 1U].fault_pwr1 = rx_buf[4];
            stack_faults[dev - 1U].fault_pwr2 = rx_buf[5];
            stack_faults[dev - 1U].fault_pwr3 = rx_buf[6];
        }
    }

    return 0;
}

int clear_faults(void)
{
    int ret;
    uint8_t data = 0xFFU;

    ret = bq796xx_write_reg(SINGLE_WRITE, BRIDGE_ADDR, BQ79600_REG_FAULT_RST, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 FAULT_RST write failed: err=%d", ret);
        return ret;
    }

    ret = bq796xx_write_reg(STACK_WRITE, 0U, BQ79616_REG_FAULT_RST1, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79616 FAULT_RST1 write failed: err=%d", ret);
        return ret;
    }

    ret = bq796xx_write_reg(STACK_WRITE, 0U, BQ79616_REG_FAULT_RST2, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79616 FAULT_RST2 write failed: err=%d", ret);
        return ret;
    }

    return 0;
}

static void fault_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    fault_pending_flag = true;
}
