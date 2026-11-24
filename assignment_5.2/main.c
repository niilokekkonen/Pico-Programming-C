#include "eeprom_log.h"
#include "pico/stdio.h"
#include <stdio.h>
#include <string.h>

// Builds the status message once, prints it and writes it to the log.
// label = text before the state, e.g. "Boot" or "Change"
static void report(const char *label, uint8_t led_state)
{
	char msg[LOG_MSG_BUF];
	unsigned seconds = (unsigned) (time_us_64() / 1000000);
	int len = snprintf(msg, sizeof(msg), "%s %us LEDs %d%d%d", label, seconds,
					   (led_state >> 0) & 1, (led_state >> 1) & 1, (led_state >> 2) & 1);
	if (len <= 0 || (size_t) len >= sizeof(msg))
	{
		printf("Message build failed or too long\n");
		return;
	}
	printf("%s\n", msg);
	if (!log_write((const uint8_t *) msg, (size_t) len)) printf("Failed to write log\n");
}

int main(void)
{
	stdio_init_all();
	sleep_ms(1000); // Time for terminal to open
	setup_i2c(i2c0, I2C_BITRATE_HZ, I2C0_SDA_PIN, I2C0_SCL_PIN);
	init_buttons(BTN_MASK);
	init_leds(LED_MASK);
	button btns[BTN_COUNT];
	uint8_t btn_pins[BTN_COUNT] = {SW0_PIN, SW1_PIN, SW2_PIN};
	uint8_t led_state;
	bool changed = false;
	char cmd_line[CMD_BUF_SIZE];
	size_t cmd_idx = 0;
	bool cmd_overflow = false;
	int i = 0;

	for (i = 0; i < BTN_COUNT; i++)
	{
		init_button_state(&btns[i], btn_pins[i], BTN_DEBOUNCE_MS);
	}

	// No valid state in EEPROM: load_led_state gives the default (middle LED), save it
	if (!load_led_state(&led_state))
	{
		if (!save_led_state(led_state)) printf("Failed to save default LED state\n");
	}
	set_leds(led_state, LED_COUNT);
	report("Boot", led_state);
	printf("Commands: read, erase\n");
	while (true)
	{
		// Buttons
		for (i = 0; i < BTN_COUNT; i++)
		{
			if (button_pressed(&btns[i], BTN_DEBOUNCE_MS))
			{
				led_state ^= 1 << i;
				changed = true;
			}
		}
		if (changed)
		{
			set_leds(led_state, LED_COUNT);
			if (!save_led_state(led_state)) printf("Failed to save LED state\n");
			report("Change", led_state);
			changed = false;
		}

		// Commands
		if (poll_line(cmd_line, CMD_BUF_SIZE, &cmd_idx, &cmd_overflow))
		{
			command_t cmd = CMD_NONE;
			if (cmd_overflow)
			{
				printf("Command too long\n");
				cmd_overflow = false;
			}
			else if (parse_command(cmd_line, &cmd) == CMD_PARSE_OK)
			{
				if (cmd == CMD_READ) log_print_all();
				else if (cmd == CMD_ERASE)
				{
					if (erase_log()) printf("Log erased\n");
					else printf("Failed to erase log\n");
				}
			}
			else printf("Unknown command. Use: read, erase\n");
		}
	}
}