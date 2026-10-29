#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define OPT_SENSOR_GPIO 28
#define STEP_MOTOR_A 2 // ln1 = A
#define STEP_MOTOR_B 3 // ln2 = B
#define STEP_MOTOR_C 6 // ln3 = C
#define STEP_MOTOR_D 13 // ln4 = D
// Bit mask for GPIO initialization
#define STEP_MOTOR_MASK ((1u << STEP_MOTOR_A) | (1u <<STEP_MOTOR_B) | (1u <<STEP_MOTOR_C) | (1u <<STEP_MOTOR_D))
#define MOTOR_CW 1 // Spins motor Clockwise
#define MOTOR_CCW 7 // Spins motor Counter Clock Wise
#define MOTOR_SLEEP_TIME 1// in ms

#define STEPS_IN_SEQ 8 // steps in half step sequence
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

// Stepper Motors Status
typedef struct
{
	bool calibrated; // calibrated
	uint32_t steps_per_rev;
	uint32_t direction;
	uint8_t sequence_index; // To store sequence index after calibration

} status;

static uint32_t take_step(uint32_t seq_index, uint8_t direction);
uint32_t binary_map(uint8_t current_state);
void check_status(const status *motor_status);
void calibrate(status *motor_status, uint8_t direction);
void run_n(const status *motor_status, int N);
uint32_t calc_avg (uint32_t total, uint8_t count);

int main(void)
{
	// Initializing OPTO fork, and stepper motor pins
	stdio_init_all();
	gpio_init(OPT_SENSOR_GPIO);
	gpio_pull_up(OPT_SENSOR_GPIO);
	gpio_init_mask(STEP_MOTOR_MASK);

	gpio_set_dir(OPT_SENSOR_GPIO, GPIO_IN); // Opto fork is input
	gpio_set_dir_out_masked(STEP_MOTOR_MASK); // Stepper motor mask is output.

	status motor_status = {
		.calibrated = false,
		.steps_per_rev = 0,
		.direction = MOTOR_CW,
		.sequence_index = 0
	}; // Start the program with motor not calibrated.
	check_status(&motor_status);
	calibrate(&motor_status, motor_status.direction);
	check_status(&motor_status);
	return 0;
}

// Moves the motor one half step
// updates + returns updated sequence index
static uint32_t take_step(uint32_t seq_index, uint8_t direction)
{
	uint8_t cur_state = half_step_arr[seq_index];
	// Looping index, direction = 1 or 7, depending on direction
	seq_index = (seq_index + direction) % STEPS_IN_SEQ;
	uint32_t mapped_mask = binary_map(cur_state);
	gpio_put_masked(STEP_MOTOR_MASK, mapped_mask);
	sleep_ms(MOTOR_SLEEP_TIME); // sleep for 1 ms
	return seq_index;
}

// Takes the current state
// maps the GPIO pins to the half_step A,B,C,D states
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
// Doesn't modify struct itself
void check_status(const status *motor_status)
{
	if (motor_status->calibrated)
	{
		printf("Calibrated\n");
		printf("Steps per revolution: %u\n", motor_status->steps_per_rev);
	}
	else
	{
		printf("Not available\n");
	}

}
// Calibrates the stepper motor, direction = 1 ClockWise or 7 CounterClockWise
void calibrate(status *motor_status, uint8_t direction)
{
	// Calibrate only if necessary
	if (!motor_status->calibrated)
	{
		bool measuring = false;
		uint8_t revolutions = 0;
		uint32_t seq_index= 0; // sequence index
		uint32_t steps_per_rev = 0;
		uint32_t total_steps = 0;
		uint32_t revolution_avg = 0;
		bool prev_sensor_state = gpio_get(OPT_SENSOR_GPIO);
		while (revolutions < 3)
		{

			seq_index = take_step(seq_index, direction); // Turning motor
			bool cur_sensor_state = gpio_get(OPT_SENSOR_GPIO);
			if (measuring)
			{
				steps_per_rev++;
			}
			// If previous was high, and the current state is low = FALLING EDGE
			if (prev_sensor_state && !cur_sensor_state)
			{
				if (!measuring)
				{
					measuring = true;
					steps_per_rev = 0;
				}
				else
				{
					measuring = false;
					total_steps += steps_per_rev;
					revolutions++;
				}
			}
			prev_sensor_state = cur_sensor_state;
		}
		// Calculating average from 3 revolutions to get a more accurate result
		revolution_avg = calc_avg(total_steps, revolutions);
		motor_status->steps_per_rev = revolution_avg;
		motor_status->calibrated = true;
		motor_status->sequence_index = seq_index;
	}
}

uint32_t calc_avg (const uint32_t total, const uint8_t count)
{
	if (count != 0) return total / count; // prevent division by zero
	return total;
}

// runs the motor for N * 1/8 of a revolution.
void run_n(const status *motor_status, int N)
{
	uint32_t max_steps = motor_status->steps_per_rev; // Steps per rev approx 4096
	uint8_t seq_index = motor_status->sequence_index;
	if (N != 0)
	{
		uint32_t  run_for =  max_steps * N / 8;
		while (1)
		{
			/* TODO: Add proper calculations for loop length,
				 Upper limit for N to not go above 32 bit digits or run for too long incase of a typo.
				 Read command, N in Main loop.
			*/
		}
	}
	if (N == 8)
	{

	}
}