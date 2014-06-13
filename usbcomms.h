#ifndef _USBCOMMS_H_
#define _USBCOMMS_H_
/**
 * Communications package for sharing data between the USB kernel and the main program
 */

// Standard headers
#include <LPC214x.h>
#include <inttypes.h>

// Project definition
// None

// helper macros
#define WAIT asm("nop")

// Endpoint Addresses
#define FT_EP_IN	0x82
#define FT_EP_OUT	0x02

// Buffer lengths for extra data outside of the USB system
#define USB_SEND_BUFFER_SIZE 2048
#define USB_RECV_BUFFER_SIZE 2048
#define USB_SEND_BUFFER_MASK (USB_SEND_BUFFER_SIZE-1)
#define USB_RECV_BUFFER_MASK (USB_RECV_BUFFER_SIZE-1)

// Command structure for what is received on the bulk pipe
struct usb_command {
	uint8_t command;
	uint8_t argument;
};

// main() side access functions
void usb_user_init();
uint8_t usb_read_ready();
uint8_t usb_send_empty();
void usb_read(uint8_t *dest, uint32_t len);
void usb_write(const uint8_t *src, uint32_t len);

void insert_delay(uint32_t Nprescalar);

// Backend functions called by the usbuser library
void usb_bulkIn();
void usb_bulkOut();

#endif
