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
    SYSTEM_STATE_FAULT
} SystemState_t;

typedef enum
{
    FAULT_NONE                  = 0,
    FAULT_APP_OUT_OF_RANGE      = 1,
    FAULT_RING_OUT_OF_RANGE     = 2,
    FAULT_APP_PLAUSIBILITY_FAIL = 3
} SystemFaultCode_t;

extern volatile SystemState_t g_system_state;
extern volatile SystemFaultCode_t g_fault_code;

void state_transition(SystemState_t current_state);
void state_action(SystemState_t current_state);

#endif /* __SYSTEM_STATE_H__ */