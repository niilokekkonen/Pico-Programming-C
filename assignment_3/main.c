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

static uint32_t take_step(uint32_t seq_index);
uint32_t binary_map(uint8_t current_state);
void status();
void calibrate();

int main(void)
{
	// Initializing OPTO fork, and stepper motor pins
	gpio_init(OPT_SENSOR_GPIO);
	gpio_pull_up(OPT_SENSOR_GPIO);
	gpio_init_mask(STEP_MOTOR_MASK);

	gpio_set_dir(OPT_SENSOR_GPIO, GPIO_IN); // Opto fork is input
	gpio_set_dir_out_masked(STEP_MOTOR_MASK); // Stepper motor mask is output.

	bool calibrated = false; // Set to calibrated after calib mode
	uint32_t step_count = 0;
	uint32_t seq_index = 0; // Sequence index


	return 0;
}

// Moves the motor one half step
// updates + returns updated sequence index
static uint32_t take_step(uint32_t seq_index)
{
	uint8_t cur_state = half_step_arr[seq_index];
	seq_index = (seq_index + 1) % STEPS_IN_SEQ; // Growing index, if i = 8 wrap to zero
	uint32_t mapped_mask = binary_map(cur_state);
	gpio_put_masked(STEP_MOTOR_MASK, mapped_mask);
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
	return mapped_mask;
}


void status()
{

}

void calibrate()
{

}