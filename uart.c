/**
 * Implementation of UART-related functions, gets() and puts(), and other
 * important things. Interrupt based, non-blocking I/O support. Note that
 * this does not check for buffer overflows on read because it does not
 * seem particularly important yet (easy to add later).
 */

// Standard headers
#include <LPC214x.h>
#include <inttypes.h>
#include <type.h>
#include <irq.h>

// Definitions
#include "uart.h"

// Buffers for send/receive plus indices
volatile int8_t uart_send[UART_SEND_BUFFER_LEN];
volatile int8_t uart_recv[UART_RECV_BUFFER_LEN];
volatile int16_t uart_send_r, uart_send_w, uart_recv_r, uart_recv_w;

/**
 * Initialization routine, sets up buffer pointers and installs handlers.
 *
 * BAUD RATE IS 14400!
 */
void uart_init(void) {
	// 8-bit word length, enable DLAB
	U0LCR = (1 << 7) | (1 << 1) | (1 << 0);

	// PCLK = 30 MHz, set baud rate generators
	U0DLL = 93;
	U0DLM = 0;
	
	// Add in fractional parameters 7:4 is MULVAL and 3:0 is DIVADDVAL
	// For PCLK = 20 MHz, 5/(5+2) is optimal. Need to divide by 3/2 again for our 30 MHz
	U0FDR = (5 << 4) | (2 << 0);

	// Clear DLAB
	U0LCR = U0LCR & ~(1 << 7);

	// Enable FIFOs and clear both RX and TX
	U0FCR = (1 << 2) | (1 << 1) | (1 << 0);

	// Set all pointers to the start of their buffers
	uart_send_r = uart_send_w = uart_recv_r = uart_recv_w = 0;

	// Install the interrupt handler so that we get called
	install_irq(UART0_INT, (void *)uart0_handler);

	// Enable interrupts on recieve and send
	U0IER = (1 << 0) | (1 << 1);
}

/**
 * Returns a single byte from the UART holding buffer, and increments buffer pointers
 */
uint8_t uart_read_byte(void) {
	uint8_t ret = 0;

	// Don't update the pointer unless the buffer isn't empty so we don't break things
	if (uart_recv_ready()) {
		ret = uart_recv[uart_recv_r];
		uart_recv_r = (uart_recv_r + 1) & UART_RECV_BUFFER_MASK;
	}

	return ret;
}

/**
 * Recv buffer is empty when read = write.
 */
uint8_t uart_recv_ready(void) {
	return uart_recv_r != uart_recv_w;
}

/**
 * Send buffer is full when the write pointer is one behind the read pointer
 */
uint8_t uart_send_full(void) {
	return ((uart_send_w + 1) & UART_SEND_BUFFER_MASK) == uart_send_r;
}

/**
 * Similarly it is empty when the write pointer is equal to the read pointer
 */
uint8_t uart_send_empty(void) {
	return uart_send_r == uart_send_w;
}

/**
 * Interrupt handler...writes to buffers, reads from buffers
 */
void uart0_handler(void) __irq {
	// Copy over interrupt identification register
	uint8_t uart_code = U0IIR;

	// Received data
	if ((uart_code & 0x0e) == 0x04) {
		uart_recv[uart_recv_w] = U0RBR;
		uart_recv_w = (uart_recv_w + 1) & UART_RECV_BUFFER_MASK;
	}
	// Transmit complete
	else if ((uart_code & 0x0e) == 0x02) {
		if (!uart_send_empty()) {
			U0THR = uart_send[uart_send_r];
			uart_send_r = (uart_send_r + 1) & UART_SEND_BUFFER_MASK;
		}
	}
	// Line error, read the LSR for kicks and then basically ignore it
	else {
		uart_code = U0LSR;
	}

	// Reset interrupt vector controller
	VICVectAddr = 0;
}

/**
 * Reads a string out of the receive buffer or blocks for it to arrive, looking
 * for either maximum length or the termination character.
 */
void uart_gets(uint8_t *dest, uint32_t length, uint8_t terminator) {
	uint8_t count = 0;
	uint8_t input;
	uint8_t exitLoop = 0;

	// Copy out until we run out of space or find the terminator
	while (count < length && !exitLoop) {
		if (uart_recv_ready()) {
			input = uart_read_byte();
			dest[count++] = input;
			if (input == terminator) {
				exitLoop = 1;
			}
		}
	}

	// Null-terminate the string, unless the terminator was \0 already
	if (terminator != '\0')
		dest[count] = '\0';
}

/**
 * Write a string to the transmit buffer and begin transmission. If we don't have transmit space,
 * be sure to block until we do.
 */
void uart_puts(uint8_t *src, uint32_t length) {
	int16_t space;
	uint8_t i;
	
	// Check for space first
	space = (uart_send_w - uart_send_r - 1) & UART_SEND_BUFFER_MASK;
	if (space < length) {
		uart_puts(src, space);
		while(!uart_send_empty()) { ; }
		uart_puts(src+space, length-space);
		return;
	}

	// Copy to the send buffer
	for (i = 0; i < length; ++i) {
		uart_send[uart_send_w] = src[i];
		uart_send_w = (uart_send_w + 1) & UART_SEND_BUFFER_MASK;
	}
	
	// Send if we aren't already transmitting
	if ((U0LSR & (1 << 5)) == (1 << 5)) {
		U0THR = uart_send[uart_send_r];
		uart_send_r = (uart_send_r + 1) & UART_SEND_BUFFER_MASK;
	}
}
