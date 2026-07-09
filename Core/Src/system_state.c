#include "main.h"
#include "system_state.h"
#include "stm32l476xx.h"
#include "stm32l4xx.h"
#include "stm32l4xx_hal_conf.h"
#include "dac.h"
#include "stm32l4xx_hal_gpio.h"
#include <stdlib.h>
extern volatile uint8_t activation_flag;
volatile SystemState_t g_system_state=SYSTEM_STATE_INIT;
volatile CalibrationData_t *flash_data = (CalibrationData_t *)CALIBRATION_ADDR;
volatile CalibrationData_t g_current_cal_data;
volatile uint8_t g_error_flag=0;
extern volatile uint8_t calibration_save_flag;
extern volatile uint16_t adc_filtered_APP1;
extern volatile uint16_t adc_filtered_APP2;
extern volatile uint16_t adc_filtered_RING;
extern volatile uint8_t brake_active;
uint16_t dac_out1 = 0;
uint16_t dac_out2 = 0;
uint16_t dac2_value = 0;


void state_transition(SystemState_t current_state)
{
    if(current_state == SYSTEM_STATE_BYPASS)
    {
        if (adc_filtered_APP1>ADC_OUT_OF_RANGE_MAX||adc_filtered_APP1<ADC_OUT_OF_RANGE_MIN||
            adc_filtered_APP2>ADC_OUT_OF_RANGE_MAX||adc_filtered_APP2<ADC_OUT_OF_RANGE_MIN)
        {
             g_error_flag = 1; 
        }
    }else if (current_state == SYSTEM_STATE_RING_IDLE ||
              current_state == SYSTEM_STATE_RING_ACTIVE ||
              current_state == SYSTEM_STATE_RING_BRAKE_OVERRIDE) 
    {
        if(adc_filtered_RING<200||adc_filtered_RING>3500)
        {
            g_system_state = SYSTEM_STATE_ERROR;
            g_error_flag = 2; 
        }
    }
    switch (current_state)
    {
        case SYSTEM_STATE_BYPASS:
            if ((activation_flag == SET) && (adc_filtered_APP1<=g_current_cal_data.pedal1_min + RATIONALITY_TOLERANCE)&&(adc_filtered_RING<=RING_ADC_MIN+ RING_ENTRY_THRESHOLD))
            { 
                // Latch the DCDT relay: switch COM from NC (pedal) to NO (MCU DAC).
                // The relay stays energized for the entire ignition cycle.
                // Only an MCU power-off de-energizes it, falling back to NC (Fail-Safe Bypass).
                HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, GPIO_PIN_SET);
                g_system_state = SYSTEM_STATE_RING_IDLE;
                activation_flag = 0; // Clear only after successful transition
            }
            // NOTE: activation_flag is intentionally NOT cleared here when conditions
            // are not met, so the flag persists until the ADC conditions are satisfied.
            break;
        case SYSTEM_STATE_INIT:
            if(flash_data->magic==CALIBRATION_MAGIC)
            {
                g_current_cal_data.pedal1_max=flash_data->pedal1_max;//load previous calibration data
                g_current_cal_data.pedal1_min=flash_data->pedal1_min;
                g_current_cal_data.pedal2_max=flash_data->pedal2_max;
                g_current_cal_data.pedal2_min=flash_data->pedal2_min;
                g_current_cal_data.magic=flash_data->magic;
                g_system_state=SYSTEM_STATE_BYPASS;//set system state to bypass as default
            }
            else
            {
                g_current_cal_data.pedal1_min = 4095; 
                g_current_cal_data.pedal1_max = 0;
                g_current_cal_data.pedal2_min = 4095;
                g_current_cal_data.pedal2_max = 0;
                g_system_state=SYSTEM_STATE_CALIBRATION;//set system state to calibration
            }
            break;
        case SYSTEM_STATE_CALIBRATION:
            if(calibration_save_flag==SET)
            {
                g_system_state=SYSTEM_STATE_BYPASS;
            }
            break;
        case SYSTEM_STATE_RING_IDLE:
            if (adc_filtered_RING > (RING_ADC_MIN + RING_ENTRY_THRESHOLD)) //import Hysteresis
            {
                g_system_state = SYSTEM_STATE_RING_ACTIVE;
            }
            if(brake_active==SET)
            {
                g_system_state = SYSTEM_STATE_RING_BRAKE_OVERRIDE;
            }
            break;
        case SYSTEM_STATE_RING_ACTIVE:
            if(brake_active==SET)//highest priority
            {
                g_system_state = SYSTEM_STATE_RING_BRAKE_OVERRIDE;
            }
            else if(adc_filtered_RING < (RING_ADC_MIN + RING_EXIT_THRESHOLD))
            {
                g_system_state = SYSTEM_STATE_RING_IDLE;
            }
            break;
        case SYSTEM_STATE_RING_BRAKE_OVERRIDE:
            if((brake_active==RESET) && (adc_filtered_RING < RING_ADC_MIN + RING_EXIT_THRESHOLD))//prevent sudden acceleration
            {
                g_system_state = SYSTEM_STATE_RING_IDLE;
            }
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
            // Before performing division, it is necessary to prevent division by zero (for example, when the system has just been initialized and max equals min).
            if ((g_current_cal_data.pedal1_max > g_current_cal_data.pedal1_min) &&
                (g_current_cal_data.pedal2_max > g_current_cal_data.pedal2_min))
            {
                // Calculate the physical travel ratio of APP1 (0 - 1000)
                int32_t travel_app1 = ((int32_t)adc_filtered_APP1 - (int32_t)g_current_cal_data.pedal1_min) * 1000 /
                                      ((int32_t)g_current_cal_data.pedal1_max - (int32_t)g_current_cal_data.pedal1_min);

                // Calculate the physical travel ratio of APP2 (0 - 1000)
                int32_t travel_app2 = ((int32_t)adc_filtered_APP2 - (int32_t)g_current_cal_data.pedal2_min) * 1000 /
                                      ((int32_t)g_current_cal_data.pedal2_max - (int32_t)g_current_cal_data.pedal2_min);

                // Limit the range to 0-1000 (to prevent pedal from hitting the physical limits, which would cause the calculation to result in a negative number or exceed 1000)
                if (travel_app1 < 0)    travel_app1 = 0;
                if (travel_app1 > 1000) travel_app1 = 1000;
                if (travel_app2 < 0)    travel_app2 = 0;
                if (travel_app2 > 1000) travel_app2 = 1000;

                // Check the deviation of the stroke between the two channels. If the deviation exceeds 10% (i.e., 100/1000), it is judged as rationality failure
                if (abs(travel_app1 - travel_app2) > 100)
                {
                    g_error_flag = 3;           // Plausibility check fail
                }
            }

            // Wait 1000ms after system boot before allowing self-learning.
            // This prevents the transient low voltage during sensor power-up from
            // being permanently caught as a false new minimum.
            if (HAL_GetTick() >= 1000)
            {
                if(adc_filtered_APP1 > ADC_OUT_OF_RANGE_MIN && adc_filtered_APP1 < ADC_OUT_OF_RANGE_MAX)//self-learning
                {
                    if(adc_filtered_APP1 > g_current_cal_data.pedal1_max)
                    {
                        g_current_cal_data.pedal1_max = adc_filtered_APP1;
                    }
                    if(adc_filtered_APP1 < g_current_cal_data.pedal1_min)
                    {
                        g_current_cal_data.pedal1_min = adc_filtered_APP1;
                    }
                }

                if(adc_filtered_APP2 > ADC_OUT_OF_RANGE_MIN && adc_filtered_APP2 < ADC_OUT_OF_RANGE_MAX)
                {
                    if(adc_filtered_APP2 > g_current_cal_data.pedal2_max)
                    {
                        g_current_cal_data.pedal2_max = adc_filtered_APP2;
                    }
                    if(adc_filtered_APP2 < g_current_cal_data.pedal2_min)
                    {
                        g_current_cal_data.pedal2_min = adc_filtered_APP2;
                    }
                }
            }

            if(calibration_save_flag == SET)
            {
                g_current_cal_data.magic = CALIBRATION_MAGIC;
                Save_Calibration_To_Flash();
                calibration_save_flag = 0;
            }

            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 0);
            break;
        case SYSTEM_STATE_INIT:
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 0);
            break;
        case SYSTEM_STATE_CALIBRATION:
        {
            // Wait 1000ms after entering calibration for APP sensors to stabilize at power-on.
            // Without this delay, the sensor's ramp-up transient (which can be much lower than
            // the true idle voltage) gets recorded as pedal_min and corrupts the calibration.
            static uint32_t cal_entry_tick = 0;
            if (cal_entry_tick == 0) cal_entry_tick = HAL_GetTick();

            if (HAL_GetTick() - cal_entry_tick >= 1000)
            {
                if(adc_filtered_APP1 > ADC_OUT_OF_RANGE_MIN)
                {
                    if(adc_filtered_APP1 > g_current_cal_data.pedal1_max)
                    {
                        g_current_cal_data.pedal1_max=adc_filtered_APP1;
                    }
                    if(adc_filtered_APP1 < g_current_cal_data.pedal1_min)
                    {
                        g_current_cal_data.pedal1_min=adc_filtered_APP1;
                    }
                }
                if(adc_filtered_APP2 > ADC_OUT_OF_RANGE_MIN)
                {
                    if(adc_filtered_APP2 > g_current_cal_data.pedal2_max)
                    {
                        g_current_cal_data.pedal2_max=adc_filtered_APP2;
                    }
                    if(adc_filtered_APP2 < g_current_cal_data.pedal2_min)
                    {
                        g_current_cal_data.pedal2_min=adc_filtered_APP2;
                    }
                }
            }

            if(calibration_save_flag==SET)
            {
                g_current_cal_data.magic = CALIBRATION_MAGIC;
                Save_Calibration_To_Flash();
                calibration_save_flag = 0;
                cal_entry_tick = 0; // Reset for next calibration session
            }
            break;
        }
        case SYSTEM_STATE_RING_IDLE:
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, g_current_cal_data.pedal1_min);
            /* DAC2 scale: compensate for 4.7k/10k ADC divider (÷10/14.7) and 1.51x output amp
             * Net factor = (14.7/10) / 1.51 = 14.7/15.1 ≈ 147/151 */
            dac2_value = (uint16_t)(((uint32_t)g_current_cal_data.pedal2_min * 147 + 75) / 151);
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac2_value);
            break;
        case SYSTEM_STATE_RING_ACTIVE:
            dac_out1 = map_accelerator_value(adc_filtered_RING, g_current_cal_data.pedal1_min, g_current_cal_data.pedal1_max);
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_out1);
            dac_out2 = map_accelerator_value(adc_filtered_RING, g_current_cal_data.pedal2_min, g_current_cal_data.pedal2_max);
            /* DAC2 scale: compensate for 4.7k/10k ADC divider (÷10/14.7) and 1.51x output amp
             * Net factor = (14.7/10) / 1.51 = 14.7/15.1 ≈ 147/151 */
            dac2_value = (uint16_t)(((uint32_t)dac_out2 * 147 + 75) / 151);
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac2_value);
            break;
        case SYSTEM_STATE_RING_BRAKE_OVERRIDE:
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, g_current_cal_data.pedal1_min);
            dac2_value = (uint16_t)(((uint32_t)g_current_cal_data.pedal2_min * 147 + 75) / 151);
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac2_value);
            break;
        case SYSTEM_STATE_ERROR:
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, g_current_cal_data.pedal1_min);
            dac2_value = (uint16_t)(((uint32_t)g_current_cal_data.pedal2_min * 147 + 75) / 151);
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac2_value);
            break;
    }
}