#include "functions.h"
#include "iuart.h"


#define STR_BUFFER 50
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5

#define BITRATE 9600
#define UART_TIMEOUT 500000 // 500 ms
typedef const char const_str;
static const_str *LORA_TEST_CMD = "AT\n";
// Reads characters through UART
// Converts them to strings, reads string or timeout occurs
void parse_string(char *str_buf, const int buf_size)
{
	char char_from_queue = 'O';
	int got_byte = 0;
	uint16_t ind = 0;
	uint32_t t = time_us_32();
	// Reading until timeout, or newline.
	do
	{
		if (ind >= buf_size - 1) return; // Prevent buffer overflow
		// Reading a byte from uart, saving the read byte to char_from_queue
		got_byte = iuart_read(UART_NR, (uint8_t *) &char_from_queue, 1);
		if (got_byte == 1)
		{
			str_buf[ind] = char_from_queue; // each character is one byte aka one element.
			ind++;													// Create a new array of characters
		}
		str_buf[ind] = '\0';
	} while (char_from_queue != '\n' && time_us_32() - t <= UART_TIMEOUT);
}