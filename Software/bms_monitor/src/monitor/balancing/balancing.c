#include "balancing.h"
#include "protocol.h"
#include "../register_maps/bq79616_regs.h"

#include <zephyr/logging/log.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(balancing, LOG_LEVEL_INF);

#define STACK_ADDR_UNUSED 0U

#define BALANCE_MIN_CELL_VOLTAGE 2.50f
#define BALANCE_START_DELTA_V 0.020f
#define BALANCE_STOP_DELTA_V 0.010f

#define CB_TIMER_DISABLED 0x00U
#define CB_TIMER_30_SECONDS 0x02U

#define VCB_DONE_DISABLED 0x00U
#define BAL_DUTY_5_SECONDS 0x00U

#define BAL_CTRL2_AUTO_BAL_GO 0x03U

#define ONE_BYTE_NUM_BYTES 1U
#define ONE_BYTE_FRAME_LEN (ONE_BYTE_NUM_BYTES + 6U)

static int clear_cell_balance_timers(void);
static int write_cell_balance_timers(uint16_t selected_cells);
static int start_cell_balancing(void);
static int calculate_selected_cells(const cell_voltage_data_t *voltages, uint16_t previous_selected_cells, uint16_t *selected_cells);
static int read_complete_cells(uint16_t *complete_cells);

int balancing_init(void)
{
    int ret;
    uint8_t data;

    ret = clear_cell_balance_timers();
    if (ret < 0)
    {
        return ret;
    }

    data = VCB_DONE_DISABLED;
    ret = write_reg(STACK_WRITE, STACK_ADDR_UNUSED, BQ79616_REG_VCB_DONE_THRESH, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("VCB_DONE_THRESH write failed: reg=0x%04X err=%d", BQ79616_REG_VCB_DONE_THRESH, ret);
        return ret;
    }

    data = BAL_DUTY_5_SECONDS;
    ret = write_reg(STACK_WRITE, STACK_ADDR_UNUSED, BQ79616_REG_BAL_CTRL1, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BAL_CTRL1 write failed: reg=0x%04X err=%d", BQ79616_REG_BAL_CTRL1, ret);
        return ret;
    }

    data = 0U;
    ret = write_reg(STACK_WRITE, STACK_ADDR_UNUSED, BQ79616_REG_BAL_CTRL2, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BAL_CTRL2 write failed: reg=0x%04X err=%d", BQ79616_REG_BAL_CTRL2, ret);
        return ret;
    }

    LOG_INF("cell balancing initialized");
    return 0;
}

int balancing_update(const cell_voltage_data_t *voltages, balancing_data_t *balancing)
{
    uint16_t selected_cells = 0U;
    uint16_t complete_cells = 0U;
    int ret;

    if (voltages == NULL)
    {
        LOG_ERR("balancing_update got NULL voltage pointer");
        return -EINVAL;
    }

    if (balancing == NULL)
    {
        LOG_ERR("balancing_update got NULL balancing pointer");
        return -EINVAL;
    }

    if (balancing->state == BALANCING_STATE_RUNNING)
    {
        ret = read_complete_cells(&complete_cells);
        if (ret < 0)
        {
            return ret;
        }

        balancing->complete_cells = complete_cells;

        if ((complete_cells & balancing->selected_cells) == balancing->selected_cells)
        {
            balancing->active = false;
            balancing->state = BALANCING_STATE_COMPLETE;
        }

        return 0;
    }

    ret = calculate_selected_cells(voltages, balancing->selected_cells, &selected_cells);
    if (ret < 0)
    {
        ret = balancing_stop();
        if (ret < 0)
        {
            return ret;
        }

        balancing->selected_cells = 0U;
        balancing->complete_cells = 0U;
        balancing->active = false;
        balancing->state = BALANCING_STATE_IDLE;
        return 0;
    }

    if (selected_cells == 0U)
    {
        ret = balancing_stop();
        if (ret < 0)
        {
            return ret;
        }

        balancing->selected_cells = 0U;
        balancing->complete_cells = 0U;
        balancing->active = false;
        balancing->state = BALANCING_STATE_IDLE;
        return 0;
    }

    ret = write_cell_balance_timers(selected_cells);
    if (ret < 0)
    {
        return ret;
    }

    ret = start_cell_balancing();
    if (ret < 0)
    {
        return ret;
    }

    balancing->selected_cells = selected_cells;
    balancing->complete_cells = 0U;
    balancing->active = true;
    balancing->state = BALANCING_STATE_RUNNING;

    return 0;
}

static int calculate_selected_cells(const cell_voltage_data_t *voltages, uint16_t previous_selected_cells, uint16_t *selected_cells)
{
    float vmin = 100.0f;

    if (selected_cells == NULL)
    {
        return -EINVAL;
    }

    *selected_cells = 0U;

    for (int i = 0; i < NUM_BALANCE_CELLS; i++)
    {
        if (!voltages->cells[i].active)
        {
            LOG_ERR("cell %d inactive during balancing update", i + 1);
            return -EINVAL;
        }

        if (voltages->cells[i].voltage < vmin)
        {
            vmin = voltages->cells[i].voltage;
        }
    }

    if (vmin < BALANCE_MIN_CELL_VOLTAGE)
    {
        return -EINVAL;
    }

    for (int i = 0; i < NUM_BALANCE_CELLS; i++)
    {
        uint16_t cell_mask = (uint16_t)(1U << i);
        float delta = voltages->cells[i].voltage - vmin;
        bool was_selected = ((previous_selected_cells & cell_mask) != 0U);

        if (was_selected)
        {
            if (delta > BALANCE_STOP_DELTA_V)
            {
                *selected_cells |= cell_mask;
            }
        }
        else
        {
            if (delta >= BALANCE_START_DELTA_V)
            {
                *selected_cells |= cell_mask;
            }
        }
    }

    return 0;
}

static int read_complete_cells(uint16_t *complete_cells)
{
    uint8_t rx[ONE_BYTE_FRAME_LEN];
    uint8_t complete1;
    uint8_t complete2;
    uint16_t mask = 0U;
    int ret;

    if (complete_cells == NULL)
    {
        return -EINVAL;
    }

    ret = read_reg(STACK_READ, STACK_ADDR_UNUSED, BQ79616_REG_CB_COMPLETE1, rx, sizeof(rx), ONE_BYTE_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("CB_COMPLETE1 read failed: reg=0x%04X err=%d", BQ79616_REG_CB_COMPLETE1, ret);
        return ret;
    }
    complete1 = rx[4];

    ret = read_reg(STACK_READ, STACK_ADDR_UNUSED, BQ79616_REG_CB_COMPLETE2, rx, sizeof(rx), ONE_BYTE_NUM_BYTES);
    if (ret < 0)
    {
        LOG_ERR("CB_COMPLETE2 read failed: reg=0x%04X err=%d", BQ79616_REG_CB_COMPLETE2, ret);
        return ret;
    }
    complete2 = rx[4];

    for (int cell = 1; cell <= NUM_BALANCE_CELLS; cell++)
    {
        if ((complete2 & (1U << (cell - 1))) != 0U)
        {
            mask |= (uint16_t)(1U << (cell - 1));
        }
    }

    ARG_UNUSED(complete1);

    *complete_cells = mask;
    return 0;
}

int balancing_stop(void)
{
    int ret;
    uint8_t data;

    ret = write_cell_balance_timers(0U);
    if (ret < 0)
    {
        return ret;
    }

    data = BAL_CTRL2_AUTO_BAL_GO;
    ret = write_reg(STACK_WRITE, STACK_ADDR_UNUSED, BQ79616_REG_BAL_CTRL2, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BAL_CTRL2 zero-timer latch failed: reg=0x%04X err=%d", BQ79616_REG_BAL_CTRL2, ret);
        return ret;
    }

    return 0;
}

static int write_cell_balance_timers(uint16_t selected_cells)
{
    uint8_t data[16];
    int ret;

    for (int i = 0; i < 16; i++)
    {
        data[i] = CB_TIMER_DISABLED;
    }

    for (int cell = 1; cell <= NUM_BALANCE_CELLS; cell++)
    {
        if ((selected_cells & (1U << (cell - 1))) != 0U)
        {
            data[16 - cell] = CB_TIMER_30_SECONDS;
        }
    }

    ret = write_reg(STACK_WRITE, STACK_ADDR_UNUSED, BQ79616_REG_CB_CELL16_CTRL, &data[0], 8U);
    if (ret < 0)
    {
        LOG_ERR("CB_CELL16_CTRL..CB_CELL9_CTRL write failed: reg=0x%04X err=%d", BQ79616_REG_CB_CELL16_CTRL, ret);
        return ret;
    }

    ret = write_reg(STACK_WRITE, STACK_ADDR_UNUSED, BQ79616_REG_CB_CELL8_CTRL, &data[8], 8U);
    if (ret < 0)
    {
        LOG_ERR("CB_CELL8_CTRL..CB_CELL1_CTRL write failed: reg=0x%04X err=%d", BQ79616_REG_CB_CELL8_CTRL, ret);
        return ret;
    }

    return 0;
}

static int clear_cell_balance_timers(void)
{
    return write_cell_balance_timers(0U);
}

static int start_cell_balancing(void)
{
    int ret;
    uint8_t data;

    data = BAL_CTRL2_AUTO_BAL_GO;
    ret = write_reg(STACK_WRITE, STACK_ADDR_UNUSED, BQ79616_REG_BAL_CTRL2, &data, 1U);
    if (ret < 0)
    {
        LOG_ERR("BAL_CTRL2 auto balance start failed: reg=0x%04X err=%d", BQ79616_REG_BAL_CTRL2, ret);
        return ret;
    }

    return 0;
}
