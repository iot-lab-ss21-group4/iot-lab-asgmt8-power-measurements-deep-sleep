#include "transitions.h"

#define MIN_ROOM_COUNT 0
// assuming that count_display_q_item is an unsigned type
#define MAX_ROOM_COUNT ((uint8_t)(-1))
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
    ((byte)&0x80 ? '1' : '0'),     \
        ((byte)&0x40 ? '1' : '0'), \
        ((byte)&0x20 ? '1' : '0'), \
        ((byte)&0x10 ? '1' : '0'), \
        ((byte)&0x08 ? '1' : '0'), \
        ((byte)&0x04 ? '1' : '0'), \
        ((byte)&0x02 ? '1' : '0'), \
        ((byte)&0x01 ? '1' : '0')
#define FIRST_N_BITMASK(n) ((1 << n) - 1)
#define POWER_OF_TWO(x) (1 << (x))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define LOGICAL_NOT(c) ((c) ? false : true)
#define STATIC_ASSERT(COND, MSG) typedef char static_assertion_##MSG[(COND) ? 1 : -1]
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define FLAG_COUNT 2 // number of barrier bits (flags)
#define ESP_INTR_FLAG_DEFAULT 0

#define STATE_IDENTIFIER_LENGTH (2 * FLAG_COUNT)
// Transitions
#define T_0000_0001 1
#define T_0000_0010 2
#define T_0001_0100 20
#define T_0001_0111 23
#define T_0010_1011 43
#define T_0010_1000 40
#define T_0100_0001 65
#define T_0100_0010 66
#define T_0111_1110 126
#define T_0111_1101 125
#define T_1011_1110 190
#define T_1011_1101 189
#define T_1000_0001 129
#define T_1000_0010 130
#define T_1110_1011 235
#define T_1110_1000 232
#define T_1101_0100 212
#define T_1101_0111 215

// Impossible states
#define S_IMP_0000 0 //only allowed as init state
#define S_IMP_0011 3
#define S_IMP_0101 5
#define S_IMP_0110 6
#define S_IMP_1001 9
#define S_IMP_1010 10
#define S_IMP_1111 15
#define S_IMP_1100 12

// assuming that FSM state is defined by 4 bits
typedef uint8_t fsm_transition;
STATIC_ASSERT(
    sizeof(fsm_transition) * 8 >= 2 * STATE_IDENTIFIER_LENGTH,
    sizeof_fsm_transition_is_big_enough);

static uint8_t FSM_STATE = 0;

static const char *TAG = "FSM";

static void apply_state_change(uint8_t state_change)
{
	uint8_t prev_new_bits = FSM_STATE & FIRST_N_BITMASK(FLAG_COUNT);
    // Replace old state bits with new state bits.
    FSM_STATE = (FSM_STATE << FLAG_COUNT) & FIRST_N_BITMASK(2 * FLAG_COUNT);
    // XOR with changed state bits (1 for changed 0 for unchanged).
    FSM_STATE ^= prev_new_bits ^ (state_change & FIRST_N_BITMASK(FLAG_COUNT));
}

void detect_impossible_state()
{
    switch (FSM_STATE)
    {
    case S_IMP_0000: //call method only after first signal
    case S_IMP_0011:
    case S_IMP_0101:
    case S_IMP_0110:
    case S_IMP_1001:
    case S_IMP_1010:
    case S_IMP_1111:
    case S_IMP_1100:
        ESP_LOGE(TAG, "Inside impossible state: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(FSM_STATE));
        break;
    default:
        break;
    }
}

int transition_handling(uint8_t state_change)
{

	fsm_transition dangling_transition = FSM_STATE << STATE_IDENTIFIER_LENGTH;
	apply_state_change(state_change);
	detect_impossible_state();
	fsm_transition transition = dangling_transition | FSM_STATE;
	int count = 0;
	switch (transition)
	{
	case T_0111_1110:
		return 1;
		count = (count < MAX_ROOM_COUNT) ? count + 1 : count;
		break;
	case T_1011_1101:
		count = (count > MIN_ROOM_COUNT) ? count - 1 : count;
		return -1;
		break;
	default:
		return 0;
		break;
	}
	return 0;
}

void print_sensor_and_fsm_state(uint8_t level_inner, uint8_t level_outer)
{
    ESP_LOGI(TAG, "Sensor state: " BYTE_TO_BINARY_PATTERN ", FSM state: " BYTE_TO_BINARY_PATTERN,
             BYTE_TO_BINARY((level_inner * INNER_STATE_CHANGE) |
                            (level_outer * OUTER_STATE_CHANGE)),
             BYTE_TO_BINARY(FSM_STATE));
}
