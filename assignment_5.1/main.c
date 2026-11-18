#include "functions.h"
#include "pico/stdio.h"
#include <stdio.h>

int main(void)
{
	stdio_init_all();
	init_buttons(BTN_MASK);
	init_leds(LED_MASK);
	button btns[BTN_COUNT];

	test_eeprom_functions();
}
