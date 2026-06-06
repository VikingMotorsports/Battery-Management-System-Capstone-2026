#ifndef BMS_PROTOCOL_H
#define BMS_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define BMS_PROTECTION_ID 0x010U
#define BMS_MONITORING_ID 0x250U

#define BMS_PROTECTION_DLC 1U
#define BMS_MONITORING_DLC 32U

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
