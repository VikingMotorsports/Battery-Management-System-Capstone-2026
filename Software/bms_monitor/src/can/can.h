#ifndef CAN_H
#define CAN_H

#include <stddef.h>
#include <stdint.h>

#define CAN_MAX_PAYLOAD_BYTES 8U

typedef struct
{
    uint32_t id;
    uint8_t data[CAN_MAX_PAYLOAD_BYTES];
    size_t len;
} can_message_t;

int can_init(void);
int can_transmit(const can_message_t *msg);

#endif
