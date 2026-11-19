#include "functions.h"
#include "pico/stdio.h"
#include <stdio.h>

int main(void)
{
	stdio_init_all();
	setup_i2c(i2c0, I2C_BITRATE_HZ, I2C0_SDA_PIN, I2C0_SCL_PIN);
	init_buttons(BTN_MASK);
	init_leds(LED_MASK);
	button btns[BTN_COUNT];
	uint8_t btn_pins[BTN_COUNT] = {SW0_PIN, SW1_PIN, SW2_PIN};
	uint8_t led_state;
	bool changed = false;
	int i = 0;
	// Initializing buttons
	for (i = 0; i < BTN_COUNT; i++)
	{
		init_button_state(&btns[i], btn_pins[i], BTN_DEBOUNCE_MS);
	}
	if (!load_led_state(&led_state)) led_state = 0b010;
	set_leds(led_state,LED_COUNT);
	uint64_t start_time = time_us_64();
	printf("Start time %llus\nLed state %02X\n", start_time / 1000000, led_state);
	while (true)
	{
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
			printf("Time since startup %llus\nLed state %02X\n", (time_us_64() - start_time) / 1000000, led_state);
			set_leds(led_state, LED_COUNT);
			bool state_saved = save_led_state(led_state);
			if (!state_saved) printf("Failed to save LED state\n");
			changed = false; // changes saved
		}
	}
}
