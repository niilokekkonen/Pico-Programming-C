#include "functions.h"
#include "pico/stdio.h"
#include <stdio.h>

int main(void)
{
	stdio_init_all();
	init_buttons(BTN_MASK);
	init_leds(LED_MASK);
	setup_i2c(i2c0, I2C_BITRATE_HZ, I2C0_SDA_PIN, I2C0_SCL_PIN);
	button btns[BTN_COUNT];

	test_eeprom_functions();
}
