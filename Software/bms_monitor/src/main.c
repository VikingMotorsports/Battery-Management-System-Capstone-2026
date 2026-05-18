#include "init.h"
#include "debug.h"
#include "voltage.h"
#include "temperature.h"
#include "current.h"
#include "faults.h"
#include "balancing.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    int ret;
    cell_voltage_data_t voltages;
    temperature_data_t temperatures;
    current_data_t current;
    fault_data_t faults;
#if defined(CONFIG_APP_CELL_BALANCING)
    balancing_data_t balancing = {0};
    balancing_state_t previous_balancing_state;
    uint16_t previous_selected_cells;
#endif

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

    ret = current_init();
    if (ret < 0)
    {
        LOG_ERR("current_init failed: %d", ret);
        return 0;
    }

    LOG_INF("current init complete");

    ret = faults_init();
    if (ret < 0)
    {
        LOG_ERR("faults_init failed: %d", ret);
        return 0;
    }

    LOG_INF("faults init complete");

#if defined(CONFIG_APP_CELL_BALANCING)
    ret = balancing_init();
    if (ret < 0)
    {
        LOG_ERR("balancing_init failed: %d", ret);
        return 0;
    }

    LOG_INF("balancing init complete");
#else
    LOG_INF("cell balancing disabled");
#endif

    k_msleep(10);

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

#if defined(CONFIG_APP_CELL_BALANCING)
            ret = balancing_stop();
            if (ret < 0)
            {
                LOG_ERR("balancing_stop failed: %d", ret);
            }
#endif

            k_msleep(1000);
            continue;
        }

        debug_print_voltages(&voltages);

        ret = read_current(&current);
        if (ret < 0)
        {
            LOG_ERR("read_current failed: %d", ret);
        }
        else
        {
            debug_print_current(&current);
        }

#if defined(CONFIG_APP_CELL_BALANCING)
        previous_balancing_state = balancing.state;
        previous_selected_cells = balancing.selected_cells;

        ret = balancing_update(&voltages, &balancing);
        if (ret < 0)
        {
            LOG_ERR("balancing_update failed: %d", ret);

            ret = balancing_stop();
            if (ret < 0)
            {
                LOG_ERR("balancing_stop failed: %d", ret);
            }
        }
        else
        {
            if (previous_balancing_state == BALANCING_STATE_RUNNING &&
                balancing.state == BALANCING_STATE_COMPLETE)
            {
                k_msleep(100);
                ret = read_cell_voltages(&voltages);
                if (ret < 0)
                {
                    LOG_ERR("post-balance read_cell_voltages failed: %d", ret);
                }
            }

            debug_print_balancing(&voltages,
                                  &balancing,
                                  previous_balancing_state,
                                  previous_selected_cells);
        }
#endif

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
