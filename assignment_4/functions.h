#pragma once
#include <string.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#define STR_BUFFER 50
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5

#define BITRATE 9600
#define UART_TIMEOUT 500000 // 500 ms
#define BTN_SLEEP_TIME = 30;
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
bool wait_for_line(char *prefix, int timeout_us, char *str_buf, int buf_size);