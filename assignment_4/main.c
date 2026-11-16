#include "functions.h"
#include "iuart.h"
#include <stdio.h>
#include <stdbool.h>


int main(void)
{
	UART_STATES state = WAIT_FOR_BTN;
	stdio_init_all();
	init_button(SW0_PIN);
	iuart_setup(UART_NR, UART_TX_PIN, UART_RX_PIN, UART_BITRATE);
	char string_buffer[STR_BFR_SIZE] = {0};
	char deveui_bfr[17] = {0};
	uint8_t attempts = 0;
	bool btn_pressed = false;
	bool got_connection = false;
	bool got_response = false;
	bool got_deveui = false;
	while (1)
	{
		switch (state)
		{
			case WAIT_FOR_BTN:
				btn_pressed = button_pressed(SW0_PIN, BTN_DEBOUNCE_MS);
				got_response = false;
				got_connection = false; // Reset state response flags
				got_deveui = false;
				if (btn_pressed)
				{
					attempts = 0;
					state = AT_TEST;
				}
				break;
			case AT_TEST:
				got_connection = send_and_wait(UART_NR, AT_TEST_CMD,AT_TEST_RETURN, string_buffer, STR_BFR_SIZE, UART_TIMEOUT);
				if (got_connection)
				{
					printf("Connected to LoRa module\nResponse: %s\n", string_buffer);
					state = VERSION;
				} else
				{
					attempts++;
					printf("Attempts: %d\n", attempts);
					if (attempts >= MAX_TEST_ATTEMPTS)
					{
						printf("Module not responding\n");
						state = WAIT_FOR_BTN; // Back to waiting state
					}
				}
				break;
			case VERSION:
				got_response = send_and_wait(UART_NR, VERSION_CMD,VERSION_RETURN, string_buffer, STR_BFR_SIZE, UART_TIMEOUT);
				if (got_response)
				{
					printf("Got version\nResponse: %s\n", string_buffer);
					state = DEVEUI;
				} else
				{
					printf("Module stopped responding\n");
					state = WAIT_FOR_BTN; // Back to waiting state
				}
				break;
				case DEVEUI:
				got_deveui = send_and_wait(UART_NR, DEVEUI_CMD,DEVEUI_RETURN, string_buffer, STR_BFR_SIZE, UART_TIMEOUT);
				if (got_deveui)
				{
					if (parse_deveui(string_buffer, deveui_bfr))
					{
						printf("DevEui: %s\n", deveui_bfr);
					}
					else printf("Failed to parse DevEui: %s\n", string_buffer);
					state = WAIT_FOR_BTN;
				} else
				{
					printf("Module stopped responding\n");
					state = WAIT_FOR_BTN; // Back to waiting state
				}
				break;
		}
	}
}
