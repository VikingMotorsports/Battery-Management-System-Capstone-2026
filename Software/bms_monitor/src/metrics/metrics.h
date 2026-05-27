#ifndef METRICS_H
#define METRICS_H

#include "current.h"
#include "temperature.h"
#include "voltage.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool valid;
    uint8_t id;
    float value;
} metric_extreme_t;

typedef struct
{
    metric_extreme_t cell_voltage_min;
    metric_extreme_t cell_voltage_max;
    metric_extreme_t cell_temperature_min;
    metric_extreme_t cell_temperature_max;

    bool pack_current_valid;
    float pack_current;

    bool pack_voltage_valid;
    float pack_voltage;

    bool pack_power_valid;
    float pack_power;
} bms_metrics_t;

void calculate_metrics(const cell_voltage_data_t *voltages,
                       const temperature_data_t *temperatures,
                       const current_data_t *current,
                       bms_metrics_t *metrics);

#endif
