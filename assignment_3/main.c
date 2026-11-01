#include "pico/stdlib.h"
#include "functions.h"
#include <stdio.h>

int main(void)
{
	// Initializing OPTO fork, and stepper motor pins
	stdio_init_all();
	gpio_init(OPT_SENSOR_GPIO);
	gpio_pull_up(OPT_SENSOR_GPIO);
	gpio_init_mask(STEP_MOTOR_MASK);

	gpio_set_dir(OPT_SENSOR_GPIO, GPIO_IN); // Opto fork is input
	gpio_set_dir_out_masked(STEP_MOTOR_MASK); // Stepper motor mask is output.
	command_t command = CMD_STATUS; // Var to read command into
	int usr_num = 0;
	motor_state motor_status = {
		.calibrated = false,
		.steps_per_rev = 0,
		.direction = MOTOR_CW,
		.sequence_index = 0
	}; // Start the program with motor not calibrated.
	print_start();
	while (1)
	{
		printf("> ");
		cmd_parse_res_t parsing_res = read_command(&command, &usr_num);
		switch (parsing_res)
		{
			case OK:
				switch (command)
				{
					case CMD_STATUS:
						check_status(&motor_status);
						break;
					case CMD_CALIB:
						calibrate(&motor_status, motor_status.direction);
						break;
					case CMD_RUN_N:
						if (!motor_status.calibrated)
						{
							printf("You need to calibrate first\n");
							break;
						}
						printf("Spinning motor for %d/8 of a revolution\n", usr_num);
						run_n(&motor_status, usr_num, motor_status.direction);
						break;
				}
			break;
			case N_NOT_IN_RANGE:
				printf("Entered number not in range %d - %d\n", 0, MAX_N);
				break;
			case UNKNOWN_CMD:
				printf("Unknown command\n");
				printf("Try status, calib, or run N (N is an integer)\n");
				break;
			case FAULTY_NUM:
				printf("Faulty number\n");
				break;
			case LINE_TOO_LONG:
				printf("Line too long\n");
				break;
			case EMPTY_INPUT:
				printf("Empty input\n");
				break;
		}
	}
	return 0;
}