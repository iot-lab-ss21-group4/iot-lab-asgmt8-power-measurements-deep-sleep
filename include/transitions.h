#ifndef TRANSITIONS_H
#define TRANSITIONS_H

#include "stdlib.h"
#include "stdint.h"
#include "esp_log.h"

int transition_handling(uint8_t state_change);
void print_sensor_and_fsm_state(uint8_t level_inner, uint8_t level_outer);

#define OUTER_STATE_CHANGE (1 << 0)
#define INNER_STATE_CHANGE (1 << 1)

#endif
