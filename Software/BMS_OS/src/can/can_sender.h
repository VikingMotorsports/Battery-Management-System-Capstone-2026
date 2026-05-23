/*
 * CAN Sender module for the VCU system.
 *
 * This module is responsible for:
 *  - constructing CAN transmit frames
 *  - encoding sensor data into CAN payloads
 *  - transmitting CAN messages onto the bus
 *  - handling transmit completion callbacks
 *
 * The sender module provides a simplified API that allows the
 * application layer to transmit sensor values without directly
 * interacting with low-level CAN frame structures.
 *
 * This improves:
 *  - modularity
 *  - code reuse
 *  - readability
 *  - maintainability
 *
 * Additional helper functions can be added later for:
 *  - brake pedal messages
 *  - steering wheel messages
 *  - dashboard communication
 *  - inverter control
 *  - battery management
 */

#ifndef CAN_SENDER_H
#define CAN_SENDER_H

#include <stdint.h>

#include <zephyr/device.h>

//Number of frames to be created for sending data
#define NUM_CAN_FRAMES 2

/* OPTIONAL
 * Define can frame specs here such as payload size, msg id, etc.
 * 
 * Update NUM_CAN_FRAMES in can_sender.h
 * 
 * If not added here, generic frames will be used
*/
//const struct can_frame vcu_frames[NUM_CAN_FRAMES];

/* ---------- Public Interface ---------- */

/*
 * CAN transmit completion callback.
 *
 * This function is automatically called by the Zephyr CAN driver
 * after a CAN frame transmission completes or fails.
 *
 * Parameters:
 *  dev   - Pointer to CAN device
 *  error - Transmission result
 *            0  -> successful transmission
 *           <0  -> transmission failed
 *  arg   - Optional user-provided argument passed to can_send()
 *
 * This callback is useful for:
 *  - debugging
 *  - error detection
 *  - transmission tracking
 *  - asynchronous CAN communication
 */
void tx_irq_callback(
	const struct device *dev,
	int error,
	void *arg
);

/*
 * Sends a 16-bit sensor value over CAN.
 *
 * This helper function:
 *  - constructs a CAN frame
 *  - encodes the sensor value into the payload
 *  - transmits the frame using the Zephyr CAN driver
 *
 * Parameters:
 *  can_dev     - Pointer to initialized CAN device
 *  sensor_value - 16-bit sensor value to transmit
 *  sensor_id    - CAN identifier for the sensor message
 *
 * Returns:
 *   0  -> transmission successfully queued
 *  <0  -> transmission failed
 */
int can_send_sensor_data(
	const struct device *can_dev,
	uint16_t sensor_value,
	uint32_t sensor_id
);

#endif /* CAN_SENDER_H */