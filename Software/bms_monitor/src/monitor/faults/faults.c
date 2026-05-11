#include "faults.h"
#include "protocol.h"
#include "../register_maps/bq79600_regs.h"
#include "../register_maps/bq79616_regs.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(faults, LOG_LEVEL_INF);

#define FAULT_GPIO_NODE DT_PATH(zephyr_user)

#define BRIDGE_ADDR 0x00U
#define ONE_BYTE_NUM_BYTES 1U
#define ONE_BYTE_FRAME_LEN (ONE_BYTE_NUM_BYTES + 6U)

#define BQ79600_NFAULT_EN_VALUE 0x04U
#define BQ79600_FCOMM_EN_VALUE 0x10U

#define BQ79616_FCOMM_EN_VALUE 0x10U

static const struct gpio_dt_spec fault_gpio = GPIO_DT_SPEC_GET(FAULT_GPIO_NODE, fault_gpios);

static struct gpio_callback fault_gpio_callback_data;
static volatile bool fault_pending_flag = false;

static uint8_t parse_one_byte(const uint8_t *rx_buf);
static void fault_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins);

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

    gpio_init_callback(&fault_gpio_callback_data,
                       fault_gpio_callback,
                       BIT(fault_gpio.pin));
    gpio_add_callback(fault_gpio.port, &fault_gpio_callback_data);

    data = BQ79600_NFAULT_EN_VALUE | BQ79600_FCOMM_EN_VALUE;
    ret = write_reg(SINGLE_WRITE, BRIDGE_ADDR, BQ79600_REG_DEV_CONF1, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 DEV_CONF1 write failed: reg=0x%04X err=%d",
                BQ79600_REG_DEV_CONF1, ret);
        return ret;
    }

    data = BQ79616_FCOMM_EN_VALUE;
    ret = write_reg(BROADCAST_WRITE, 0U, BQ79616_REG_DEV_CONF, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79616 DEV_CONF write failed: reg=0x%04X err=%d",
                BQ79616_REG_DEV_CONF, ret);
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
    uint8_t rx_buf[ONE_BYTE_FRAME_LEN];
    int ret;

    if (faults == NULL)
    {
        LOG_ERR("read_faults got NULL output pointer");
        return -EINVAL;
    }

    /* ---------------- BQ79600 bridge fault tree ---------------- */

    ret = read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_SUMMARY,
                   rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 FAULT_SUMMARY read failed: err=%d", ret);
        return ret;
    }
    faults->bridge.summary = parse_one_byte(rx_buf);
    faults->bridge.active = (faults->bridge.summary != 0U);

    ret = read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_COMM1,
                   rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 FAULT_COMM1 read failed: err=%d", ret);
        return ret;
    }
    faults->bridge.fault_comm1 = parse_one_byte(rx_buf);

    ret = read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_COMM2,
                   rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 FAULT_COMM2 read failed: err=%d", ret);
        return ret;
    }
    faults->bridge.fault_comm2 = parse_one_byte(rx_buf);

    ret = read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_REG,
                   rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 FAULT_REG read failed: err=%d", ret);
        return ret;
    }
    faults->bridge.fault_reg = parse_one_byte(rx_buf);

    ret = read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_SYS,
                   rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 FAULT_SYS read failed: err=%d", ret);
        return ret;
    }
    faults->bridge.fault_sys = parse_one_byte(rx_buf);

    ret = read_reg(SINGLE_READ, BRIDGE_ADDR, BQ79600_REG_FAULT_PWR,
                   rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 FAULT_PWR read failed: err=%d", ret);
        return ret;
    }
    faults->bridge.fault_pwr = parse_one_byte(rx_buf);

    /* ---------------- BQ79616 stack fault tree ---------------- */

    for (uint8_t dev = 1U; dev <= NUM_STACK_DEVICES; dev++)
    {
        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_SUMMARY,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_SUMMARY read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].summary = parse_one_byte(rx_buf);
        faults->stack[dev - 1U].active = (faults->stack[dev - 1U].summary != 0U);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_PROT1,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_PROT1 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_prot1 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_PROT2,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_PROT2 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_prot2 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_GPIO,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_GPIO read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_gpio = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_MISC,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_MISC read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_misc = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_VCCB1,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_VCCB1 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_vccb1 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_VCCB2,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_VCCB2 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_vccb2 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_VCOW1,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_VCOW1 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_vcow1 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_VCOW2,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_VCOW2 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_vcow2 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_CBOW1,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_CBOW1 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_cbow1 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_CBOW2,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_CBOW2 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_cbow2 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_CBFET1,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_CBFET1 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_cbfet1 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMP_CBFET2,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMP_CBFET2 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comp_cbfet2 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_OTP,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_OTP read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_otp = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMM1,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMM1 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comm1 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMM2,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMM2 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comm2 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_COMM3,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_COMM3 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_comm3 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_OT,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_OT read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_ot = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_UT,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_UT read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_ut = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_OV1,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_OV1 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_ov1 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_OV2,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_OV2 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_ov2 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_UV1,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_UV1 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_uv1 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_UV2,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_UV2 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_uv2 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_SYS,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_SYS read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_sys = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_PWR1,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_PWR1 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_pwr1 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_PWR2,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_PWR2 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_pwr2 = parse_one_byte(rx_buf);

        ret = read_reg(SINGLE_READ, dev, BQ79616_REG_FAULT_PWR3,
                       rx_buf, sizeof(rx_buf), ONE_BYTE_NUM_BYTES);
        if (ret < 0)
        {
            LOG_ERR("BQ79616 dev %u FAULT_PWR3 read failed: err=%d", dev, ret);
            return ret;
        }
        faults->stack[dev - 1U].fault_pwr3 = parse_one_byte(rx_buf);
    }

    return 0;
}

int clear_faults(void)
{
    int ret;
    uint8_t data = 0xFFU;

    ret = write_reg(SINGLE_WRITE, BRIDGE_ADDR, BQ79600_REG_FAULT_RST, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79600 FAULT_RST write failed: err=%d", ret);
        return ret;
    }

    ret = write_reg(BROADCAST_WRITE, 0U, BQ79616_REG_FAULT_RST1, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79616 FAULT_RST1 write failed: err=%d", ret);
        return ret;
    }

    ret = write_reg(BROADCAST_WRITE, 0U, BQ79616_REG_FAULT_RST2, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BQ79616 FAULT_RST2 write failed: err=%d", ret);
        return ret;
    }

    return 0;
}

static uint8_t parse_one_byte(const uint8_t *rx_buf)
{
    return rx_buf[4];
}

static void fault_gpio_callback(const struct device *port,
                                struct gpio_callback *cb,
                                uint32_t pins)
{
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    fault_pending_flag = true;
}