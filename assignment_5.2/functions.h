#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
// I2C constants
#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17
#define I2C_BITRATE_HZ (100 * 1000) // 100 kHz
#define I2C_TIMEOUT_US 500000
// EEPROM constants
#define EEPROM_ADDR 0x50 // EEPROM device address
// The largest possible address to store led state with 2 bytes
// EEPROM address range from 0x0000 -> 0x7FFF
#define LED_STATE_ADDR 0x7FFE
// Log starts from address 0, and the first two kilobytes are reserved for log msgs
// Therefore last address = 2047 = 0x7FF
#define LOG_START_ADDR 0x0
#define LOG_STOP_ADDR 0x7FF
#define MAX_LOG_ENTRY_LEN 61 // Maximum amount of characters per log entry
#define LOG_ENTRIES 32 // Amount of log entries reserved in EEPROM
#define EEPROM_BYTES 32768 // bytes
#define EEPROM_PAGESIZE 64 // bytes
#define W_DATA_ARR_LEN 3
#define R_ADDR_LEN 2
#define READ_LEN 1
#define WR_SLEEP_TIME_MS 5
// Button constants
#define SW0_PIN 9
#define SW1_PIN 8
#define SW2_PIN 7
#define BTN_MASK (1u << (SW0_PIN) | 1u <<(SW1_PIN) | 1u << (SW2_PIN))
#define BTN_COUNT 3
#define BTN_DEBOUNCE_MS 50
// LED constants
#define LED1_PIN 20
#define LED2_PIN 21
#define LED3_PIN 22
#define LED_MASK (1u << (LED1_PIN) | (1u << LED2_PIN) | (1u << LED3_PIN))
#define LED_START LED1_PIN // Start point for leds, to calculate led position for set_leds
#define LED_COUNT 3

typedef struct ledstate
{
 uint8_t state;
 uint8_t not_state;
} ledstate;

typedef struct button
{
 uint32_t last_accepted_time_us; // microseconds
 bool last_state; // True = not pressed, false = pressed
 uint8_t btn_pin;
}button;

bool write_byte(i2c_inst_t *i2c, uint8_t device_addr, uint16_t w_addr, uint8_t w_data);
bool read_byte(i2c_inst_t *i2c, uint8_t device_addr, uint16_t r_addr, uint8_t *r_data);
bool read_n_bytes(i2c_inst_t *i2c, uint8_t device_addr, uint16_t r_addr, uint8_t *dest_arr, size_t dest_len);
bool write_n_bytes(i2c_inst_t *i2c, uint8_t device_addr, uint16_t w_addr, const uint8_t *src_arr, size_t src_len);
bool button_pressed(button *btn, uint32_t debounce_ms);
bool led_state_is_valid(ledstate *ls);
bool load_led_state(uint8_t *led_state);
bool save_led_state(uint8_t led_state);
bool log_is_valid(const uint8_t *log_buf, size_t log_buf_len);
bool read_log_slot(size_t n, uint8_t *msg_buf, size_t msg_buf_len);
bool find_free_slot(size_t *free_slot, uint8_t *read_failed);
bool erase_log();
bool log_write(uint8_t *msg_buf, size_t msg_buf_len);

size_t parse_log_entry(const uint8_t *msg_buf, size_t msg_buf_len, uint8_t *out_msg, size_t out_msg_len);

uint16_t crc16(const uint8_t *data_p, size_t length);

void set_leds(uint8_t led_state, uint8_t led_count);
void set_led_state(ledstate *ls, uint8_t value);
void setup_i2c(i2c_inst_t *i2c, uint bitrate, uint8_t sda, uint8_t scl);
void pull_up(void);
void init_buttons(uint32_t btn_mask);
void init_leds(uint32_t led_mask);
void init_button_state(button *btn, uint8_t pin, uint32_t debounce_ms);
void log_print_all();