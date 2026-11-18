#include "functions.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include <string.h>

// Initializes I2C, and SDA/SCL pins for I2C
void setup_i2c(i2c_inst_t *i2c, uint bitrate, uint8_t sda_pin, uint8_t scl_pin)
{
	i2c_init(i2c, bitrate);
	gpio_set_function(sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(scl_pin, GPIO_FUNC_I2C);
}

// Inits GPIO pins as output
void init_leds(uint32_t led_mask)
{
	// Initialize LED pins with mask
	gpio_init_mask(led_mask);
	gpio_set_dir_masked(led_mask, led_mask);
}

// Inits GPIO pins as pull_up input
void init_buttons(uint32_t btn_mask)
{
	gpio_init_mask(btn_mask);
	gpio_set_dir_in_masked(btn_mask);
	pull_up();
}
// Pulls up 3 GPIO pins
void pull_up(void)
{
	gpio_pull_up(SW0_PIN);
	gpio_pull_up(SW1_PIN);
	gpio_pull_up(SW2_PIN);
}

// writes a byte to EEPROM through I2C
// Returns true if byte was written, else false
bool write_byte(i2c_inst_t *i2c, uint8_t device_addr, uint16_t w_addr, uint8_t w_data)
{
	uint8_t msb = w_addr >> 8;
	uint8_t lsb = w_addr & 0xFF;
	uint8_t data_arr[W_DATA_ARR_LEN] = {msb, lsb, w_data};
	int returned_vals = i2c_write_blocking(i2c, device_addr, data_arr, W_DATA_ARR_LEN, false);
	if (returned_vals == W_DATA_ARR_LEN) sleep_ms(WR_SLEEP_TIME_MS); // Wait for write operation to be completed
	return returned_vals == W_DATA_ARR_LEN; // True if all were written, else false
}

// Reads a byte from EEPROM through I2C
// Returns true if read was successful, else false
// Saves read data to *r_data
bool read_byte(i2c_inst_t *i2c, uint8_t device_addr, uint16_t r_addr, uint8_t *r_data)
{
	uint8_t msb = r_addr >> 8;
	uint8_t lsb = r_addr & 0xFF;
	uint8_t data_arr[R_ADDR_LEN] = {msb, lsb};
	// First open I2C connection with write, but without data
	int returned_vals = i2c_write_blocking(i2c, device_addr, data_arr, R_ADDR_LEN, true);
	if (returned_vals != R_ADDR_LEN) return false; // Couldn't open connection
	int read_vals = i2c_read_blocking(i2c, device_addr, r_data, READ_LEN, false);
	return read_vals == READ_LEN;
}

// Reads n amount of bytes from EEPROM through I2C
// Returns true if read was successful, else false
// Saves read data to dest_arr[dest_len], dest_len = amount of bytes to read
bool read_n_bytes(i2c_inst_t *i2c, uint8_t device_addr, uint16_t r_addr, uint8_t *dest_arr, size_t dest_len)
{
	if (dest_arr == NULL)	return false; // Necessary safety measures
	if (dest_len == 0)	return false;
	if ((uint32_t) r_addr + dest_len > EEPROM_BYTES) return false;
	uint8_t msb = r_addr >> 8;
	uint8_t lsb = r_addr & 0xFF;
	uint8_t addr_buf[R_ADDR_LEN] = {msb, lsb};
	int returned_vals = i2c_write_blocking(i2c, device_addr, addr_buf, R_ADDR_LEN, true);
	if (returned_vals != R_ADDR_LEN) return false; // Couldn't open connection
	int read_vals = i2c_read_blocking(i2c, device_addr, dest_arr, dest_len, false);
	return (size_t) read_vals == dest_len;
}


// Writes n amount of bytes to EEPROM through I2C, src_len = amount of bytes
// Returns true if write was successful
// Returns false if writing stops abruptly, src_arr is NULL or len is zero
// Writes values from src_arr to EEPROM
bool write_n_bytes(i2c_inst_t *i2c, uint8_t device_addr, uint16_t w_addr, const uint8_t *src_arr, size_t src_len)
{
	if (src_arr == NULL)	return false; // Necessary safety measures
	if (src_len == 0)	return false;
	if ((uint32_t) w_addr + src_len > EEPROM_BYTES) return false;
	while (src_len > 0)
	{
		uint8_t msb = w_addr >> 8;
		uint8_t lsb = w_addr & 0xFF;
		uint16_t space_left = EEPROM_PAGESIZE - w_addr % EEPROM_PAGESIZE;
		// if src_len is smaller than the space left, write src_len amount of bytes, else space_left amount.
		size_t chunk_size = src_len < space_left ? src_len : space_left;
		uint8_t data_chunk[EEPROM_PAGESIZE + 2] = {msb, lsb};
		memcpy(data_chunk + 2, src_arr, chunk_size);
		// chunk_size + 2 is for the msb/lsb
		int bytes_written = i2c_write_blocking(i2c, device_addr, data_chunk, chunk_size + 2, false);
		if ((size_t) bytes_written != chunk_size + 2) return false;
		sleep_ms(WR_SLEEP_TIME_MS);
		src_len -= chunk_size;
		w_addr += chunk_size;
		src_arr += chunk_size;
	}
	return true;
}

// sets led_state in struct
void set_led_state(ledstate *ls, uint8_t value)
{
	ls->state = value;
	ls->not_state = ~value;
}

// Checks ledstate from ledstate struct
bool led_state_is_valid(ledstate *ls)
{
	return ls->state == (uint8_t) ~ls->not_state;
}
// Sets GPIO pins according to led_state variable
void set_leds(uint8_t led_state, uint8_t led_count)
{
	for (int i = 0; i < led_count; i++)
	{
		uint8_t new_bit = (led_state >> i) & 1;
		gpio_put(LED_START + i, new_bit);
	}
}
// Loads LED state from EEPROM to *led_state
// Returns true if successfully read the LED state
// else false
bool load_led_state(uint8_t *led_state)
{
	ledstate temp_state;
	bool read_ok = read_n_bytes(i2c0, EEPROM_ADDR, LED_STATE_ADDR, (uint8_t *) &temp_state, sizeof(temp_state));
	if (read_ok)
	{
		bool valid_state = led_state_is_valid(&temp_state);
		if (valid_state)
		{
			*led_state = temp_state.state & 0x07; // Leave the last three bits to the result
			return true;
		}
		return false;
	}
	return false;
}

void init_button_state(button *btn, uint8_t pin, uint32_t debounce_ms)
{
	btn->btn_pin = pin;
	btn->last_accepted_time_us = time_us_32() - debounce_ms * 1000;
	btn->last_state = true; // Not pressed
}

bool button_pressed(button *btn, uint32_t debounce_ms)
{
	bool cur_state = gpio_get(btn->btn_pin);
	uint32_t now = time_us_32();
	if (cur_state == btn->last_state) return false; // No changes
	if (now - btn->last_accepted_time_us < debounce_ms * 1000) return false;
	btn->last_state = cur_state;
	btn->last_accepted_time_us = now;
	return !cur_state;
}

// Runs all the different eeprom functions.
void test_eeprom_functions(void)
{
	uint8_t read_val = 0;
	uint8_t write_val = 0x5A; // 0101 1010
	uint8_t write_val2 = 0x7F; // 0111 1111
	uint8_t write_bytes[3] = {0x5F, 0x67, 0xFF};
	uint8_t byte_buffer[64] = {0};
	uint16_t r_w_addr = 0x1234; // Ranges from 0x0000 -> 07FFF
	uint16_t write_n_addr = 0x3345;
	sleep_ms(100);
	if (write_byte(i2c0, EEPROM_ADDR, r_w_addr, write_val)) printf("Succesfully wrote data\n");
	else printf("Failed to write data\n");
	if (read_byte(i2c0, EEPROM_ADDR, r_w_addr, &read_val)) printf("Successfully read data %02X\n", read_val);
	else printf("Failed to read data!\n");
	if (write_byte(i2c0, EEPROM_ADDR, r_w_addr, write_val2)) printf("Succesfully wrote data\n");
	else printf("Failed to write data\n");
	if (read_byte(i2c0, EEPROM_ADDR, r_w_addr, &read_val)) printf("Successfully read data %02X\n", read_val);
	else printf("Failed to read data!\n");
	if (write_n_bytes(i2c0, EEPROM_ADDR,write_n_addr, write_bytes, 3)) printf("Successfully written data\n");
	else printf("Failed to write data\n");
	if (read_n_bytes(i2c0, EEPROM_ADDR, write_n_addr, byte_buffer,3)) printf("Successfully read data\nRead value1: %02X\nRead value2: %02X\n", byte_buffer[0],byte_buffer[1]);
	else printf("Failed to read data!\n");
}



