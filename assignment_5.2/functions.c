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
	int returned_vals = i2c_write_timeout_us(i2c, device_addr, data_arr, W_DATA_ARR_LEN, false, I2C_TIMEOUT_US);
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
	int returned_vals = i2c_write_timeout_us(i2c, device_addr, data_arr, R_ADDR_LEN, true, I2C_TIMEOUT_US);
	if (returned_vals != R_ADDR_LEN) return false; // Couldn't open connection
	int read_vals = i2c_read_timeout_us(i2c, device_addr, r_data, READ_LEN, false, I2C_TIMEOUT_US);
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
	int returned_vals = i2c_write_timeout_us(i2c, device_addr, addr_buf, R_ADDR_LEN, true, I2C_TIMEOUT_US);
	if (returned_vals == PICO_ERROR_TIMEOUT) return false;
	if (returned_vals != R_ADDR_LEN) return false; // Couldn't open connection to EEPROM
	int read_vals = i2c_read_timeout_us(i2c, device_addr, dest_arr, dest_len, false, I2C_TIMEOUT_US);
	if (read_vals < 0) return false; // Read error, PICO_ERROR_TIMEOUT/GENERIC ERROR
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
		int bytes_written = i2c_write_timeout_us(i2c, device_addr, data_chunk, chunk_size + 2, false, I2C_TIMEOUT_US);
		if (bytes_written < 0) return false;  // write error, PICO_ERROR_TIMEOUT/GENERIC ERROR
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
// else false (led_state is NULL, state is not valid or read failed)
bool load_led_state(uint8_t *led_state)
{
	if (led_state != NULL) return false;
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
		// Not valid state, return false
		*led_state = 0b010;
		return false;
	}
	return false;
}

// Saves led_state to EEPROM
// Returns true if succesfull
// Else false
bool save_led_state(uint8_t led_state)
{
	ledstate temp_stat;
	set_led_state(&temp_stat, led_state);
	bool write_ok = write_n_bytes(i2c0, EEPROM_ADDR, LED_STATE_ADDR, (uint8_t *) &temp_stat, sizeof(temp_stat));
	if (write_ok) return true;
	return false;
}

// Initializes button state
void init_button_state(button *btn, uint8_t pin, uint32_t debounce_ms)
{
	btn->btn_pin = pin;
	btn->last_accepted_time_us = time_us_32() - debounce_ms * 1000;
	btn->last_state = true; // Not pressed
}
// Checks if buttons current state, previous state and debounce
// Returns true if press is accepted
// else false
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

// Validates one log row
// Returns true if valid, else false
// For log to be valid, '\0' has to be after log_buf[0], but before log_buf[62], and pass crc
bool log_is_valid(const uint8_t *log_buf, size_t log_buf_len)
{
	if (log_buf == NULL || log_buf_len < 4) return false; // 4 is minimum bytes for valid log, char + '\0' + 2 bytes of crc
	uint8_t max_search_len = log_buf_len - 2; // '\0' has to be in idx log - crc bytes
	uint8_t *null_char_ptr = memchr(log_buf, '\0', max_search_len); // Find NULL
	if (null_char_ptr == NULL || null_char_ptr == log_buf) return false; // no '\0' in log msg or null in buf[0]
	size_t null_idx = null_char_ptr - log_buf; // Get the length of the string
	return crc16(log_buf, null_idx + 3) == 0; // Returns true if CRC checksum is correct, else false.
}

// Creates a log entry with msg + '\0' + CRC msb + CRC lsb
bool parse_log_entry(const uint8_t *msg_buf, size_t msg_buf_len, uint8_t out_msg, size_t out_msg_len)
{
	if (msg_buf == NULL || msg_buf_len == 0) return false;

}

// Calculates and returns CRC checksum
uint16_t crc16(const uint8_t *data_p, size_t length)
{
	uint8_t x;
	uint16_t crc = 0xFFFF;
	while (length--)
	{
		x = crc >> 8 ^ *data_p++;
		x ^= x >> 4;
		crc = (crc << 8) ^ (uint16_t) (x << 12) ^ (uint16_t) (x << 5) ^ (uint16_t) x;
	}
	return crc;
}




