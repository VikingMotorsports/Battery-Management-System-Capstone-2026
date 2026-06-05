#include "init.h"
#include "debug.h"
#include "voltage.h"
#include "temperature.h"
#include "current.h"
#include "faults.h"
#include "balancing.h"
#include "metrics.h"
#include "telemetry.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if defined(CONFIG_APP_VOLTAGE_MONITORING) || defined(CONFIG_APP_TEMPERATURE_MONITORING)
#define APP_BQ796XX_MONITORING 1
#endif

int main(void)
{
    int ret;

#if defined(CONFIG_APP_VOLTAGE_MONITORING)
    cell_voltage_data_t voltages;
#endif
#if defined(CONFIG_APP_TEMPERATURE_MONITORING)
    temperature_data_t temperatures;
#endif
#if defined(CONFIG_APP_CURRENT_MONITORING)
    current_data_t current;
#endif
#if defined(CONFIG_APP_FAULT_MONITORING)
    fault_data_t faults = {0};
#endif
#if defined(CONFIG_APP_CAN_TELEMETRY)
    bms_metrics_t metrics;
#endif
#if defined(CONFIG_APP_CELL_BALANCING)
    balancing_data_t balancing = {0};
    balancing_state_t previous_balancing_state;
    uint16_t previous_selected_cells;
#endif

#if defined(CONFIG_APP_CURRENT_MONITORING) || defined(APP_BQ796XX_MONITORING)
    LOG_INF("battery monitor startup");

    ret = monitor_init();
    if (ret < 0)
    {
        LOG_ERR("monitor_init failed: %d", ret);
        return 0;
    }

    LOG_INF("monitor init complete");
#else
    LOG_INF("all monitor backends disabled");
#endif

#if defined(CONFIG_APP_VOLTAGE_MONITORING)
    ret = voltage_init();
    if (ret < 0)
    {
        LOG_ERR("voltage_init failed: %d", ret);
        return 0;
    }

    LOG_INF("voltage init complete");
#else
    LOG_INF("voltage monitoring disabled");
#endif

#if defined(CONFIG_APP_TEMPERATURE_MONITORING)
    ret = temperature_init();
    if (ret < 0)
    {
        LOG_ERR("temperature_init failed: %d", ret);
        return 0;
    }

    LOG_INF("temperature init complete");
#else
    LOG_INF("temperature monitoring disabled");
#endif

#if defined(CONFIG_APP_CURRENT_MONITORING)
    ret = current_init();
    if (ret < 0)
    {
        LOG_ERR("current_init failed: %d", ret);
        return 0;
    }

    LOG_INF("current init complete");
#else
    LOG_INF("current monitoring disabled");
#endif

#if defined(CONFIG_APP_FAULT_MONITORING)
    ret = faults_init();
    if (ret < 0)
    {
        LOG_ERR("faults_init failed: %d", ret);
        return 0;
    }

    LOG_INF("faults init complete");
#else
    LOG_INF("fault monitoring disabled");
#endif

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

#if defined(APP_BQ796XX_MONITORING)
    ret = commit_customer_crc();
    if (ret < 0)
    {
        LOG_ERR("commit_customer_crc failed: %d", ret);
        return 0;
    }

    LOG_INF("customer CRC commit complete");
#endif

#if defined(CONFIG_APP_FAULT_MONITORING)
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
        faults = (fault_data_t){0};
    }

    k_msleep(2000);
#endif

    while (1)
    {
#if defined(CONFIG_APP_CAN_TELEMETRY)
        bool have_voltages = false;
        bool have_temperatures = false;
        bool have_current = false;
#endif
#if defined(CONFIG_APP_FAULT_MONITORING)
        bool had_fault_interrupt = false;
#endif

#if defined(CONFIG_APP_FAULT_MONITORING)
        if (faults_pending())
        {
            faults_clear_pending();
            had_fault_interrupt = true;
        }
#endif

#if defined(CONFIG_APP_VOLTAGE_MONITORING)
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
#if defined(CONFIG_APP_CAN_TELEMETRY)
        have_voltages = true;
#endif
#endif

#if defined(CONFIG_APP_CURRENT_MONITORING)
        ret = read_current(&current);
        if (ret < 0)
        {
            LOG_ERR("read_current failed: %d", ret);
        }
        else
        {
            debug_print_current(&current);
#if defined(CONFIG_APP_CAN_TELEMETRY)
            have_current = true;
#endif
        }
#endif

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

#if defined(CONFIG_APP_TEMPERATURE_MONITORING)
        ret = read_temperatures(&temperatures);
        if (ret < 0)
        {
            LOG_ERR("read_temperatures failed: %d", ret);
        }
        else
        {
            debug_print_temperatures(&temperatures);
#if defined(CONFIG_APP_CAN_TELEMETRY)
            have_temperatures = true;
#endif
        }
#endif

#if defined(CONFIG_APP_FAULT_MONITORING)
        ret = read_faults(&faults);
        if (ret < 0)
        {
            LOG_ERR("read_faults failed: %d", ret);
        }
        else if (had_fault_interrupt)
        {
            debug_print_faults(&faults);
        }
#endif

#if defined(CONFIG_APP_CAN_TELEMETRY)
        calculate_metrics(
#if defined(CONFIG_APP_VOLTAGE_MONITORING)
            have_voltages ? &voltages : NULL,
#else
            NULL,
#endif
#if defined(CONFIG_APP_TEMPERATURE_MONITORING)
            have_temperatures ? &temperatures : NULL,
#else
            NULL,
#endif
#if defined(CONFIG_APP_CURRENT_MONITORING)
            have_current ? &current : NULL,
#else
            NULL,
#endif
            &metrics);

        ret = telemetry_transmit(
            &metrics,
#if defined(CONFIG_APP_FAULT_MONITORING)
            &faults
#else
            NULL
#endif
        );
        if (ret < 0)
        {
            LOG_ERR("telemetry_transmit failed: %d", ret);
        }
#endif

        k_msleep(1000);
    }

    return 0;
}
