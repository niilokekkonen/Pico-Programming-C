//
// Created by niilo on 11/1/2026.
//
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "functions.h"
#include "pico/stdlib.h"
static void empty_lf(void);
static bool rest_is_empty(const char *start_addr);
static const uint8_t half_step_arr[STEPS_IN_SEQ] = {
	0b1000, // A = 1. Step 1
	0b1100, // A, B = 1. Step 2
	0b0100, // B = 1. Step 3
	0b0110, // B,C = 1. Step 4
	0b0010, // C = 1. Step 5
	0b0011, // C, D = 1. Step 6
	0b0001, // D = 1. Step 7
	0b1001, // A, D = 1. Step 8
};

// Moves the motor one half step
// updates + returns updated sequence index
uint32_t take_step(uint32_t seq_index, uint8_t direction)
{
	uint8_t cur_state = half_step_arr[seq_index];
	// Looping index, direction = 1 or 7, depending on direction
	seq_index = (seq_index + direction) % STEPS_IN_SEQ;
	uint32_t mapped_mask = binary_map(cur_state);
	gpio_put_masked(STEP_MOTOR_MASK, mapped_mask);
	sleep_ms(MOTOR_SLEEP_TIME_MS);
	return seq_index;
}

// Current state = 4 bit state. MSB = A, LSB = D
// maps the GPIO pins to the A,B,C,D states in each step
// Returns mapped mask
uint32_t binary_map(const uint8_t current_state)
{
	uint32_t mapped_mask = 0;
	if (current_state & 0x8) // 0b1000 A is on
	{
		mapped_mask |= 1u << STEP_MOTOR_A;
	}
	if (current_state & 0x4) // 0b0100 B is on
	{
		mapped_mask |= 1u << STEP_MOTOR_B;
	}
	if (current_state & 0x2) // 0b0010 C is on
	{
		mapped_mask |= 1u << STEP_MOTOR_C;
	}
	if (current_state & 0x1) // 0b0001 D is on
	{
		mapped_mask |= 1u << STEP_MOTOR_D;
	}
	// Return the combined mask
	return mapped_mask;
}

// Checks motor status. Takes a pointer to struct
void check_status(const motor_state *motor_status)
{
	if (motor_status->calibrated)
	{
		printf("Calibrated\n");
		printf("Steps per revolution: %u\n", motor_status->steps_per_rev);
	}
	else
	{
		printf("Not calibrated\n");
		printf("Not available\n");
	}

}
// Calibrates the stepper motor, direction = 1 ClockWise or 7 CounterClockWise
void calibrate(motor_state *motor_status, uint8_t direction)
{
	bool measuring = false;
	uint8_t revolutions = 0;
	uint32_t seq_index= motor_status->sequence_index; // sequence index
	uint32_t steps_per_rev = 0;
	uint32_t total_steps = 0;
	uint32_t revolution_avg = 0;
	bool prev_sensor_state = gpio_get(OPT_SENSOR_GPIO);
	printf("Starting calibration\n");
	// Measure for 1 'trigger' falling edge, and 3 'end measurement'
	while (revolutions < 3)
	{
		seq_index = take_step(seq_index, direction); // Turning motor
		bool cur_sensor_state = gpio_get(OPT_SENSOR_GPIO);
		if (measuring)
		{
			steps_per_rev++;
		}
		// If previous was high, and the current state is low = FALLING EDGE
		if (FORK_FALLING_EDGE)
		{
			if (!measuring)
			{
				measuring = true;
			}
			else // Measuring = true
			{
				total_steps += steps_per_rev;
				steps_per_rev = 0;
				revolutions++;
				printf("steps: %u\n", total_steps);
			}
		}
		prev_sensor_state = cur_sensor_state;
	}
	// Calculating average from 3 revolutions to get a more accurate result
	revolution_avg = calc_avg(total_steps, revolutions);
	printf("Total average: %u\n", revolution_avg);
	motor_status->steps_per_rev = revolution_avg;
	motor_status->calibrated = true;
	motor_status->sequence_index = seq_index;
	printf("Calibration complete\n");
}


// Calculates the average
uint32_t calc_avg (const uint32_t total, const uint8_t count)
{
	if (count != 0) return total / count; // prevent division by zero
	return total;
}

// runs the motor for N * 1/8 of a revolution.
// Saves last index to struct via ptr
void run_n(motor_state *motor_status, int N, uint32_t direction)
{
	uint32_t max_steps = motor_status->steps_per_rev; // Steps per rev approx 4096
	uint8_t seq_index = motor_status->sequence_index;
	if (N != 0)
	{

		uint32_t drive_motor = max_steps * N / 8;
		while (drive_motor != 0)
		{
			seq_index = take_step(seq_index, direction);
			drive_motor--;
		}
	}
	motor_status->sequence_index = seq_index;
}

void print_start(void)
{
    printf("Enter calib to calibrate\nor\nstatus to check state\n");
    printf("or run N to run the motor for N * 1/8 of a revolution\n");
}

// Returns enum of what happened during reading and verifying cmd
// Writes command to *command and number to *number
cmd_parse_res_t read_command(command_t *command, int *number)
{
    char line_buffer[CMD_BUF];
    char cmd_buffer[WORD_BUF];
    int offset = 0;
    if (fgets(line_buffer, CMD_BUF, stdin) != NULL)
    {
        if (!strchr(line_buffer, '\n'))
        {
            empty_lf();
            return LINE_TOO_LONG;
        }
        if (sscanf(line_buffer, "%18s%n", cmd_buffer, &offset) == 1)
        {
            if (!strcmp(cmd_buffer, "status"))
            {
                *command = CMD_STATUS;
                return OK;
            }
            if (!strcmp(cmd_buffer, "calib"))
            {
                *command = CMD_CALIB;
                return OK;
            }
            if (!strcmp(cmd_buffer, "run"))
            {
                cmd_parse_res_t result = valid_number(line_buffer, offset, number);
                if(result == OK)
                {
                    *command = CMD_RUN_N;
                    return OK;
                }
                return result;
            }
        }
        return UNKNOWN_CMD; // sscanf fails to read command, or command is not one of the predefined ones
    }
    return EMPTY_INPUT; // fgets fails
}

// Checks if the rest of a string is empty or not.
// Starts from *start_addr
// Returns true if empty, false if not.
bool rest_is_empty(const char *start_addr)
{
    while (isspace((unsigned char)*start_addr) != 0) start_addr++;
    return *start_addr == '\0';
}

// Empties linefeed
void empty_lf(void)
{
    int getchar_res = getchar();
    while (getchar_res != '\n' && getchar_res != EOF) getchar_res = getchar();
}

// Validates if a number exists
// returns the result of validation.
cmd_parse_res_t valid_number(char *line_buffer, int offset, int *number)
{
    char *end_ptr = &line_buffer[offset]; // Initial value
    long user_number = 0;
    if(rest_is_empty(&line_buffer[offset]))
    {
        *number = FULL_ROTATION; // command is just 'run', turn the motor by one revolution
        return OK;
    }
	 // Use strtol *end_ptr to see how the number is formatted
    user_number = strtol(&line_buffer[offset], &end_ptr, 10);
    if (&line_buffer[offset] == end_ptr) return FAULTY_NUM; // Command = run xyz or similar
    if (rest_is_empty(end_ptr))
    {
        if (user_number < 0 || user_number > MAX_N) return N_NOT_IN_RANGE;
        *number = (int) user_number;
        return OK; // command is 'run N', where N = integer
    }
    return FAULTY_NUM; // Command is 'run 8abc' or similar = !(Rest_is_empty)
}