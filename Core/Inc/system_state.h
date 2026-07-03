#ifndef __SYSTEM_STATE_H
#define __SYSTEM_STATE_H

#include "stm32l4xx_hal.h"

typedef enum
{
    SYSTEM_STATE_INIT,
    SYSTEM_STATE_CALIBRATION,
    SYSTEM_STATE_BYPASS,
    SYSTEM_STATE_RING_IDLE,
    SYSTEM_STATE_RING_ACTIVE,
    SYSTEM_STATE_RING_BRAKE_OVERRIDE,
    SYSTEM_STATE_ERROR
} SystemState_t;

extern volatile SystemState_t g_system_state;

void state_transition(SystemState_t current_state);
void state_action(SystemState_t current_state);

#endif /* __SYSTEM_STATE_H__ */