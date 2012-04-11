#ifndef _UART_H_
#define _UART_H_
/**
 * Declares the UART-related things plus the useful userspace stuff like
 * gets() and puts().
 */

// Standard headers
// None

// Definitions
// None

// Magic numbers
#define UART_SEND_BUFFER_LEN	256
#define UART_RECV_BUFFER_LEN	256
#define UART_SEND_BUFFER_MASK	(UART_SEND_BUFFER_LEN-1)
#define UART_RECV_BUFFER_MASK	(UART_RECV_BUFFER_LEN-1)

// Functions
void uart0_handler(void) __irq;
void uart_init(void);
uint8_t uart_send_full(void);
uint8_t uart_send_empty(void);
uint8_t uart_recv_ready(void);
uint8_t uart_read_byte(void);
void uart_gets(uint8_t *dest, uint32_t max_length, uint8_t terminator);
void uart_puts(uint8_t *src, uint32_t length);

#endif
