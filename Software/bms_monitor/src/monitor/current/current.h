#ifndef CURRENT_H
#define CURRENT_H

#include <stdint.h>

typedef struct
{
    int16_t raw_shunt_voltage;
    int16_t raw_current;
    float shunt_voltage;
    float current;
} current_data_t;

int current_init(void);
int read_current(current_data_t *current);

#endif
