#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "faults.h"
#include "metrics.h"

int telemetry_transmit(const bms_metrics_t *metrics, const fault_data_t *faults);

#endif
