#ifndef DEBUG_H
#define DEBUG_H

#include "monitor/faults/faults.h"
#include "monitor/voltage/voltage.h"
#include "monitor/temperature/temperature.h"
#include "monitor/current/current.h"
#include "monitor/balancing/balancing.h"
#include "metrics.h"

void debug_print_voltages(const cell_voltage_data_t *voltages);
void debug_print_temperatures(const temperature_data_t *temperatures);
void debug_print_current(const current_data_t *current);
void debug_print_faults(const fault_data_t *faults);
void debug_print_telemetry(const bms_metrics_t *metrics, const fault_data_t *faults);

void debug_print_balancing(const cell_voltage_data_t *voltages,
                           const balancing_data_t *balancing,
                           balancing_state_t previous_state,
                           uint16_t previous_selected_cells);

#endif
