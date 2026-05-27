#include "metrics.h"

#include <float.h>
#include <stddef.h>
#include <string.h>

static void calculate_voltage_metrics(const cell_voltage_data_t *voltages, bms_metrics_t *metrics);
static void calculate_temperature_metrics(const temperature_data_t *temperatures, bms_metrics_t *metrics);
static void calculate_current_metrics(const current_data_t *current, bms_metrics_t *metrics);

void calculate_metrics(const cell_voltage_data_t *voltages,
                       const temperature_data_t *temperatures,
                       const current_data_t *current,
                       bms_metrics_t *metrics)
{
    if (metrics == NULL)
    {
        return;
    }

    memset(metrics, 0, sizeof(*metrics));

    calculate_voltage_metrics(voltages, metrics);
    calculate_temperature_metrics(temperatures, metrics);
    calculate_current_metrics(current, metrics);
}

static void calculate_voltage_metrics(const cell_voltage_data_t *voltages, bms_metrics_t *metrics)
{
    float min_voltage = FLT_MAX;
    float max_voltage = -FLT_MAX;

    if (voltages == NULL)
    {
        return;
    }

    for (size_t i = 0; i < NUM_CELLS_PER_MONITOR; i++)
    {
        if (!voltages->cells[i].active)
        {
            continue;
        }

        metrics->pack_voltage_valid = true;
        metrics->pack_voltage += voltages->cells[i].voltage;

        if (voltages->cells[i].voltage < min_voltage)
        {
            min_voltage = voltages->cells[i].voltage;
            metrics->cell_voltage_min.valid = true;
            metrics->cell_voltage_min.id = (uint8_t)(i + 1U);
            metrics->cell_voltage_min.value = voltages->cells[i].voltage;
        }

        if (voltages->cells[i].voltage > max_voltage)
        {
            max_voltage = voltages->cells[i].voltage;
            metrics->cell_voltage_max.valid = true;
            metrics->cell_voltage_max.id = (uint8_t)(i + 1U);
            metrics->cell_voltage_max.value = voltages->cells[i].voltage;
        }
    }
}

static void calculate_temperature_metrics(const temperature_data_t *temperatures, bms_metrics_t *metrics)
{
    float min_temperature = FLT_MAX;
    float max_temperature = -FLT_MAX;

    if (temperatures == NULL)
    {
        return;
    }

    for (size_t i = 0; i < NUM_THERMISTORS_PER_MONITOR; i++)
    {
        if (!temperatures->thermistors[i].active)
        {
            continue;
        }

        if (temperatures->thermistors[i].temperature_c < min_temperature)
        {
            min_temperature = temperatures->thermistors[i].temperature_c;
            metrics->cell_temperature_min.valid = true;
            metrics->cell_temperature_min.id = (uint8_t)(i + 1U);
            metrics->cell_temperature_min.value = temperatures->thermistors[i].temperature_c;
        }

        if (temperatures->thermistors[i].temperature_c > max_temperature)
        {
            max_temperature = temperatures->thermistors[i].temperature_c;
            metrics->cell_temperature_max.valid = true;
            metrics->cell_temperature_max.id = (uint8_t)(i + 1U);
            metrics->cell_temperature_max.value = temperatures->thermistors[i].temperature_c;
        }
    }
}

static void calculate_current_metrics(const current_data_t *current, bms_metrics_t *metrics)
{
    if (current == NULL)
    {
        return;
    }

    metrics->pack_current_valid = true;
    metrics->pack_current = current->current;

    if (metrics->pack_voltage_valid)
    {
        metrics->pack_power_valid = true;
        metrics->pack_power = metrics->pack_voltage * metrics->pack_current;
    }
}
