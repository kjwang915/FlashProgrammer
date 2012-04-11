/**
 * Flash programmer main entry point
 */

// Standard headers
#include <LPC214x.h>
#include <inttypes.h>
#include <type.h>
#include <irq.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Definitions
#include "main.h"
#include "usb.h"
#include "usbhw.h"
#include "uart.h"
#include "usbdesc.h"
#include "usbcomms.h"

#define mylen 2048  //one page has 2048 bytes
#define Nbyte 64

uint8_t write_buffer[mylen];
uint8_t read_buffer[Nbyte];

/**
 * Application entry point
 */
int main(void) {
	//uint32_t buffer_r;
	uint32_t i, otime, j;
	uint32_t address;
	uint32_t count;
	uint8_t cmd[10];
	uint8_t otime1[4];
	uint8_t result;
	uint8_t buffer[32];

	init();
	usb_user_init();

	USB_Init();
	USB_Connect(TRUE);
	
	memset(write_buffer, 0x00, mylen * sizeof(uint8_t));
	
	count=6000000; //6e6

	while (1) {
		if (usb_read_ready()) {
			usb_read(cmd, 4);
			if (strncmp((char *)cmd, "m", 1) == 0) {
			
				//*************debug*************
				FIO1DIR = 0x00ff0000;
				FIO1CLR = 0xffffffff;
				//*******************************
				memset(buffer, 0xff, 32);

				for ( i=0; i<1000;i++)
				{
					read(0, 32, buffer);
					memset(buffer, 0xff, 32);
					usb_write(buffer, 32);
					//usb_write((uint8_t *)"Done.", 5);
					//usb_write(buffer, 64);
				}
				
				read(0, 32, buffer);
				usb_write((uint8_t *)"Done.", 5);
				FIO1SET=0x00ff0000;
			/*	address = (((uint32_t) 0x00) << 26) | (((uint32_t) 0x10) << 18) | (((uint32_t) (0x05)) << 12);
				
				// *******************debug**************
					result = erase(address);
					result = write(address, mylen, write_buffer);
					read(address, Nbyte, read_buffer);
					usb_write(read_buffer, Nbyte);
					if (result != 0) {
						usb_write((uint8_t *) "Error.", 6);
						continue;
					} */
				// *******************************************
				//restore stress level
			/*	for (i=0;i<100;++i){
					result = write(address, mylen, write_buffer);
					result = erase(address);
				}
				
				for (j=0;j<10;j++)
				{
					result = write(address, mylen, write_buffer);
					// Erase to start things off
					result = erase(address);
				/ *	if (result != 0) {
						usb_write((uint8_t *) "Error.", 6);
						continue;
					} * /
					
					// ***************wait for  two hours***********
				/ *	T1PR=71999;  
					T1TCR=2; //stop and reset time
					T1MCR=0x20;
					T1MR1=T1TC+3000000;
					T1TCR=1; //start the timer
					while ((T1TCR&1)==1);  
					// ***************************************
					T1MCR=0x00;  //stop comparing
					T1PR=49;  //prescaler
					T1TCR=2; //stop and reset time
					T1TCR=1; //start the timer
					
					for (i = 0; i < count; ++i) {
						read(address, Nbyte, read_buffer);
						//buffer[buffer_r++] = results[0];
						// **********debug**************
						FIO1SET=0x000f0000; 
						// ******************************
						usb_write(read_buffer, Nbyte);
					}
					
					otime=T1TC;
					T1TCR=2; //stop and reset time
					otime1[0] = (uint8_t) (otime >> 24);  //time used
					otime1[1] = (uint8_t) (otime >> 16);
					otime1[2] = (uint8_t) (otime >> 8);
					otime1[3] = (uint8_t) (otime);
					usb_write(otime1,4);
				}	*/							
			}
			else if (strncmp((char *)cmd, "r", 1) == 0) {
				// Read command
				memset(buffer, 0, 64);
				read(0, 64, buffer);
				usb_write((uint8_t *)"Read result: ", 12);
				usb_write(buffer, 64);
			}
			//FIO1SET=0x00ff0000; //when the lights are on, the program is finished	
/*			else if (strncmp((char *)cmd, "p", 1) == 0) {
				for (i = 0; i < 10; ++i) {
					buffer[i] = cmd[1];
				}
				results[0] = write(0, 10, buffer);
				usb_write((uint8_t *)"Program result: ", 15);
				usb_write(results, 1);
			}
			else if (strncmp((char *)cmd, "r", 1) == 0) {
				// Read command
				memset(buffer, 0, 64);
				read(0, 64, buffer);
				usb_write((uint8_t *)"Read result: ", 12);
				usb_write(buffer, 64);
			}
			else if (strncmp((char *)cmd, "e", 1) == 0) {
				results[0] = erase(0);
				usb_write((uint8_t *)"Erase result: ", 13);
				usb_write(results, 1);
			}  */
		}
	}
	return 0;
}

/**
 * Initializes interrupt controller, port pins, and all modules not configured in Startup.S
 */
void init(void) {
	// Set up interrupt controller, we'll need it for UART/USB stuff
	init_VIC();

	// Configure GPIO for both Port0 and Port1 to use fast I/O registers
	SCS |= 3;

	// Configure I/O directionality for the control bits
	reset_io();

	// Set up UART so we have communications
	PINSEL0 = 1 | (1 << 2);	// P0.0 and P0.1 configured as Rx0 and Tx0
}
/**
 * Resets the I/O pin configuration to the one after boot, which affects directionality and value
 */
void reset_io(void) {
	FIO0DIR = P_CHIP_ENABLE | P_ADDR_LATCH | P_CMD_LATCH | P_READ_ENABLE | P_WRITE_ENABLE | P_WRITE_PROTECT;
	FIO0SET = P_CHIP_ENABLE | P_WRITE_ENABLE | P_WRITE_PROTECT | P_READ_ENABLE;
	FIO0CLR = P_ADDR_LATCH | P_CMD_LATCH;
	WAIT; WAIT; WAIT;
	DEASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;
}

/**
 * Erases an entire block (the smallest granularity possible) from the device
 */
uint8_t erase(uint32_t address) {
	uint8_t result;
	
	// Activate the chip and set up I/O
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;

	// Set up control signals
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE | P_ADDR_LATCH;
	FIO0SET = P_WRITE_PROTECT | P_READ_ENABLE;

	WAIT; WAIT;
	
	// Write the first word for erase
	write_cmd_word(CMD_BLOCK_ERASE_1);

	// Write address for it
	write_address(address, 1);

	// Write second command word
	write_cmd_word(CMD_BLOCK_ERASE_2);
	FIO0CLR = P_READ_ENABLE;

	// Change to input
	SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
	
	// Wait for erase to finish
	WAIT_FOR_BUSY;
	reset_io();
	DEASSERT_CHIP_ENABLE; WAIT;

	// Read status and find out if the program was successful, returning status
	result = read_status();
	if ((result & SR_GENERIC_ERROR) == SR_GENERIC_ERROR)
		return 1;
	return 0;
}

/**
 * Reads the status register
 */
uint8_t read_status(void) {
	uint8_t result;

	// Activate the chip and set up the I/O
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;

	// Sanitize input control states
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE | P_ADDR_LATCH;
	FIO0SET = P_READ_ENABLE;

	// Write the read status command word
	write_cmd_word(CMD_READ_STATUS);

	// Change to input
	SET_IO_AS_INPUT; WAIT; WAIT; WAIT;

	// Drive Read low to load the status register onto the I/O pins
	FIO0CLR = P_READ_ENABLE; WAIT;
	result = FIO0PIN2;

	// Clear chip enable and return results
	DEASSERT_CHIP_ENABLE; WAIT;
	reset_io();
	return result;
}

/**
 * Sequentially programs some number of bytes starting at a given address.
 */
uint8_t write(uint32_t address, uint32_t count, const uint8_t *src) {
	uint32_t i, result;

	// Maximum page size that can be written at once is 2k + 64 bytes of spare
	if (count > 2112)
		count = 2112;

	// Activate the chip
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;
	FIO0SET = P_WRITE_ENABLE; WAIT;

	WAIT; WAIT;
	
	// Send the first half of the page program command
	write_cmd_word(CMD_PAGE_PROGRAM_1);

	// Send the address to start at (multiple bus cycles)
	write_address(address, 0);

	// With the address loaded, the data now needs to be loaded
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE | P_ADDR_LATCH;
	FIO0SET = P_READ_ENABLE | P_WRITE_PROTECT;

	// Actually send out the data
	for (i = 0; i < count; ++i) {
		FIO0CLR = P_WRITE_ENABLE; WAIT;
		WRITE_VALUE(*(src+i));
		FIO0SET = P_WRITE_ENABLE; WAIT;
	}

	// Now send out the final command to begin programming
	write_cmd_word(CMD_PAGE_PROGRAM_2);

	// Just to be safe go back to input mode
	SET_IO_AS_INPUT;

	// Wait for programming to finish
	WAIT_FOR_BUSY;
	DEASSERT_CHIP_ENABLE; WAIT;
	reset_io();

	// Read status and find out if the program was successful, returning status
	result = read_status();
	if ((result & SR_GENERIC_ERROR) == SR_GENERIC_ERROR)
		return 1;
	return 0;
}

/**
 * Sequentially read some number of bytes starting at the given address
 */
void read(uint32_t address, uint32_t count, uint8_t *dest) {
	uint32_t i;

	// Maximum page size is 2k + 64 bytes of spare
	if (count > 2112)
		count = 2112;

	// Activate the chip
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;

	WAIT; WAIT;
	
	// Send first half of the read command
	write_cmd_word(CMD_READ_1);

	// Send the address (multiple bus cycles)
	write_address(address, 0);

	// With the address loaded, write the read confirm command
	write_cmd_word(CMD_READ_2);
	
	// Change to input type pins
	SET_IO_AS_INPUT;

	// Now, wait for the busy signal to become de-asserted
	WAIT_FOR_BUSY;

	// Burn some time before reading, it improves results for some reason
	for (i = 0; i < 100; ++i) {
		WAIT;
	}
	
	// Strobe /R to read out the data, for as many bytes as we want to read
	for (i = 0; i < count; ++i) {
		FIO0CLR = P_READ_ENABLE; WAIT;
		*(dest+i) = FIO0PIN2;
		FIO0SET = P_READ_ENABLE; WAIT;
	}

	// Disable the chip once we are done
	DEASSERT_CHIP_ENABLE;
	reset_io();
}

/**
 * Helper function that writes a command word and asserts all the proper control signals
 */
void write_cmd_word(uint8_t cmd) {
	// Set control signals to load command word
	FIO0SET = P_CMD_LATCH | P_READ_ENABLE;
	FIO0CLR = P_ADDR_LATCH | P_WRITE_ENABLE;
	WAIT; WAIT;

	// Send the suggested command
	WRITE_WITH_FLOP(cmd);

	// Clear the command latch so we don't have any weird timing problems
	FIO0CLR = P_CMD_LATCH; WAIT; WAIT;
}

/**
 * Helper function that writes an address to the bus
 */
void write_address(uint32_t address, uint8_t doErase) {
	// Set up for address input, according to table 4 on page 16
	FIO0SET = P_ADDR_LATCH; // | P_READ_ENABLE
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE;
	WAIT;

	// The address is loaded in 8-bit segments in the sequence specified on page 16, table 5

	// These two blocks are only loaded if we need a full address
	if (!doErase) {
		// First, load the lowest 8 bytes
		WRITE_WITH_FLOP((uint8_t)(address & 0x000000ff));
	
		// Next load in four zeros along with the four next MSBs
		WRITE_WITH_FLOP((uint8_t)((address & 0x00000f00) >> 8));
	}
	
	// Next, load in the next eight bits (12-19)
	WRITE_WITH_FLOP((uint8_t)((address & 0x000ff000) >> 12));

	// Next eight bits (20-27)
	WRITE_WITH_FLOP((uint8_t)((address & 0x0ff00000) >> 20));

	// Last few remaining bits, which do not extend to a full 32 bits (only 31 for 8 Gb, and only 30 for 4 Gb!)
	WRITE_WITH_FLOP((uint8_t)((address & 0x70000000) >> 28));

	// De-assert the address latch now, just in case
	FIO0CLR = P_ADDR_LATCH;
}
