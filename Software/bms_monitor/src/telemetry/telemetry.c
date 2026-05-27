#include "telemetry.h"
#include "bms_protocol.h"

#include <zephyr/logging/log.h>
#include <stddef.h>

LOG_MODULE_REGISTER(telemetry, LOG_LEVEL_INF);

static void build_monitoring_data(const bms_metrics_t *metrics, bms_monitoring_data_t *monitoring);
static void build_protection_data(const fault_data_t *faults, bms_protection_data_t *protection);

int telemetry_transmit(const bms_metrics_t *metrics, const fault_data_t *faults)
{
    bms_monitoring_data_t monitoring = {0};
    bms_protection_data_t protection = {0};

    build_protection_data(faults, &protection);
    build_monitoring_data(metrics, &monitoring);

    return bms_transmit(&protection, &monitoring);
}

static void build_monitoring_data(const bms_metrics_t *metrics,
                                  bms_monitoring_data_t *monitoring)
{
    if (metrics == NULL)
    {
        return;
    }

    if (metrics->cell_voltage_min.valid)
    {
        monitoring->cell_voltage_min_id = metrics->cell_voltage_min.id;
        monitoring->cell_voltage_min = metrics->cell_voltage_min.value;
    }

    if (metrics->cell_voltage_max.valid)
    {
        monitoring->cell_voltage_max_id = metrics->cell_voltage_max.id;
        monitoring->cell_voltage_max = metrics->cell_voltage_max.value;
    }

    if (metrics->cell_temperature_min.valid)
    {
        monitoring->cell_temperature_min_id = metrics->cell_temperature_min.id;
        monitoring->cell_temperature_min = metrics->cell_temperature_min.value;
    }

    if (metrics->cell_temperature_max.valid)
    {
        monitoring->cell_temperature_max_id = metrics->cell_temperature_max.id;
        monitoring->cell_temperature_max = metrics->cell_temperature_max.value;
    }

    if (metrics->pack_current_valid)
    {
        monitoring->pack_current = metrics->pack_current;
    }

    if (metrics->pack_voltage_valid)
    {
        monitoring->pack_voltage = metrics->pack_voltage;
    }

    if (metrics->pack_power_valid)
    {
        monitoring->pack_power = metrics->pack_power;
    }
}

static void build_protection_data(const fault_data_t *faults, bms_protection_data_t *protection)
{
    if (faults == NULL)
    {
        return;
    }

    for (size_t i = 0; i < NUM_STACK_DEVICES; i++)
    {
        protection->over_temperature |= faults->stack[i].fault_ot != 0U;
        protection->under_temperature |= faults->stack[i].fault_ut != 0U;
        protection->over_voltage |= (faults->stack[i].fault_ov1 != 0U) || (faults->stack[i].fault_ov2 != 0U);
        protection->under_voltage |= (faults->stack[i].fault_uv1 != 0U) || (faults->stack[i].fault_uv2 != 0U);
    }

    protection->over_current_discharge = faults->ina.shunt_over_limit;
}
