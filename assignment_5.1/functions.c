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

/*
// Writes n amount of bytes to EEPROM through I2C, src_len = amount of bytes
// Returns true if write was successful
// Returns false if writing stops abruptly, src_arr is NULL or len is zero
// Writes values from src_arr to EEPROM
bool write_n_bytes(i2c_inst_t *i2c, uint8_t device_addr, uint16_t w_addr, const uint8_t *src_arr, size_t src_len)
{
	if (src_arr == NULL)	return false; // Necessary safety measures
	if (src_len == 0)	return false;
	if ((uint32_t) w_addr + src_len > EEPROM_BYTES) return false;
	for (size_t i = 0; i < src_len; i++)
	{
		if (!write_byte(i2c, device_addr, (uint16_t) (w_addr + i), src_arr[i])) return false;
	}
	return true;
}
*/
// Runs all the different functions.
void test_functions(void)
{
	uint8_t read_val = 0;
	uint8_t write_val = 0x5A; // 0101 1010
	uint8_t write_val2 = 0x7F; // 0111 1111
	uint8_t write_bytes[3] = {0x5F, 0x67, 0xFF};
	uint8_t byte_buffer[EEPROM_BYTES] = {0};
	stdio_init_all();
	uint16_t r_w_addr = 0x1234; // Ranges from 0x0000 -> 07FFF
	uint16_t write_n_addr = 0x3345;
	setup_i2c(i2c0, I2C_BITRATE_HZ, I2C0_SDA_PIN, I2C0_SCL_PIN);
	sleep_ms(100);
	if (write_byte(i2c0, EEPROM_ADDR, r_w_addr, write_val)) printf("Succesfully wrote data\n");
	else printf("Failed to write data\n");
	if (read_byte(i2c0, EEPROM_ADDR, r_w_addr, &read_val)) printf("Successfully read data %02x\n", read_val);
	else printf("Failed to read data!\n");
	if (write_byte(i2c0, EEPROM_ADDR, r_w_addr, write_val2)) printf("Succesfully wrote data\n");
	else printf("Failed to write data\n");
	if (read_byte(i2c0, EEPROM_ADDR, r_w_addr, &read_val)) printf("Successfully read data %02x\n", read_val);
	else printf("Failed to read data!\n");
	if (write_n_bytes(i2c0, EEPROM_ADDR,write_n_addr, write_bytes, 3)) printf("Successfully written data\n");
	else printf("Failed to write data\n");
	if (read_n_bytes(i2c0, EEPROM_ADDR, write_n_addr, byte_buffer,3)) printf("Successfully read data\nRead value1: %0X\nRead value2: %0X\n", byte_buffer[0],byte_buffer[1]);
	else printf("Failed to read data!\n");
	while (true)
	{
		sleep_ms(100);
	}
}

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
		if (bytes_written != chunk_size + 2) return false;
		sleep_ms(WR_SLEEP_TIME_MS);
		src_len -= chunk_size; // Lower src_length
		w_addr += chunk_size;
		src_arr += chunk_size;
	}
	return true;
}