#include "functions.h"
#include "pico/stdio.h"
#include <stdio.h>

int main(void)
{
	ledstate state_of_LEDS;
	stdio_init_all();
	test_eeprom_functions();
	printf("Size of struct %d" , sizeof(state_of_LEDS));
}
