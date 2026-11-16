#include "functions.h"
#include "pico/stdio.h"
#include <stdio.h>

int main(void)
{
	uint8_t read_val = 0;
	uint8_t write_val = 0x5A; // 0101 1010
	uint8_t write_val2 = 0x7F; // 0111 1111
	stdio_init_all();
	uint16_t r_w_addr = 0x1234; // Ranges from 0x0000 -> 07FFF
	setup_i2c(i2c0, I2C_BITRATE_HZ, I2C0_SDA_PIN, I2C0_SCL_PIN);
	sleep_ms(100);
	if (write_byte(i2c0, 0x50, r_w_addr, write_val)) printf("Succesfully wrote data\n");
	else printf("Failed to write data\n");
	if (read_byte(i2c0, 0x50, r_w_addr, &read_val)) printf("Successfully read data %02x\n", read_val);
	else printf("Failed to read data!\n");
	if (write_byte(i2c0, 0x50, r_w_addr, write_val2)) printf("Succesfully wrote data\n");
	else printf("Failed to write data\n");
	if (read_byte(i2c0, 0x50, r_w_addr, &read_val)) printf("Successfully read data %02x\n", read_val);
	else printf("Failed to read data!\n");
	int reading = 1;
	while (reading)
	{
		sleep_ms(100);
	}
}
