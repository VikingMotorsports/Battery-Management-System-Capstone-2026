#include "init.h"
#include "debug.h"
#include "voltage.h"
#include "temperature.h"
#include "faults.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    int ret;
    cell_voltage_data_t voltages;
    temperature_data_t temperatures;
    fault_data_t faults;

    LOG_INF("battery monitor startup");

    ret = monitor_init();
    if (ret < 0)
    {
        LOG_ERR("monitor_init failed: %d", ret);
        return 0;
    }

    LOG_INF("monitor init complete");

    ret = voltage_init();
    if (ret < 0)
    {
        LOG_ERR("voltage_init failed: %d", ret);
        return 0;
    }

    LOG_INF("voltage init complete");

    ret = temperature_init();
    if (ret < 0)
    {
        LOG_ERR("temperature_init failed: %d", ret);
        return 0;
    }

    LOG_INF("temperature init complete");

    ret = faults_init();
    if (ret < 0)
    {
        LOG_ERR("faults_init failed: %d", ret);
        return 0;
    }

    LOG_INF("faults init complete");

    ret = commit_customer_crc();
    if (ret < 0)
    {
        LOG_ERR("commit_customer_crc failed: %d", ret);
        return 0;
    }

    LOG_INF("customer CRC commit complete");

    ret = read_faults(&faults);
    if (ret < 0)
    {
        LOG_ERR("read_faults failed: %d", ret);
    }
    else
    {
        debug_print_faults(&faults);
    }

    k_msleep(2000);

    ret = clear_faults();
    if (ret < 0)
    {
        LOG_ERR("clear_faults failed: %d", ret);
    }
    else
    {
        LOG_INF("faults clear complete");
    }

    ret = read_faults(&faults);
    if (ret < 0)
    {
        LOG_ERR("read_faults failed: %d", ret);
    }
    else
    {
        debug_print_faults(&faults);
    }

    k_msleep(2000);

    while (1)
    {
        if (faults_pending())
        {
            faults_clear_pending();

            ret = read_faults(&faults);
            if (ret < 0)
            {
                LOG_ERR("read_faults failed: %d", ret);
            }
            else
            {
                debug_print_faults(&faults);
            }
        }

        ret = read_cell_voltages(&voltages);
        if (ret < 0)
        {
            LOG_ERR("read_cell_voltages failed: %d", ret);
            k_msleep(1000);
            continue;
        }
        debug_print_voltages(&voltages);

        ret = read_temperatures(&temperatures);
        if (ret < 0)
        {
            LOG_ERR("read_temperatures failed: %d", ret);
        }
        else
        {
            debug_print_temperatures(&temperatures);
        }

        k_msleep(1000);
    }

    return 0;
}