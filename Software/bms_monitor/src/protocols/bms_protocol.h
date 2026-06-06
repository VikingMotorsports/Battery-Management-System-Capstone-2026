#ifndef BMS_PROTOCOL_H
#define BMS_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define BMS_PROTECTION_ID 0x010U
#define BMS_CELL_VOLTAGE_MIN_ID 0x250U
#define BMS_CELL_VOLTAGE_MAX_ID 0x251U
#define BMS_CELL_TEMPERATURE_MIN_ID 0x252U
#define BMS_CELL_TEMPERATURE_MAX_ID 0x253U
#define BMS_PACK_ELECTRICAL_ID 0x254U
#define BMS_PACK_POWER_ID 0x255U

#define BMS_PROTECTION_DLC 1U
#define BMS_ID_FLOAT_DLC 5U
#define BMS_PACK_ELECTRICAL_DLC 8U
#define BMS_PACK_POWER_DLC 4U

typedef struct
{
    bool under_temperature;
    bool over_temperature;
    bool under_voltage;
    bool over_voltage;
    bool over_current_discharge;
} bms_protection_data_t;

typedef struct
{
    uint8_t cell_voltage_min_id;
    float cell_voltage_min;

    uint8_t cell_voltage_max_id;
    float cell_voltage_max;

    uint8_t cell_temperature_min_id;
    float cell_temperature_min;

    uint8_t cell_temperature_max_id;
    float cell_temperature_max;

    float pack_current;
    float pack_voltage;
    float pack_power;
} bms_monitoring_data_t;

int bms_transmit(const bms_protection_data_t *protection, const bms_monitoring_data_t *monitoring);

#endif
