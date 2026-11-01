//
// Created by niilo on 11/1/2026.
//

#ifndef PICO_ASSIGNMENTS_FUNCTIONS_H
#define PICO_ASSIGNMENTS_FUNCTIONS_H

#include "pico/stdlib.h"

#define OPT_SENSOR_GPIO 28
#define STEP_MOTOR_A 2 // ln1 = A
#define STEP_MOTOR_B 3 // ln2 = B
#define STEP_MOTOR_C 6 // ln3 = C
#define STEP_MOTOR_D 13 // ln4 = D
// Bit mask for GPIO initialization
#define STEP_MOTOR_MASK ((1u << STEP_MOTOR_A) | (1u <<STEP_MOTOR_B) | (1u <<STEP_MOTOR_C) | (1u <<STEP_MOTOR_D))
#define MOTOR_CW 1 // Spins motor Clockwise
#define MOTOR_CCW 7 // Spins motor Counter Clock Wise
#define MOTOR_SLEEP_TIME_MS 1

#define STEPS_IN_SEQ 8 // steps in half step sequence
#define CMD_BUF 20
#define WORD_BUF (CMD_BUF - 1)
// Upper limit for N, steps_per_rev = 4096, MOTOR_SLEEP_TIME_MS = 1ms
// Max runtime = MAX_N * (steps_per_rev/8) * MOTOR_SLEEP_TIME_MS
// 117 * (4096 / 8) * 1 = 59 904 ms, about 1 min
#define MAX_N 117
#define FULL_ROTATION 8 // 8/8 = full rotation
// Stepper Motors Data
typedef struct
{
	bool calibrated; // calibrated
	uint32_t steps_per_rev;
	uint32_t direction; // 1 CW or 7 CCW
	uint8_t sequence_index; // To store sequence index after calibration

} motor_state;

typedef enum
{
	CMD_STATUS,
	CMD_CALIB,
	CMD_RUN_N
} command_t;

typedef enum
{
	OK,
	UNKNOWN_CMD,
	N_NOT_IN_RANGE,
	FAULTY_NUM,
	LINE_TOO_LONG,
	EMPTY_INPUT
} cmd_parse_res_t;

uint32_t take_step(uint32_t seq_index, uint8_t direction);
uint32_t binary_map(uint8_t current_state);
void check_status(const motor_state *motor_status);
void calibrate(motor_state *motor_status, uint8_t direction);
void run_n(motor_state *motor_status, int N, uint32_t direction);
uint32_t calc_avg (uint32_t total, uint8_t count);
cmd_parse_res_t read_command(command_t *command, int *number);
cmd_parse_res_t valid_number(char *line_buffer, int offset, int *number) ;
void print_start(void);


#endif //PICO_ASSIGNMENTS_FUNCTIONS_H