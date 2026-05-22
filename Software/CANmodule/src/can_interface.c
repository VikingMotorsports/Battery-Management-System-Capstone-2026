/*
 * CAN Interface module implementation for the VCU system.
 *
 * This file contains the low-level CAN hardware initialization logic.
 * It is responsible for:
 *
 *  - Obtaining the CAN peripheral from the Zephyr devicetree
 *  - Configuring CAN bus timing
 *  - Setting CAN operating modes
 *  - Starting the CAN controller
 */

#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>

#include "can_interface.h"
#include "can_database.h"

/*
 * Global CAN device handle.
 *
 * DEVICE_DT_GET() retrieves the CAN peripheral specified by the
 * zephyr_canbus entry in the Zephyr devicetree configuration.
 */
const struct device *const can_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/*
 * Returns the CAN device handle used by the system.
 *
 * Other modules can call this function to access the initialized
 * CAN controller without directly owning the global variable.
 *
 * Returns:
 *  Pointer to CAN device structure.
 */
const struct device *can_interface_get_device(void) {
	return can_dev;
}

/*
 * Initializes and starts the CAN controller.
 *
 * This function:
 *  1. Calculates CAN timing parameters
 *  2. Configures CAN bitrate and sample point
 *  3. Verifies the CAN hardware is ready
 *  4. Optionally enables loopback mode
 *  5. Starts the CAN controller
 *
 * Current configuration:
 *  - Bitrate: 500 kbps
 *  - Sample point: 87.5%
 *
 * Parameters:
 *  can_dev - Pointer to CAN device to initialize.
 *
 * Returns:
 *   0  -> Initialization successful
 *  <0  -> Initialization failed
 */
int can_interface_init(const struct device *can_dev) {
	int ret;

	/*
	 * Structure used by Zephyr to store calculated CAN timing
	 * parameters such as prescaler and time segments.
	 */
	struct can_timing timing;

	//set bitrate to 500K and sample point to 87.5%
	ret = can_calc_timing(can_dev, &timing, 500000, 875);

	if (ret != 0) {
		printf("Error calculating CAN timing [%d]\n", ret);
		return ret;
	}

	/* Apply calculated timing settings to CAN controller */
	ret = can_set_timing(can_dev, &timing);

	if (ret != 0) {
		printf("Error setting CAN timing [%d]\n", ret);
		return ret;
	}

	/* Verify CAN hardware is initialized and ready */
	if (!device_is_ready(can_dev)) {
		printf("CAN ERROR! Device %s not ready.\n", can_dev->name);
		return -ENODEV;
	}

#ifdef CONFIG_LOOPBACK_MODE

	/*
	 * Enable internal CAN loopback mode.
	 *
	 * In loopback mode:
	 *  - transmitted messages are immediately received locally
	 *  - no external CAN transceiver or bus is required
	 */
	ret = can_set_mode(can_dev, CAN_MODE_LOOPBACK);

	if (ret != 0) {
		printf("Error setting CAN mode [%d]\n", ret);
		return ret;
	}

#endif

	/* Start CAN controller operation */
	ret = can_start(can_dev);

	if (ret != 0) {
		printf("Error starting CAN controller [%d]\n", ret);
		return ret;
	}

	printf("CAN interface initialized successfully\n");

	return 0;
}