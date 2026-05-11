#include "init.h"
#include "voltage.h"
#include "temperature.h"
#include "faults.h"
#include "protocol.h"
#include "monitor/register_maps/bq79616_regs.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
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
        LOG_ERR("initial read_faults failed: %d", ret);
    }
    else
    {
        printk("Initial Bridge Fault Summary: 0x%02X\n", faults.bridge.fault_summary);
        printk("Initial Bridge Fault COMM1:   0x%02X\n", faults.bridge.fault_comm1);
        printk("Initial Bridge Fault COMM2:   0x%02X\n", faults.bridge.fault_comm2);
        printk("Initial Bridge Fault REG:     0x%02X\n", faults.bridge.fault_reg);
        printk("Initial Bridge Fault SYS:     0x%02X\n", faults.bridge.fault_sys);
        printk("Initial Bridge Fault PWR:     0x%02X\n", faults.bridge.fault_pwr);

        for (int i = 0; i < NUM_STACK_DEVICES; i++)
        {
            printk("Initial Stack Dev %d Summary:        0x%02X\n", i + 1, faults.stack[i].fault_summary);
            printk("  PROT1=0x%02X PROT2=0x%02X\n",
                   faults.stack[i].fault_prot1,
                   faults.stack[i].fault_prot2);

            printk("  COMP_GPIO=0x%02X COMP_MISC=0x%02X VCCB1=0x%02X VCCB2=0x%02X\n",
                   faults.stack[i].fault_comp_gpio,
                   faults.stack[i].fault_comp_misc,
                   faults.stack[i].fault_comp_vccb1,
                   faults.stack[i].fault_comp_vccb2);

            printk("  VCOW1=0x%02X VCOW2=0x%02X CBOW1=0x%02X CBOW2=0x%02X\n",
                   faults.stack[i].fault_comp_vcow1,
                   faults.stack[i].fault_comp_vcow2,
                   faults.stack[i].fault_comp_cbow1,
                   faults.stack[i].fault_comp_cbow2);

            printk("  CBFET1=0x%02X CBFET2=0x%02X OTP=0x%02X\n",
                   faults.stack[i].fault_comp_cbfet1,
                   faults.stack[i].fault_comp_cbfet2,
                   faults.stack[i].fault_otp);

            printk("  COMM1=0x%02X COMM2=0x%02X COMM3=0x%02X\n",
                   faults.stack[i].fault_comm1,
                   faults.stack[i].fault_comm2,
                   faults.stack[i].fault_comm3);

            printk("  OT=0x%02X UT=0x%02X OV1=0x%02X OV2=0x%02X UV1=0x%02X UV2=0x%02X\n",
                   faults.stack[i].fault_ot,
                   faults.stack[i].fault_ut,
                   faults.stack[i].fault_ov1,
                   faults.stack[i].fault_ov2,
                   faults.stack[i].fault_uv1,
                   faults.stack[i].fault_uv2);

            printk("  SYS=0x%02X PWR1=0x%02X PWR2=0x%02X PWR3=0x%02X\n",
                   faults.stack[i].fault_sys,
                   faults.stack[i].fault_pwr1,
                   faults.stack[i].fault_pwr2,
                   faults.stack[i].fault_pwr3);
        }

        printk("\n");
    }

    k_msleep(2000);

    ret = clear_faults();
    if (ret < 0)
    {
        LOG_ERR("faults clear failed: %d", ret);
    }

    LOG_INF("faults clear complete");

    ret = read_faults(&faults);
    if (ret < 0)
    {
        LOG_ERR("faults read failed: %d", ret);
    }
    else
    {
        printk("Bridge Fault Summary Re-read: 0x%02X\n", faults.bridge.fault_summary);
        printk("Bridge Fault COMM1 Re-read:   0x%02X\n", faults.bridge.fault_comm1);
        printk("Bridge Fault COMM2 Re-read:   0x%02X\n", faults.bridge.fault_comm2);
        printk("Bridge Fault REG Re-read:     0x%02X\n", faults.bridge.fault_reg);
        printk("Bridge Fault SYS Re-read:     0x%02X\n", faults.bridge.fault_sys);
        printk("Bridge Fault PWR Re-read:     0x%02X\n", faults.bridge.fault_pwr);

        for (int i = 0; i < NUM_STACK_DEVICES; i++)
        {
            printk("Stack Dev %d Summary Re-read:        0x%02X\n", i + 1, faults.stack[i].fault_summary);
            printk("  PROT1=0x%02X PROT2=0x%02X\n",
                   faults.stack[i].fault_prot1,
                   faults.stack[i].fault_prot2);

            printk("  COMP_GPIO=0x%02X COMP_MISC=0x%02X VCCB1=0x%02X VCCB2=0x%02X\n",
                   faults.stack[i].fault_comp_gpio,
                   faults.stack[i].fault_comp_misc,
                   faults.stack[i].fault_comp_vccb1,
                   faults.stack[i].fault_comp_vccb2);

            printk("  VCOW1=0x%02X VCOW2=0x%02X CBOW1=0x%02X CBOW2=0x%02X\n",
                   faults.stack[i].fault_comp_vcow1,
                   faults.stack[i].fault_comp_vcow2,
                   faults.stack[i].fault_comp_cbow1,
                   faults.stack[i].fault_comp_cbow2);

            printk("  CBFET1=0x%02X CBFET2=0x%02X OTP=0x%02X\n",
                   faults.stack[i].fault_comp_cbfet1,
                   faults.stack[i].fault_comp_cbfet2,
                   faults.stack[i].fault_otp);

            printk("  COMM1=0x%02X COMM2=0x%02X COMM3=0x%02X\n",
                   faults.stack[i].fault_comm1,
                   faults.stack[i].fault_comm2,
                   faults.stack[i].fault_comm3);

            printk("  OT=0x%02X UT=0x%02X OV1=0x%02X OV2=0x%02X UV1=0x%02X UV2=0x%02X\n",
                   faults.stack[i].fault_ot,
                   faults.stack[i].fault_ut,
                   faults.stack[i].fault_ov1,
                   faults.stack[i].fault_ov2,
                   faults.stack[i].fault_uv1,
                   faults.stack[i].fault_uv2);

            printk("  SYS=0x%02X PWR1=0x%02X PWR2=0x%02X PWR3=0x%02X\n",
                   faults.stack[i].fault_sys,
                   faults.stack[i].fault_pwr1,
                   faults.stack[i].fault_pwr2,
                   faults.stack[i].fault_pwr3);
        }

        printk("\n");
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
                printk("Bridge Fault Summary: 0x%02X\n", faults.bridge.fault_summary);
                printk("Bridge Fault COMM1:   0x%02X\n", faults.bridge.fault_comm1);
                printk("Bridge Fault COMM2:   0x%02X\n", faults.bridge.fault_comm2);
                printk("Bridge Fault REG:     0x%02X\n", faults.bridge.fault_reg);
                printk("Bridge Fault SYS:     0x%02X\n", faults.bridge.fault_sys);
                printk("Bridge Fault PWR:     0x%02X\n", faults.bridge.fault_pwr);

                for (int i = 0; i < NUM_STACK_DEVICES; i++)
                {
                    printk("Stack Dev %d Summary:        0x%02X\n", i + 1, faults.stack[i].fault_summary);
                    printk("  PROT1=0x%02X PROT2=0x%02X\n",
                           faults.stack[i].fault_prot1,
                           faults.stack[i].fault_prot2);

                    printk("  COMP_GPIO=0x%02X COMP_MISC=0x%02X VCCB1=0x%02X VCCB2=0x%02X\n",
                           faults.stack[i].fault_comp_gpio,
                           faults.stack[i].fault_comp_misc,
                           faults.stack[i].fault_comp_vccb1,
                           faults.stack[i].fault_comp_vccb2);

                    printk("  VCOW1=0x%02X VCOW2=0x%02X CBOW1=0x%02X CBOW2=0x%02X\n",
                           faults.stack[i].fault_comp_vcow1,
                           faults.stack[i].fault_comp_vcow2,
                           faults.stack[i].fault_comp_cbow1,
                           faults.stack[i].fault_comp_cbow2);

                    printk("  CBFET1=0x%02X CBFET2=0x%02X OTP=0x%02X\n",
                           faults.stack[i].fault_comp_cbfet1,
                           faults.stack[i].fault_comp_cbfet2,
                           faults.stack[i].fault_otp);

                    printk("  COMM1=0x%02X COMM2=0x%02X COMM3=0x%02X\n",
                           faults.stack[i].fault_comm1,
                           faults.stack[i].fault_comm2,
                           faults.stack[i].fault_comm3);

                    printk("  OT=0x%02X UT=0x%02X OV1=0x%02X OV2=0x%02X UV1=0x%02X UV2=0x%02X\n",
                           faults.stack[i].fault_ot,
                           faults.stack[i].fault_ut,
                           faults.stack[i].fault_ov1,
                           faults.stack[i].fault_ov2,
                           faults.stack[i].fault_uv1,
                           faults.stack[i].fault_uv2);

                    printk("  SYS=0x%02X PWR1=0x%02X PWR2=0x%02X PWR3=0x%02X\n",
                           faults.stack[i].fault_sys,
                           faults.stack[i].fault_pwr1,
                           faults.stack[i].fault_pwr2,
                           faults.stack[i].fault_pwr3);
                }

                uint8_t rx_buf[7];
                ret = read_reg(SINGLE_READ, 1U, BQ79616_REG_CUST_CRC_HI,
                               rx_buf, sizeof(rx_buf), 1);
                if (ret < 0)
                {
                    LOG_ERR("CUST_CRC_HI failed: err=%d", ret);
                    return ret;
                }
                printk("CUST_CRC_HI: 0x%02X\n", rx_buf[4]);

                ret = read_reg(SINGLE_READ, 1U, BQ79616_REG_CUST_CRC_LO,
                               rx_buf, sizeof(rx_buf), 1);
                if (ret < 0)
                {
                    LOG_ERR("CUST_CRC_LO failed: err=%d", ret);
                    return ret;
                }
                printk("CUST_CRC_LO: 0x%02X\n", rx_buf[4]);

                ret = read_reg(SINGLE_READ, 1U, BQ79616_REG_CUST_CRC_RSLT_HI,
                               rx_buf, sizeof(rx_buf), 1);
                if (ret < 0)
                {
                    LOG_ERR("CUST_CRC_RSLT_HI failed: err=%d", ret);
                    return ret;
                }
                printk("CUST_CRC_RSLT_HI: 0x%02X\n", rx_buf[4]);

                ret = read_reg(SINGLE_READ, 1U, BQ79616_REG_CUST_CRC_RSLT_LO,
                               rx_buf, sizeof(rx_buf), 1);
                if (ret < 0)
                {
                    LOG_ERR("CUST_CRC_RSLT_LO failed: err=%d", ret);
                    return ret;
                }
                printk("CUST_CRC_RSLT_LO: 0x%02X\n", rx_buf[4]);

                printk("\n");
            }
        }

        ret = read_cell_voltages(&voltages);
        if (ret < 0)
        {
            LOG_ERR("read_cell_voltages failed: %d", ret);
            k_msleep(1000);
            continue;
        }

        for (int i = 0; i < NUM_CELLS_PER_MONITOR; i++)
        {
            if (!voltages.cells[i].active)
            {
                printk("Cell %2d: inactive\n", i + 1);
                continue;
            }

            float volts = voltages.cells[i].voltage;
            int whole = (int)volts;
            int frac = (int)((volts - (float)whole) * 1000000.0f);

            if (frac < 0)
            {
                frac = -frac;
            }

            printk("Cell %2d: raw=0x%04X voltage=%d.%06d V\n",
                   i + 1,
                   (uint16_t)voltages.cells[i].raw,
                   whole,
                   frac);
        }

        printk("\n");

        // ret = read_temperatures(&temperatures);
        // if (ret < 0)
        // {
        //     LOG_ERR("read_temperatures failed: %d", ret);
        // }
        // else
        // {
        //     for (int i = 0; i < NUM_THERMISTORS_PER_MONITOR; i++)
        //     {
        //         if (!temperatures.thermistors[i].active)
        //         {
        //             printk("Thermistor %d: inactive\n", i + 1);
        //             continue;
        //         }

        //         float temp_c = temperatures.thermistors[i].temperature_c;
        //         int temp_whole = (int)temp_c;
        //         int temp_frac = (int)((temp_c - (float)temp_whole) * 1000.0f);

        //         if (temp_frac < 0)
        //         {
        //             temp_frac = -temp_frac;
        //         }

        //         float gpio_v = temperatures.thermistors[i].gpio_volts;
        //         int gpio_whole = (int)gpio_v;
        //         int gpio_frac = (int)((gpio_v - (float)gpio_whole) * 1000000.0f);

        //         if (gpio_frac < 0)
        //         {
        //             gpio_frac = -gpio_frac;
        //         }

        //         float tsref_v = temperatures.thermistors[i].tsref_volts;
        //         int tsref_whole = (int)tsref_v;
        //         int tsref_frac = (int)((tsref_v - (float)tsref_whole) * 1000000.0f);

        //         if (tsref_frac < 0)
        //         {
        //             tsref_frac = -tsref_frac;
        //         }

        //         float rntc = temperatures.thermistors[i].thermistor_ohms;
        //         int r_whole = (int)rntc;
        //         int r_frac = (int)((rntc - (float)r_whole) * 10.0f);

        //         if (r_frac < 0)
        //         {
        //             r_frac = -r_frac;
        //         }

        //         printk("Thermistor %d: raw_gpio=0x%04X raw_tsref=0x%04X "
        //                "gpio=%d.%06d V tsref=%d.%06d V Rntc=%d.%1d ohm T=%d.%03d C\n",
        //                i + 1,
        //                temperatures.thermistors[i].raw_gpio,
        //                temperatures.thermistors[i].raw_tsref,
        //                gpio_whole,
        //                gpio_frac,
        //                tsref_whole,
        //                tsref_frac,
        //                r_whole,
        //                r_frac,
        //                temp_whole,
        //                temp_frac);
        //     }
        // }

        // printk("\n");
        k_msleep(1000);
    }

    return 0;
}