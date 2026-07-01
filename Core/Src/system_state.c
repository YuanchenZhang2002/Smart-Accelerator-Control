#include "main.h"
#include "system_state.h"
#include "stm32l4xx_hal_conf.h"
extern volatile uint8_t activation_flag;
volatile SystemState_t g_system_state=SYSTEM_STATE_BYPASS;
volatile CalibrationData_t *flash_data = (CalibrationData_t *)CALIBRATION_ADDR;
volatile CalibrationData_t g_current_cal_data;

void state_transition(SystemState_t g_current_state)
{
    switch (g_current_state)
    {
        case SYSTEM_STATE_BYPASS:
            if (activation_flag == SET && pedal_output==ADC_OUT_OF_RANGE_MIN)
            {
                g_system_state = SYSTEM_STATE_INIT;
            }
            break;
        case SYSTEM_STATE_RING_INIT:
            if(flash_data->magic==CALIBRATION_MAGIC)
            {
                g_current_cal_data.pedal1_max=flash_data->pedal1_max;//load previous calibration data
                g_current_cal_data.pedal1_min=flash_data->pedal1_min;
                g_current_cal_data.pedal2_max=flash_data->pedal2_max;
                g_current_cal_data.pedal2_min=flash_data->pedal2_min;
                g_current_cal_data.magic=flash_data->magic;
                g_system_state=SYSTEM_STATE_RING_IDLE;//set system state to ring idle
            }
            else
            {
                g_system_state=SYSTEM_STATE_CALIBRATION;//set system state to calibration
            }
            break;
        case SYSTEM_STATE_CALIBRATION:
            break;
        case SYSTEM_STATE_RING_IDLE:
            break;
        case SYSTEM_STATE_RING_ACTIVE:
            break;
        case SYSTEM_STATE_RING_BRAKE_OVERRIDE:
            break;
        case SYSTEM_STATE_ERROR:
            break;
    }
}

void state_action(SystemState_t g_current_state)
{
    switch (g_current_state)
    {
        case SYSTEM_STATE_BYPASS:
            break;
        case SYSTEM_STATE_RING_INIT:
            break;
        case SYSTEM_STATE_CALIBRATION:
            break;
        case SYSTEM_STATE_RING_IDLE:
            break;
        case SYSTEM_STATE_RING_ACTIVE:
            break;
        case SYSTEM_STATE_RING_BRAKE_OVERRIDE:
            break;
        case SYSTEM_STATE_ERROR:
            break;
    }
}