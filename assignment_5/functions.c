#include "functions.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Initializes I2C, and SDA/SCL pins for I2C
void setup_i2c(i2c_inst_t *i2c, uint bitrate, uint8_t sda_pin, uint8_t scl_pin)
{
	i2c_init(i2c, bitrate);
	gpio_set_function(sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(scl_pin, GPIO_FUNC_I2C);
}

// writes a byte to EEPROM through I2C
// Returns true if byte was written, else false
bool write_byte(i2c_inst_t *i2c, uint8_t device_addr, uint16_t w_addr, uint8_t w_data)
{
	uint8_t msb = w_addr >> 8;
	uint8_t lsb = w_addr & 0xFF;
	uint8_t data_arr[DATA_ARR_LEN] = {msb, lsb, w_data};
	int returned_vals = i2c_write_blocking(i2c, device_addr, data_arr, DATA_ARR_LEN, false);
	if (returned_vals == DATA_ARR_LEN) sleep_ms(WR_SLEEP_TIME_MS); // Wait for write operation to be completed
	return returned_vals == DATA_ARR_LEN; // True if all were written, else false
}

bool read_byte(i2c_inst_t *i2c, uint8_t device_addr, uint16_t w_addr, uint8_t w_data);
