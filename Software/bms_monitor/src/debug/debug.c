#include "debug.h"

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

static void print_float_6(float value)
{
    int whole = (int)value;
    int frac = (int)((value - (float)whole) * 1000000.0f);

    if (frac < 0)
    {
        frac = -frac;
    }

    printk("%d.%06d", whole, frac);
}

static void print_float_3(float value)
{
    int whole = (int)value;
    int frac = (int)((value - (float)whole) * 1000.0f);

    if (frac < 0)
    {
        frac = -frac;
    }

    printk("%d.%03d", whole, frac);
}

static void print_float_1(float value)
{
    int whole = (int)value;
    int frac = (int)((value - (float)whole) * 10.0f);

    if (frac < 0)
    {
        frac = -frac;
    }

    printk("%d.%1d", whole, frac);
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

void debug_print_faults(const fault_data_t *faults)
{
#if defined(CONFIG_APP_DEBUG_PRINTS) && defined(CONFIG_APP_DEBUG_FAULTS)
    if (faults == NULL)
    {
        printk("Faults got null pointer\n");
        return;
    }

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

    printk("\n");
#else
    ARG_UNUSED(faults);
#endif
}