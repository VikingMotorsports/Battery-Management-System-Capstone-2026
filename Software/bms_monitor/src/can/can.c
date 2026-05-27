#include "can.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

LOG_MODULE_REGISTER(can, LOG_LEVEL_INF);

#define CAN_NODE DT_NODELABEL(fdcan1)
#define CAN_BUS_BITRATE 500000U
#define CAN_BUS_TX_TIMEOUT_MS 100U

static const struct device *can_dev = DEVICE_DT_GET(CAN_NODE);
static bool can_started = false;

int can_init(void)
{
    int ret;

    if (can_started)
    {
        return 0;
    }

    if (!device_is_ready(can_dev))
    {
        LOG_ERR("CAN device not ready");
        return -ENODEV;
    }

    ret = can_set_bitrate(can_dev, CAN_BUS_BITRATE);
    if (ret < 0)
    {
        LOG_ERR("can_set_bitrate failed: %d", ret);
        return ret;
    }

    ret = can_set_bitrate_data(can_dev, CAN_BUS_BITRATE);
    if (ret < 0)
    {
        LOG_ERR("can_set_bitrate_data failed: %d", ret);
        return ret;
    }

    ret = can_set_mode(can_dev, CAN_MODE_FD);
    if (ret < 0)
    {
        LOG_ERR("can_set_mode(CAN_MODE_FD) failed: %d", ret);
        return ret;
    }

    ret = can_start(can_dev);
    if (ret < 0)
    {
        LOG_ERR("can_start failed: %d", ret);
        return ret;
    }

    can_started = true;

    return 0;
}

int can_transmit(const can_message_t *msg)
{
    struct can_frame frame = {0};
    int ret;

    if (msg == NULL)
    {
        LOG_ERR("invalid CAN transmit args");
        return -EINVAL;
    }

    if (msg->len > CAN_MAX_DLEN)
    {
        LOG_ERR("CAN FD payload too large: len=%u", (unsigned int)msg->len);
        return -EMSGSIZE;
    }

    frame.id = msg->id;
    frame.dlc = can_bytes_to_dlc(msg->len);

    frame.flags |= CAN_FRAME_FDF;

    if (msg->len > 0U)
    {
        memcpy(frame.data, msg->data, msg->len);
    }

    ret = can_send(can_dev, &frame, K_MSEC(CAN_BUS_TX_TIMEOUT_MS), NULL, NULL);
    if (ret < 0)
    {
        LOG_ERR("can_send failed: id=0x%03x len=%u err=%d", msg->id, (unsigned int)msg->len, ret);
        return ret;
    }

    return 0;
}
