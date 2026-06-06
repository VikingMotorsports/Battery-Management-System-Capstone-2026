#include "bms_protocol.h"
#include "can.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(bms_protocol, LOG_LEVEL_INF);

#define BMS_PROTECTION_UT_BIT 0U
#define BMS_PROTECTION_OT_BIT 1U
#define BMS_PROTECTION_UV_BIT 2U
#define BMS_PROTECTION_OV_BIT 3U
#define BMS_PROTECTION_OCD_BIT 4U

static void build_protection_frame(const bms_protection_data_t *protection, can_message_t *msg);
static void build_id_float_frame(uint32_t frame_id, uint8_t signal_id, float value, can_message_t *msg);
static void build_pack_electrical_frame(const bms_monitoring_data_t *monitoring, can_message_t *msg);
static void build_pack_power_frame(const bms_monitoring_data_t *monitoring, can_message_t *msg);
static int transmit_monitoring_frames(const bms_monitoring_data_t *monitoring);
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

    return transmit_monitoring_frames(monitoring);
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

static int transmit_monitoring_frames(const bms_monitoring_data_t *monitoring)
{
    can_message_t msg;
    int ret;

    build_id_float_frame(BMS_CELL_VOLTAGE_MIN_ID,
                         monitoring->cell_voltage_min_id,
                         monitoring->cell_voltage_min,
                         &msg);
    ret = can_transmit(&msg);
    if (ret < 0)
    {
        return ret;
    }

    build_id_float_frame(BMS_CELL_VOLTAGE_MAX_ID,
                         monitoring->cell_voltage_max_id,
                         monitoring->cell_voltage_max,
                         &msg);
    ret = can_transmit(&msg);
    if (ret < 0)
    {
        return ret;
    }

    build_id_float_frame(BMS_CELL_TEMPERATURE_MIN_ID,
                         monitoring->cell_temperature_min_id,
                         monitoring->cell_temperature_min,
                         &msg);
    ret = can_transmit(&msg);
    if (ret < 0)
    {
        return ret;
    }

    build_id_float_frame(BMS_CELL_TEMPERATURE_MAX_ID,
                         monitoring->cell_temperature_max_id,
                         monitoring->cell_temperature_max,
                         &msg);
    ret = can_transmit(&msg);
    if (ret < 0)
    {
        return ret;
    }

    build_pack_electrical_frame(monitoring, &msg);
    ret = can_transmit(&msg);
    if (ret < 0)
    {
        return ret;
    }

    build_pack_power_frame(monitoring, &msg);
    return can_transmit(&msg);
}

static void build_id_float_frame(uint32_t frame_id, uint8_t signal_id, float value, can_message_t *msg)
{
    *msg = (can_message_t){
        .id = frame_id,
        .len = BMS_ID_FLOAT_DLC,
    };

    msg->data[0] = signal_id;
    put_float32_le(&msg->data[1], value);
}

static void build_pack_electrical_frame(const bms_monitoring_data_t *monitoring, can_message_t *msg)
{
    *msg = (can_message_t){
        .id = BMS_PACK_ELECTRICAL_ID,
        .len = BMS_PACK_ELECTRICAL_DLC,
    };

    put_float32_le(&msg->data[0], monitoring->pack_current);
    put_float32_le(&msg->data[4], monitoring->pack_voltage);
}

static void build_pack_power_frame(const bms_monitoring_data_t *monitoring, can_message_t *msg)
{
    *msg = (can_message_t){
        .id = BMS_PACK_POWER_ID,
        .len = BMS_PACK_POWER_DLC,
    };

    put_float32_le(&msg->data[0], monitoring->pack_power);
}

static void put_float32_le(uint8_t *dst, float value)
{
    uint32_t raw;

    memcpy(&raw, &value, sizeof(raw));
    sys_put_le32(raw, dst);
}
