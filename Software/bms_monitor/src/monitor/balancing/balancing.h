#ifndef BALANCING_H
#define BALANCING_H

#include "voltage.h"

#include <stdint.h>
#include <stdbool.h>

#define NUM_BALANCE_CELLS 6U

typedef enum
{
    BALANCING_STATE_IDLE = 0,
    BALANCING_STATE_RUNNING,
    BALANCING_STATE_COMPLETE,
} balancing_state_t;

typedef struct
{
    uint16_t selected_cells;
    uint16_t complete_cells;
    bool active;
    balancing_state_t state;
} balancing_data_t;

int balancing_init(void);
int balancing_update(const cell_voltage_data_t *voltages, balancing_data_t *balancing);
int balancing_stop(void);

#endif
