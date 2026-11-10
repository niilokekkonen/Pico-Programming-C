#pragma once
#include <string.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"

typedef enum
{
	SEND_CMD,
	RECV_CMD,
	TIMEOUT_CMD,
}UART_STATES;


void parse_string();
void init_uart1();