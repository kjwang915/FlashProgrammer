/**
 * Implementation of the USB communications routines
 */

// Standard headers
// None

// Project definitions
#include "type.h"
#include "usbcomms.h"
#include "usb.h"
#include "usbhw.h"
#include "usbreg.h"

// Personal data storage area
volatile uint8_t usb_send_buf[USB_SEND_BUFFER_SIZE];
volatile uint8_t usb_recv_buf[USB_RECV_BUFFER_SIZE];
volatile uint16_t usb_send_r, usb_send_w, usb_recv_r, usb_recv_w;
volatile uint8_t inTransfer;


/**
 * Inserts delay.
 */
void insert_delay(uint32_t Nprescaler)  //Nprescaler=99 means 0.1 seconds
{
	// insert delay for the usb transmittion to finish
	T0PR=Nprescaler;  //7 2 0.1 second delay
	T0TCR=2; //stop and reset time
	T0MCR=0x20;
	T0MR1=T0TC+30000;  //30e6,
	T0PC=0; //reset prescale counter register
	T0TCR=1; //start the timer
	while ((T0TCR&1)==1)
	{
		WAIT;
	}
}


/**
 * Properly initialize the data
 */
void usb_user_init() {
	usb_send_r = usb_send_w = usb_recv_r = usb_recv_w = 0;
	inTransfer = 0;
}

/**
 * Is there unread stuff in the recv buffer?
 * This buffer is empty when our read pointer catches up to the next read pointer
 */
uint8_t usb_read_ready() {
	return (usb_recv_r != usb_recv_w);
}

/**
 *  Determines when the send buffer has space in it
 */
uint8_t usb_send_empty() {
	return usb_send_r == usb_send_w;
}

/**
 * Read data out of the USB buffer, waiting to fill the requested length
 */
void usb_read(uint8_t *dest, uint32_t len) {
	uint32_t count = 0;

	while (count < len) {
		if (usb_read_ready()) {
			*(dest+count) = usb_recv_buf[usb_recv_r];
			usb_recv_r = (usb_recv_r + 1) & USB_RECV_BUFFER_MASK;
			count += 1;
		}
	}
}

/**
 * Write data into the USB buffer, breaking things apart into multiple buffers
 * if there is not enough room
 */
void usb_write(const uint8_t *src, uint32_t len) {
	uint16_t space;  
	uint32_t transferSize;
	uint16_t i;

	// Dodge null writes
	if (len == 0)
		return;

	// Write directly to the buffer at first
	if (usb_send_empty() && !inTransfer) {
		inTransfer = 1;
		if (len > 64) {
			transferSize = USB_WriteEP(FT_EP_IN, src, 64);
		}
		else {
			transferSize = USB_WriteEP(FT_EP_IN, src, len);
		}

		if (transferSize != len) {
			len -= transferSize;
			src += transferSize;
		}
		else {
			return;
		}
	}

	// Check for space first
	space = USB_SEND_BUFFER_MASK - ((usb_send_w - usb_send_r) & USB_SEND_BUFFER_MASK);
	if (space < len) {  
		usb_write(src, space);  
		while(!usb_send_empty()) { ; }
		usb_write(src+space, len-space);
		return;
	}

	// Copy to the send buffer
	for (i = 0; i < len; ++i) {
		usb_send_buf[usb_send_w] = src[i];
		usb_send_w = (usb_send_w + 1) & USB_SEND_BUFFER_MASK;
	}
	
	// Call insert_delay here
	insert_delay(99);
}

/**
 * Called by the USB controller on a Bulk endpoint IN event
 * which means that we should transmit data.
 */
void usb_bulkIn() {
	int32_t transferSize;
	int32_t actualTransfer;
	uint8_t buffer[64];
	uint8_t i;

	transferSize = ((usb_send_w - usb_send_r) & USB_SEND_BUFFER_MASK);
	if (transferSize > 64)
		transferSize = 64;

	if (transferSize == 0) {
		inTransfer = 0;
		return;
	}

	for (i = 0; i < transferSize; ++i) {
		buffer[i] = usb_send_buf[usb_send_r];
		usb_send_r = (usb_send_r + 1) & USB_SEND_BUFFER_MASK;
	}

	actualTransfer = USB_WriteEP(FT_EP_IN, buffer, transferSize);
	if (actualTransfer < transferSize) {
		usb_send_r = (usb_send_r - (transferSize - actualTransfer)) & USB_SEND_BUFFER_MASK;
	}
}

/**
 * Called by the USB controller on a bulk endpoint OUT event
 * which means that we should receive data.
 */
void usb_bulkOut() {
	int32_t actualTransfer;
	uint8_t buffer[64];
	uint8_t i;

	actualTransfer = USB_ReadEP(FT_EP_OUT, buffer);

	for (i = 0; i < actualTransfer; ++i) {
		usb_recv_buf[usb_recv_w] = buffer[i];
		usb_recv_w = (usb_recv_w + 1) & USB_RECV_BUFFER_MASK;
	}
}
