#ifndef FAULTS_H
#define FAULTS_H

#include <stdbool.h>
#include <stdint.h>

#define NUM_STACK_DEVICES 1U

typedef struct
{
    uint8_t fault_summary;

    uint8_t fault_comm1;
    uint8_t fault_comm2;

    uint8_t fault_reg;
    uint8_t fault_sys;
    uint8_t fault_pwr;

    bool active;
} bridge_fault_data_t;

typedef struct
{
    uint8_t fault_summary;

    uint8_t fault_prot1;
    uint8_t fault_prot2;

    uint8_t fault_comp_vccb1;
    uint8_t fault_comp_vccb2;
    uint8_t fault_comp_vcow1;
    uint8_t fault_comp_vcow2;
    uint8_t fault_comp_cbow1;
    uint8_t fault_comp_cbow2;
    uint8_t fault_comp_cbfet1;
    uint8_t fault_comp_cbfet2;
    uint8_t fault_comp_gpio;
    uint8_t fault_comp_misc;

    uint8_t fault_otp;

    uint8_t fault_comm1;
    uint8_t fault_comm2;
    uint8_t fault_comm3;

    uint8_t fault_ot;
    uint8_t fault_ut;

    uint8_t fault_ov1;
    uint8_t fault_ov2;
    uint8_t fault_uv1;
    uint8_t fault_uv2;

    uint8_t fault_sys;

    uint8_t fault_pwr1;
    uint8_t fault_pwr2;
    uint8_t fault_pwr3;

    bool active;
} stack_fault_data_t;

typedef struct
{
    bridge_fault_data_t bridge;
    stack_fault_data_t stack[NUM_STACK_DEVICES];
} fault_data_t;

int faults_init(void);
bool faults_pending(void);
void faults_clear_pending(void);
int read_faults(fault_data_t *faults);
int clear_faults(void);

#endif