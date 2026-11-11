#include "functions.h"
#include "iuart.h"

// Reads characters through UART
// Converts them to strings, reads string or timeout occurs
// Parameters, str buffer, size, and timeout in microseconds
parse_result parse_string(char *str_buf, const int buf_size, uint32_t timeout_us)
{
	char char_from_queue;
	int got_byte = 0;
	int index = 0;
	bool line_done = false;
	parse_result result;
	uint32_t t = time_us_32();
	// Reading until timeout, or newline.
	do
	{
		// Reading a byte from uart, saving the read byte to char_from_queue
		got_byte = iuart_read(UART_NR, (uint8_t *) &char_from_queue, 1);
		if (got_byte == 1)
		{
			if (char_from_queue == '\n') line_done = true; // Encounter end
			else if (char_from_queue == '\r') {} // don't add CR to buffer
			else
			{
				str_buf[index] = char_from_queue; // each character is one byte aka one element.
				index++;													// Create a new array of characters
			}
		}
		// If buffer overflow, timeout or line is read, stop loop
	} while (!line_done && index < buf_size - 1 && time_us_32() - t <= timeout_us);
	str_buf[index] = '\0';
	if (line_done)  result = PARSE_OK;
	else if (index == buf_size - 1) result = PARSE_OVERFLOW;
	else result = PARSE_TIMEOUT;
	return result;
}

// Initialize btn_pin as output pin
void init_button(int btn_pin)
{
	gpio_init(btn_pin);
	gpio_set_dir(btn_pin, GPIO_IN);
	gpio_pull_up(btn_pin);
}


// Pull up resistor logic. Check button, and debounce
// Takes btn pin, and how long to sleep in debounce
bool button_pressed(int btn_pin, int sleep_time_ms)
{
	if (!gpio_get(btn_pin))
	{
		sleep_ms(sleep_time_ms);
		if (!gpio_get(btn_pin))
		{
			while (!gpio_get(btn_pin)) sleep_ms(10); // Wait for release
			return true;
		}
		return false;
	}
	// BTN not pressed
	return false;
}

// Flushes uart RX queue
void flush_uart(int uart_nr)
{
	uint8_t discarded_byte; // temporary var for discarding the bytes
	while (iuart_read(uart_nr, &discarded_byte, 1)) ;
}