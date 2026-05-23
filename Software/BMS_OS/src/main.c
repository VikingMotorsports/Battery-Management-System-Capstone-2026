/*
 * Main application for the VCU (Vehicle Control Unit) module.
 *
 * This application demonstrates a modular embedded software architecture
 * using the Zephyr RTOS. The system currently performs:
 *
 *  - accelerator pedal ADC acquisition
 *  - PWM motor control
 *  - CAN bus initialization
 *  - CAN message transmission
 *  - CAN message reception using a dedicated thread
 *
 * System Overview:
 *
 *   pedal_adc module
 *        ↓
 *   main control loop
 *        ↓
 *   motor_controller_pwm module
 *        ↓
 *   can_sender module
 *
 * Meanwhile:
 *
 *   can_receiver module
 *        ↓
 *   background CAN receive thread
 *
 * The main loop continuously:
 *  1. reads accelerator pedal position
 *  2. applies pedal deadband logic
 *  3. calculates PWM duty cycle
 *  4. drives the motor controller
 *  5. transmits pedal position over CAN
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "pedal_adc.h"
#include "motor_controller_pwm.h"
#include "can_interface.h"
#include "can_receiver.h"
#include "can_sender.h"
#include "can_database.h"

/*
 * Main control loop execution period in milliseconds.
 *
 * This determines how frequently:
 *  - ADC values are sampled
 *  - motor outputs are updated
 *  - CAN messages are transmitted
 */
#define LOOP_PERIOD_MS 20

/*
 * Main application entry point.
 *
 * This function performs:
 *  - subsystem initialization
 *  - CAN interface startup
 *  - CAN receiver thread creation
 *  - continuous vehicle control execution
 *
 * Returns:
 *   0  -> normal operation (typically never reached)
 *  <0  -> initialization failure
 */
int main(void)
{
	static int rc;

	/*
	 * Retrieve pointer to initialized CAN controller device.
	 *
	 * The CAN hardware itself is owned by the CAN interface
	 * module and accessed through this helper function.
	 */
	const struct device *can_dev =
		can_interface_get_device();

	/* ---------- Subsystem Initialization ---------- */

	/*
	 * Initialize CAN controller hardware.
	 */
	rc = can_interface_init(can_dev);
	if (rc) {
		printk("can_interface_init failed: %d\n", rc);
		return rc;
	}

	printf("Finished init.\n");

	/* ---------- Main Vehicle Control Loop ---------- */

	while (1) {
		/* ---------- CAN Transmission ---------- */

		// * Convert duty cycle percentage into integer
		// * value for CAN transmission.
		uint16_t acc_pedal_percent = (uint16_t)duty;

		// * Transmit accelerator pedal position over CAN bus.
		if (can_send_sensor_data(can_dev, acc_pedal_percent, ACCELERATOR_MSG_ID)) {
			printk("can_send_sensor_data failed: %d\n", rc);
		}

		// * Wait until next control loop iteration.
		k_msleep(LOOP_PERIOD_MS);
	}
};
