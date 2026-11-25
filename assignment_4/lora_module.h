#pragma once
#include <string.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#define STR_BFR_SIZE 50
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define SW0_PIN 9

#define UART_BITRATE 9600
#define UART_TIMEOUT 500000 // 500 ms
#define BTN_DEBOUNCE_MS 100 // For btn debounce
#define START_STR "DevEui, "
#define AT_TEST_CMD "AT\r\n"
#define AT_TEST_RETURN "+AT: OK"
#define VERSION_CMD "AT+VER\r\n"
#define VERSION_RETURN "+VER"
#define DEVEUI_CMD "AT+ID=DevEui\r\n"
#define DEVEUI_RETURN "+ID: DevEui"
#define MAX_TEST_ATTEMPTS 5
typedef enum
{
	WAIT_FOR_BTN,
	AT_TEST,
	VERSION,
	DEVEUI
} UART_STATES;


typedef enum
{
	PARSE_OK,
	PARSE_TIMEOUT,
	PARSE_OVERFLOW // char array overflows
} parse_result;

parse_result parse_string(char *str_buf, int buf_size, uint32_t timeout_us);
bool button_pressed(int btn_pin, int sleep_time_ms);
void flush_uart(int uart_nr);
bool wait_for_line(const char *prefix, uint32_t timeout_us, char *str_buf, int buf_size);
bool send_and_wait(int uart_nr, const char *cmd, const char *prefix, char *buf, int size, uint32_t timeout_us);
bool parse_deveui(const char *line, char *out);
void init_button(int btn_pin);