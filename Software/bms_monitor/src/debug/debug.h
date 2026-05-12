#ifndef DEBUG_H
#define DEBUG_H

#include "monitor/faults/faults.h"
#include "monitor/voltage/voltage.h"
#include "monitor/temperature/temperature.h"

void debug_print_voltages(const cell_voltage_data_t *voltages);
void debug_print_temperatures(const temperature_data_t *temperatures);
void debug_print_faults(const fault_data_t *faults);

#endif