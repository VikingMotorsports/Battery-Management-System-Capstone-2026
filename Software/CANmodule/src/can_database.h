#ifndef CAN_DATABASE_H
#define CAN_DATABASE_H

#include <stdint.h>

/*
 * Add new CAN message IDs here
 * 
 * filters and receiving behavior can be added in can_receiver
 * 
 * frame information and format can be added in can_sender
 */

/* ---------- CAN Message IDs ---------- */

#define ACCELERATOR_MSG_ID 0x080
#define MOTOR_DUTY_MSG_ID 0x100

#endif