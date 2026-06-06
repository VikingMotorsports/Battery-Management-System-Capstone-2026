#include "debug.h"

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(CONFIG_APP_FAULT_MONITORING) && \
    (defined(CONFIG_APP_VOLTAGE_MONITORING) || defined(CONFIG_APP_TEMPERATURE_MONITORING))
#define APP_DEBUG_BQ796XX_FAULTS 1
#endif

#if defined(CONFIG_APP_FAULT_MONITORING) && defined(CONFIG_APP_CURRENT_MONITORING)
#define APP_DEBUG_INA238_FAULTS 1
#endif

static void print_float_6(float value)
{
    bool negative = value < 0.0f;

    if (negative)
    {
        value = -value;
    }

    int whole = (int)value;
    int frac = (int)((value - (float)whole) * 1000000.0f);

    printk("%s%d.%06d", negative ? "-" : "", whole, frac);
}

static void print_float_3(float value)
{
    bool negative = value < 0.0f;

    if (negative)
    {
        value = -value;
    }

    int whole = (int)value;
    int frac = (int)((value - (float)whole) * 1000.0f);

    printk("%s%d.%03d", negative ? "-" : "", whole, frac);
}

static void print_float_1(float value)
{
    bool negative = value < 0.0f;

    if (negative)
    {
        value = -value;
    }

    int whole = (int)value;
    int frac = (int)((value - (float)whole) * 10.0f);

    printk("%s%d.%1d", negative ? "-" : "", whole, frac);
}

void debug_print_voltages(const cell_voltage_data_t *voltages)
{
#if defined(CONFIG_APP_DEBUG_PRINTS) && defined(CONFIG_APP_DEBUG_VOLTAGES)
    if (voltages == NULL)
    {
        printk("Voltages got null pointer\n");
        return;
    }

    for (int i = 0; i < NUM_CELLS_PER_MONITOR; i++)
    {
        if (!voltages->cells[i].active)
        {
            continue;
        }

        printk("Cell %2d: raw=0x%04X voltage=", i + 1, (uint16_t)voltages->cells[i].raw);

        print_float_6(voltages->cells[i].voltage);
        printk(" V\n");
    }

    printk("\n");
#else
    ARG_UNUSED(voltages);
#endif
}

void debug_print_temperatures(const temperature_data_t *temperatures)
{
#if defined(CONFIG_APP_DEBUG_PRINTS) && defined(CONFIG_APP_DEBUG_TEMPERATURES)
    if (temperatures == NULL)
    {
        printk("Temperatures got null pointer\n");
        return;
    }

    for (int i = 0; i < NUM_THERMISTORS_PER_MONITOR; i++)
    {
        if (!temperatures->thermistors[i].active)
        {
            continue;
        }

        printk("Thermistor %d: raw_gpio=0x%04X raw_tsref=0x%04X gpio=", i + 1, temperatures->thermistors[i].raw_gpio, temperatures->thermistors[i].raw_tsref);

        print_float_6(temperatures->thermistors[i].gpio_volts);
        printk(" V tsref=");

        print_float_6(temperatures->thermistors[i].tsref_volts);
        printk(" V Rntc=");

        print_float_1(temperatures->thermistors[i].thermistor_ohms);
        printk(" ohm T=");

        print_float_3(temperatures->thermistors[i].temperature_c);
        printk(" C\n");
    }

    printk("\n");
#else
    ARG_UNUSED(temperatures);
#endif
}

void debug_print_current(const current_data_t *current)
{
#if defined(CONFIG_APP_DEBUG_PRINTS) && defined(CONFIG_APP_DEBUG_CURRENT)
    if (current == NULL)
    {
        printk("Current got null pointer\n");
        return;
    }

    printk("Current: raw=0x%04X vshunt=", (uint16_t)current->raw_shunt_voltage);
    print_float_6(current->shunt_voltage);
    printk(" V raw=0x%04X current=", (uint16_t)current->raw_current);
    print_float_6(current->current);
    printk(" A\n\n");
#else
    ARG_UNUSED(current);
#endif
}

void debug_print_faults(const fault_data_t *faults)
{
#if defined(CONFIG_APP_DEBUG_PRINTS) && defined(CONFIG_APP_DEBUG_FAULTS)
    if (faults == NULL)
    {
        printk("Faults got null pointer\n");
        return;
    }

#if defined(APP_DEBUG_BQ796XX_FAULTS)
    if (faults->bridge.fault_summary != 0U)
    {
        printk("Bridge fault summary: 0x%02X\n", faults->bridge.fault_summary);
    }

    if (faults->bridge.fault_comm1 != 0U)
    {
        printk("\tCOMM1: 0x%02X\n", faults->bridge.fault_comm1);
    }

    if (faults->bridge.fault_comm2 != 0U)
    {
        printk("\tCOMM2: 0x%02X\n", faults->bridge.fault_comm2);
    }

    if (faults->bridge.fault_reg != 0U)
    {
        printk("\tREG:   0x%02X\n", faults->bridge.fault_reg);
    }

    if (faults->bridge.fault_sys != 0U)
    {
        printk("\tSYS:   0x%02X\n", faults->bridge.fault_sys);
    }

    if (faults->bridge.fault_pwr != 0U)
    {
        printk("\tPWR:   0x%02X\n", faults->bridge.fault_pwr);
    }

    for (int i = 0; i < NUM_STACK_DEVICES; i++)
    {
        if (faults->stack[i].fault_summary != 0U)
        {
            printk("Stack device %d summary: 0x%02X\n", i + 1, faults->stack[i].fault_summary);
        }

        if (faults->stack[i].fault_prot1 != 0U)
        {
            printk("\tPROT1: 0x%02X\n", faults->stack[i].fault_prot1);
        }

        if (faults->stack[i].fault_prot2 != 0U)
        {
            printk("\tPROT2: 0x%02X\n", faults->stack[i].fault_prot2);
        }

        if (faults->stack[i].fault_comp_gpio != 0U)
        {
            printk("\tCOMP_GPIO: 0x%02X\n", faults->stack[i].fault_comp_gpio);
        }

        if (faults->stack[i].fault_comp_misc != 0U)
        {
            printk("\tCOMP_MISC: 0x%02X\n", faults->stack[i].fault_comp_misc);
        }

        if (faults->stack[i].fault_comp_vccb1 != 0U)
        {
            printk("\tCOMP_VCCB1: 0x%02X\n", faults->stack[i].fault_comp_vccb1);
        }

        if (faults->stack[i].fault_comp_vccb2 != 0U)
        {
            printk("\tCOMP_VCCB2: 0x%02X\n", faults->stack[i].fault_comp_vccb2);
        }

        if (faults->stack[i].fault_comp_vcow1 != 0U)
        {
            printk("\tCOMP_VCOW1: 0x%02X\n", faults->stack[i].fault_comp_vcow1);
        }

        if (faults->stack[i].fault_comp_vcow2 != 0U)
        {
            printk("\tCOMP_VCOW2: 0x%02X\n", faults->stack[i].fault_comp_vcow2);
        }

        if (faults->stack[i].fault_comp_cbow1 != 0U)
        {
            printk("\tCOMP_CBOW1: 0x%02X\n", faults->stack[i].fault_comp_cbow1);
        }

        if (faults->stack[i].fault_comp_cbow2 != 0U)
        {
            printk("\tCOMP_CBOW2: 0x%02X\n", faults->stack[i].fault_comp_cbow2);
        }

        if (faults->stack[i].fault_comp_cbfet1 != 0U)
        {
            printk("\tCOMP_CBFET1: 0x%02X\n", faults->stack[i].fault_comp_cbfet1);
        }

        if (faults->stack[i].fault_comp_cbfet2 != 0U)
        {
            printk("\tCOMP_CBFET2: 0x%02X\n", faults->stack[i].fault_comp_cbfet2);
        }

        if (faults->stack[i].fault_otp != 0U)
        {
            printk("\tOTP: 0x%02X\n", faults->stack[i].fault_otp);
        }

        if (faults->stack[i].fault_comm1 != 0U)
        {
            printk("\tCOMM1: 0x%02X\n", faults->stack[i].fault_comm1);
        }

        if (faults->stack[i].fault_comm2 != 0U)
        {
            printk("\tCOMM2: 0x%02X\n", faults->stack[i].fault_comm2);
        }

        if (faults->stack[i].fault_comm3 != 0U)
        {
            printk("\tCOMM3: 0x%02X\n", faults->stack[i].fault_comm3);
        }

        if (faults->stack[i].fault_ot != 0U)
        {
            printk("\tOT: 0x%02X\n", faults->stack[i].fault_ot);
        }

        if (faults->stack[i].fault_ut != 0U)
        {
            printk("\tUT: 0x%02X\n", faults->stack[i].fault_ut);
        }

        if (faults->stack[i].fault_ov1 != 0U)
        {
            printk("\tOV1: 0x%02X\n", faults->stack[i].fault_ov1);
        }

        if (faults->stack[i].fault_ov2 != 0U)
        {
            printk("\tOV2: 0x%02X\n", faults->stack[i].fault_ov2);
        }

        if (faults->stack[i].fault_uv1 != 0U)
        {
            printk("\tUV1: 0x%02X\n", faults->stack[i].fault_uv1);
        }

        if (faults->stack[i].fault_uv2 != 0U)
        {
            printk("\tUV2: 0x%02X\n", faults->stack[i].fault_uv2);
        }

        if (faults->stack[i].fault_sys != 0U)
        {
            printk("\tSYS: 0x%02X\n", faults->stack[i].fault_sys);
        }

        if (faults->stack[i].fault_pwr1 != 0U)
        {
            printk("\tPWR1: 0x%02X\n", faults->stack[i].fault_pwr1);
        }

        if (faults->stack[i].fault_pwr2 != 0U)
        {
            printk("\tPWR2: 0x%02X\n", faults->stack[i].fault_pwr2);
        }

        if (faults->stack[i].fault_pwr3 != 0U)
        {
            printk("\tPWR3: 0x%02X\n", faults->stack[i].fault_pwr3);
        }
    }
#endif

#if defined(APP_DEBUG_INA238_FAULTS)
    if (faults->ina.active)
    {
        printk("INA238 fault summary: 0x%04X\n", faults->ina.diag_alrt);

        if (faults->ina.math_overflow)
        {
            printk("\tMATHOF\n");
        }

        if (faults->ina.temp_over_limit)
        {
            printk("\tTMPOL\n");
        }

        if (faults->ina.shunt_over_limit)
        {
            printk("\tSHNTOL\n");
        }

        if (faults->ina.shunt_under_limit)
        {
            printk("\tSHNTUL\n");
        }

        if (faults->ina.bus_over_limit)
        {
            printk("\tBUSOL\n");
        }

        if (faults->ina.bus_under_limit)
        {
            printk("\tBUSUL\n");
        }

        if (faults->ina.power_over_limit)
        {
            printk("\tPOL\n");
        }

        if (faults->ina.memory_checksum_error)
        {
            printk("\tMEMSTAT\n");
        }
    }
#endif

    printk("\n");
#else
    ARG_UNUSED(faults);
#endif
}

void debug_print_telemetry(const bms_metrics_t *metrics, const fault_data_t *faults)
{
#if defined(CONFIG_APP_DEBUG_PRINTS) && defined(CONFIG_APP_DEBUG_CAN_TELEMETRY)
    bool under_temperature = false;
    bool over_temperature = false;
    bool under_voltage = false;
    bool over_voltage = false;
    bool over_current_discharge = false;

    if (metrics == NULL)
    {
        printk("CAN telemetry got null pointer\n");
        return;
    }

    if (faults != NULL)
    {
        for (size_t i = 0; i < NUM_STACK_DEVICES; i++)
        {
            over_temperature |= faults->stack[i].fault_ot != 0U;
            under_temperature |= faults->stack[i].fault_ut != 0U;
            over_voltage |= (faults->stack[i].fault_ov1 != 0U) || (faults->stack[i].fault_ov2 != 0U);
            under_voltage |= (faults->stack[i].fault_uv1 != 0U) || (faults->stack[i].fault_uv2 != 0U);
        }

        over_current_discharge = faults->ina.shunt_over_limit;
    }

    printk("CAN telemetry transmitted\n");
    printk("\t0x%03X BMS_Protection DLC=%u: UT=%u OT=%u UV=%u OV=%u OCD=%u\n",
           BMS_PROTECTION_ID,
           BMS_PROTECTION_DLC,
           under_temperature,
           over_temperature,
           under_voltage,
           over_voltage,
           over_current_discharge);

    printk("\t0x%03X BMS_CellVoltageMin DLC=%u: CellVoltMinID=%u CellVoltMin=",
           BMS_CELL_VOLTAGE_MIN_ID,
           BMS_ID_FLOAT_DLC,
           metrics->cell_voltage_min.valid ? metrics->cell_voltage_min.id : 0U);
    print_float_3(metrics->cell_voltage_min.valid ? metrics->cell_voltage_min.value : 0.0f);
    printk(" V\n");

    printk("\t0x%03X BMS_CellVoltageMax DLC=%u: CellVoltMaxID=%u CellVoltMax=",
           BMS_CELL_VOLTAGE_MAX_ID,
           BMS_ID_FLOAT_DLC,
           metrics->cell_voltage_max.valid ? metrics->cell_voltage_max.id : 0U);
    print_float_3(metrics->cell_voltage_max.valid ? metrics->cell_voltage_max.value : 0.0f);
    printk(" V\n");

    printk("\t0x%03X BMS_CellTemperatureMin DLC=%u: CellTempMinID=%u CellTempMin=",
           BMS_CELL_TEMPERATURE_MIN_ID,
           BMS_ID_FLOAT_DLC,
           metrics->cell_temperature_min.valid ? metrics->cell_temperature_min.id : 0U);
    print_float_3(metrics->cell_temperature_min.valid ? metrics->cell_temperature_min.value : 0.0f);
    printk(" C\n");

    printk("\t0x%03X BMS_CellTemperatureMax DLC=%u: CellTempMaxID=%u CellTempMax=",
           BMS_CELL_TEMPERATURE_MAX_ID,
           BMS_ID_FLOAT_DLC,
           metrics->cell_temperature_max.valid ? metrics->cell_temperature_max.id : 0U);
    print_float_3(metrics->cell_temperature_max.valid ? metrics->cell_temperature_max.value : 0.0f);
    printk(" C\n");

    printk("\t0x%03X BMS_PackElectrical DLC=%u: PackCurrent=",
           BMS_PACK_ELECTRICAL_ID,
           BMS_PACK_ELECTRICAL_DLC);
    print_float_3(metrics->pack_current_valid ? metrics->pack_current : 0.0f);
    printk(" A PackVoltage=");
    print_float_3(metrics->pack_voltage_valid ? metrics->pack_voltage : 0.0f);
    printk(" V\n");

    printk("\t0x%03X BMS_PackPower DLC=%u: PackPower=",
           BMS_PACK_POWER_ID,
           BMS_PACK_POWER_DLC);
    print_float_3(metrics->pack_power_valid ? metrics->pack_power : 0.0f);
    printk(" W\n\n");
#else
    ARG_UNUSED(metrics);
    ARG_UNUSED(faults);
#endif
}

static const char *balancing_state_name(balancing_state_t state)
{
    switch (state)
    {
    case BALANCING_STATE_IDLE:
        return "idle";
    case BALANCING_STATE_RUNNING:
        return "running";
    case BALANCING_STATE_COMPLETE:
        return "complete";
    default:
        return "unknown";
    }
}

static void print_balancing_voltage_summary(const cell_voltage_data_t *voltages)
{
    float vmin = 100.0f;
    float vmax = 0.0f;
    int min_cell = -1;
    int max_cell = -1;

    if (voltages == NULL)
    {
        printk("Balancing voltage summary got null pointer\n");
        return;
    }

    for (int i = 0; i < NUM_BALANCE_CELLS; i++)
    {
        if (!voltages->cells[i].active)
        {
            continue;
        }

        if (voltages->cells[i].voltage < vmin)
        {
            vmin = voltages->cells[i].voltage;
            min_cell = i + 1;
        }

        if (voltages->cells[i].voltage > vmax)
        {
            vmax = voltages->cells[i].voltage;
            max_cell = i + 1;
        }
    }

    printk("Balancing voltage summary\n");

    if (min_cell > 0 && max_cell > 0)
    {
        printk("\tmin cell %d: ", min_cell);
        print_float_6(vmin);
        printk(" V\n");

        printk("\tmax cell %d: ", max_cell);
        print_float_6(vmax);
        printk(" V\n");

        printk("\tspread: ");
        print_float_6(vmax - vmin);
        printk(" V\n");

        for (int i = 0; i < NUM_BALANCE_CELLS; i++)
        {
            if (!voltages->cells[i].active)
            {
                continue;
            }

            printk("\tcell %d: voltage=", i + 1);
            print_float_6(voltages->cells[i].voltage);
            printk(" V delta=");
            print_float_6(voltages->cells[i].voltage - vmin);
            printk(" V\n");
        }
    }
    else
    {
        printk("\tunavailable\n");
    }

    printk("\n");
}

static void print_balancing_cycle_start(const cell_voltage_data_t *voltages,
                                        const balancing_data_t *balancing)
{
    float vmin = 100.0f;
    int min_cell = -1;

    if (voltages == NULL || balancing == NULL)
    {
        printk("Balancing cycle start got null pointer\n");
        return;
    }

    for (int i = 0; i < NUM_BALANCE_CELLS; i++)
    {
        if (!voltages->cells[i].active)
        {
            continue;
        }

        if (voltages->cells[i].voltage < vmin)
        {
            vmin = voltages->cells[i].voltage;
            min_cell = i + 1;
        }
    }

    printk("Balancing cycle start\n");
    printk("\tstate=%s active=%d selected_cells=0x%04X\n",
           balancing_state_name(balancing->state),
           balancing->active,
           balancing->selected_cells);

    if (min_cell > 0)
    {
        printk("\tminimum cell %d: ", min_cell);
        print_float_6(vmin);
        printk(" V\n");
    }

    for (int cell = 1; cell <= NUM_BALANCE_CELLS; cell++)
    {
        if ((balancing->selected_cells & (1U << (cell - 1))) == 0U)
        {
            continue;
        }

        printk("\tcell %d: voltage=", cell);
        print_float_6(voltages->cells[cell - 1].voltage);
        printk(" V delta=");
        print_float_6(voltages->cells[cell - 1].voltage - vmin);
        printk(" V\n");
    }

    printk("\n");
}

static void print_balancing_cycle_complete(const balancing_data_t *balancing)
{
    if (balancing == NULL)
    {
        printk("Balancing cycle complete got null pointer\n");
        return;
    }

    printk("Balancing cycle complete\n");
    printk("\tselected_cells=0x%04X complete_cells=0x%04X\n",
           balancing->selected_cells,
           balancing->complete_cells);

    for (int cell = 1; cell <= NUM_BALANCE_CELLS; cell++)
    {
        if ((balancing->selected_cells & (1U << (cell - 1))) == 0U)
        {
            continue;
        }

        printk("\tcell %d: complete=%d\n",
               cell,
               (balancing->complete_cells & (1U << (cell - 1))) != 0U);
    }

    printk("\n");
}

void debug_print_balancing(const cell_voltage_data_t *voltages,
                           const balancing_data_t *balancing,
                           balancing_state_t previous_state,
                           uint16_t previous_selected_cells)
{
#if defined(CONFIG_APP_DEBUG_PRINTS) && defined(CONFIG_APP_DEBUG_BALANCING)
    if (balancing == NULL)
    {
        printk("Balancing got null pointer\n");
        return;
    }

    if (previous_state != BALANCING_STATE_RUNNING &&
        balancing->state == BALANCING_STATE_RUNNING)
    {
        print_balancing_cycle_start(voltages, balancing);
    }
    else if (previous_state == BALANCING_STATE_RUNNING &&
             balancing->state == BALANCING_STATE_COMPLETE)
    {
        print_balancing_cycle_complete(balancing);
        print_balancing_voltage_summary(voltages);
    }
    else if (previous_state != BALANCING_STATE_IDLE &&
             balancing->state == BALANCING_STATE_IDLE)
    {
        printk("Balancing stopped: previous_selected_cells=0x%04X\n\n",
               previous_selected_cells);
    }
#else
    ARG_UNUSED(voltages);
    ARG_UNUSED(balancing);
    ARG_UNUSED(previous_state);
    ARG_UNUSED(previous_selected_cells);
#endif
}
