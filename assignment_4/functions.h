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
static const char *LORA_TEST_CMD = "AT\n";

typedef enum
{
	SEND_CMD,
	RECV_CMD,
	TIMEOUT_CMD,
}UART_STATES;


typedef enum
{
	PARSE_OK,
	PARSE_TIMEOUT,
	PARSE_OVERFLOW // char array overflows
} parse_result;

parse_result parse_string();
void init_uart1();