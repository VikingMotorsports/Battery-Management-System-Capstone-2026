#include "bms_protocol.h"
#include "can.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(bms_protocol, LOG_LEVEL_INF);

#define BMS_PROTECTION_ID 0x010U
#define BMS_MONITORING_ID 0x250U

#define BMS_PROTECTION_DLC 1U
#define BMS_MONITORING_DLC 32U

#define BMS_PROTECTION_UT_BIT 0U
#define BMS_PROTECTION_OT_BIT 1U
#define BMS_PROTECTION_UV_BIT 2U
#define BMS_PROTECTION_OV_BIT 3U
#define BMS_PROTECTION_OCD_BIT 4U

static void build_protection_frame(const bms_protection_data_t *protection, can_message_t *msg);
static void build_monitoring_frame(const bms_monitoring_data_t *monitoring, can_message_t *msg);
static void put_float32_le(uint8_t *dst, float value);

int bms_transmit(const bms_protection_data_t *protection, const bms_monitoring_data_t *monitoring)
{
    can_message_t msg;
    int ret;

    if ((protection == NULL) || (monitoring == NULL))
    {
        LOG_ERR("invalid BMS transmit args");
        return -EINVAL;
    }

    build_protection_frame(protection, &msg);

    ret = can_transmit(&msg);
    if (ret < 0)
    {
        return ret;
    }

    build_monitoring_frame(monitoring, &msg);

    return can_transmit(&msg);
}

static void build_protection_frame(const bms_protection_data_t *protection, can_message_t *msg)
{
    *msg = (can_message_t){
        .id = BMS_PROTECTION_ID,
        .len = BMS_PROTECTION_DLC,
    };

    if (protection->under_temperature)
    {
        msg->data[0] |= BIT(BMS_PROTECTION_UT_BIT);
    }

    if (protection->over_temperature)
    {
        msg->data[0] |= BIT(BMS_PROTECTION_OT_BIT);
    }

    if (protection->under_voltage)
    {
        msg->data[0] |= BIT(BMS_PROTECTION_UV_BIT);
    }

    if (protection->over_voltage)
    {
        msg->data[0] |= BIT(BMS_PROTECTION_OV_BIT);
    }

    if (protection->over_current_discharge)
    {
        msg->data[0] |= BIT(BMS_PROTECTION_OCD_BIT);
    }
}

static void build_monitoring_frame(const bms_monitoring_data_t *monitoring, can_message_t *msg)
{
    *msg = (can_message_t){
        .id = BMS_MONITORING_ID,
        .len = BMS_MONITORING_DLC,
    };

    msg->data[0] = monitoring->cell_voltage_min_id;
    put_float32_le(&msg->data[1], monitoring->cell_voltage_min);

    msg->data[5] = monitoring->cell_voltage_max_id;
    put_float32_le(&msg->data[6], monitoring->cell_voltage_max);

    msg->data[10] = monitoring->cell_temperature_min_id;
    put_float32_le(&msg->data[11], monitoring->cell_temperature_min);

    msg->data[15] = monitoring->cell_temperature_max_id;
    put_float32_le(&msg->data[16], monitoring->cell_temperature_max);

    put_float32_le(&msg->data[20], monitoring->pack_current);
    put_float32_le(&msg->data[24], monitoring->pack_voltage);
    put_float32_le(&msg->data[28], monitoring->pack_power);
}

static void put_float32_le(uint8_t *dst, float value)
{
    uint32_t raw;

    memcpy(&raw, &value, sizeof(raw));
    sys_put_le32(raw, dst);
}
