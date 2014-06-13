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
//#include <time.h>

// Definitions
#include "main.h"
#include "usb.h"
#include "usbhw.h"
#include "uart.h"
#include "usbdesc.h"
#include "usbcomms.h"

/*
 *   This code checks if partial programming a page will mess up the paired pages
 */

// 16Gb MLC Hynix
#define Nbyte 8640  // 8k bytes + 448 spare
#define Nbytebase 8192
#define Nbytespare 448
#define Nbit 69120  //Nbit=Nbyte*8
#define Nblocks 1024 // number of blocks in a Hynix 4gbit part
#define Npages 256 // number pages per block
#define BLOCKOFFSET 22
#define PAGEOFFSET 14

#define pagestotest 64

#define blocklistsize 10
uint16_t blocklist[blocklistsize] = {12, 23, 56, 68, 199, 345, 500, 670, 800, 988};

uint16_t pageprogramtimes[Npages] = {11,11,
                                    11,11,30,60, // 6
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 22
                                    11,11,30,60,
                                    11,11,30,60, // 30
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 42
                                    11,11,30,60,
                                    11,11,30,60, // 50
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 62
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 74
                                    11,11,30,60,
                                    11,11,30,60, 
                                    11,11,30,60, // 86
                                    11,11,30,60, // 90
                                    11,11,30,60, 
                                    11,11,30,60,
                                    11,11,30,60, // 102
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 114
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 126
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 138
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 150
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 162
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 174
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 186
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 198
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 210
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 222
                                    11,11,30,60,
                                    11,11,30,60, 
                                    11,11,30,60, // 234
                                    11,11,30,60,
                                    11,11,30,60,
                                    11,11,30,60, // 246
                                    11,11,30,60, // 250
                                    11,11,30,60, // 254
                                    30,60};

/**************************************
** Global read/write buffers         **
** Keep these global, not in main()  **
** unsure what the compiler is doing **
** but programs break when you have  **
** these big buffers in functions    **
**************************************/
uint8_t write_buffer[Nbyte];
uint8_t read_buffer[Nbyte];
//uint8_t * readbufNbytebaseptr = (read_buffer + Nbytebase);

// address from which things are derived. I'm pretty sure this is all 0
// I'm not sure why we have this
uint32_t address0 = (((uint32_t) 0x00) << 26) | (((uint32_t) 0x00) << BLOCKOFFSET) | (((uint32_t) (0x00)) << PAGEOFFSET); 

// timer
//clock_t timer;

uint8_t otimep[4]; 
uint8_t otimee[4]; 
uint8_t addressout[4];


/**
 * Application entry point
 */
int main(void) {


    uint8_t cmd[20];

    uint32_t address;

 	uint32_t j;

 	uint32_t block;

 	init();
 	usb_user_init();

 	USB_Init();
 	USB_Connect(TRUE);

	//intialize the chip
 	ASSERT_CHIP_ENABLE;
 	SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
 	WAIT_FOR_BUSY;
 	SET_IO_AS_OUTPUT;

	//ASSERT_CHIP_ENABLE;
 	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE | P_ADDR_LATCH;
 	FIO0SET = P_WRITE_PROTECT | P_READ_ENABLE;

    write_cmd_word(0xFF);  //the first command to initialized the chip

    while (1) {
    	if (usb_read_ready()) {
    		usb_read(cmd, 4);
            usb_write(cmd, 4);
            //insertdelay

    
            if (strncmp((char *) cmd, "ppa", 3) == 0) {

                uint32_t write = 0;

                if (strncmp((char *) cmd, "ppam", 4) == 0) {
                    write = 1;
                }

                //uint32_t blockyes = 0;
                uint32_t ppfractionl = 0;
                uint32_t ppfractionm = 0;
                uint32_t ppfractionh = 0;
                uint32_t pptime = 0;

                while( 1 ) {


                    if (usb_read_ready()) {
                        usb_read(cmd, 4); // read in block

                        if (!ppfractionl) {
                            ppfractionl = atoi(cmd);
                        }
                        else if (!ppfractionm) {
                            ppfractionm = atoi(cmd);
                        }
                        else if (!ppfractionh) {
                            ppfractionh = atoi(cmd);
                        }
                        
                    }

                    if ( ( (ppfractionl && ppfractionm && ppfractionh) || !write) ) {
                        usb_write((uint8_t *) "PPFRAC", 6); // block and page are successfully read from input
                        break; // exit while loop
                    }

                }


                for (j = 0; j < blocklistsize; j++) {

                    block = blocklist[j];

                    uint8_t blockout[2];
                    blockout[0] = (uint8_t) (block>>8);
                    blockout[1] = (uint8_t) (block);
                    usb_write(blockout,2);
                    //insertdelay

                    // address of block
                    address = address0 | (((uint32_t) block ) << BLOCKOFFSET ) | (((uint32_t) 0) << PAGEOFFSET );

                    // do an erase first
                    if (write) {
                        complete_erase(address, otimee);
                        memset(write_buffer, 0x00, Nbyte); // write this
                    }


                    uint32_t ik;

                    for (ik = 0; ik < pagestotest; ik++) {

                        address = address0 | (((uint32_t) block) << BLOCKOFFSET) | (((uint32_t) ik) << PAGEOFFSET);
                        
                        
                        
                        if (pageprogramtimes[ik] == 11) {
                            pptime = ppfractionl * pageprogramtimes[ik] * 10;
                            usb_write((uint8_t *) "L",1);
                        }
                        else if (pageprogramtimes[ik] == 30) {
                            pptime = ppfractionm * pageprogramtimes[ik] * 10;
                            usb_write((uint8_t *) "M",1);
                        }
                        else if (pageprogramtimes[ik] == 60) {
                            pptime = ppfractionh * pageprogramtimes[ik] * 10;
                            usb_write((uint8_t *) "H",1);
                        }
                        else {
                            pptime = 70000;
                        }

                        uint8_t pptimeout[4];
                        pptimeout[0] = (uint8_t) (pptime>>24);
                        pptimeout[1] = (uint8_t) (pptime>>16);
                        pptimeout[2] = (uint8_t) (pptime>>8);
                        pptimeout[3] = (uint8_t) (pptime);
                        usb_write(pptimeout,4);
                        
                        if (write) {

                            partial_write(address, Nbyte, write_buffer, pptime); 
                        }

                        //page number
                        uint8_t ikout = (uint8_t) (ik);
                        usb_write(&ikout,1);
                        
                        read(address, Nbyte, read_buffer);
                        usb_write(read_buffer, Nbyte);
                        usb_write((uint8_t *) "Done.", 5);

                    }


                    if (write) {
                        usb_write((uint8_t *) "!<3<3<3!", 8);
                    }
                    else {
                        usb_write((uint8_t *) "!=)=)=)!", 8);
                    }

                }

                
                usb_write((uint8_t *) "DONEDONEDONE", 12);

            }

		} // end if (usb_read_ready())
	} // end while(1)

	return 0;
}

void readID(uint8_t *dest)
{
	uint8_t i;
	// Activate the chip and set up I/O
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;
	//cmd input
	write_cmd_word(CMD_READ_SIGNATURE);
	
	//address input
	FIO0SET = P_ADDR_LATCH | P_READ_ENABLE;
	FIO0CLR = P_CMD_LATCH;// | P_WRITE_ENABLE;	
	
	WRITE_WITH_FLOP(0x00);
	
	FIO0CLR = P_ADDR_LATCH;
	//tAR and tWHR
	WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
	
	for (i = 0; i < 5; ++i) {
		FIO0CLR = P_READ_ENABLE; //WAIT;
		*(dest+i) = FIO0PIN2;// WAIT;
		FIO0SET = P_READ_ENABLE; //WAIT;
	}
	DEASSERT_CHIP_ENABLE;
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
	DEASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;
}

/**
 * Erases an entire block (the smallest granularity possible) from the device
 */
uint8_t complete_erase(uint32_t address, uint8_t *otime1) {
	uint8_t result;
	uint32_t otime;
	
	//set up timer 0 to moniter the erase time latency
	T0MCR=0x00;  //stop comparing
	T0PR=0;  //prescaler
	T0TCR=2; //stop and reset time
	
	// Activate the chip and set up I/O
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;

	// Set up control signals
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE | P_ADDR_LATCH;
	FIO0SET = P_WRITE_PROTECT | P_READ_ENABLE;

	
	// Write the first word for erase
	write_cmd_word(CMD_BLOCK_ERASE_1);

	// Write address for it
	write_address(address, 1);

	// Write second command word
	write_cmd_word(CMD_BLOCK_ERASE_2);
	
	T0TCR=1; //start the timer
	
	FIO0CLR = P_READ_ENABLE;

	// Change to input
	SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
	
	// Wait for erase to finish
	WAIT_FOR_BUSY;
	
	//record the time
	otime=T0TC;
	T0TCR=2; //stop and reset time
	otime1[0] = (uint8_t) (otime >> 24);  //time used
	otime1[1] = (uint8_t) (otime >> 16);
	otime1[2] = (uint8_t) (otime >> 8);
	otime1[3] = (uint8_t) (otime);
	
	reset_io();
	DEASSERT_CHIP_ENABLE;

	// Read status and find out if the program was successful, returning status
	result = read_status();
	if ((result & SR_GENERIC_ERROR) == SR_GENERIC_ERROR)
		return 1;
	return 0;
}

/**
 * Erases an entire block (the smallest granularity possible) from the device
 */
uint8_t partial_erase(uint32_t address, uint32_t terase) {
	//  *********control timing******
	T0PR=0;  //one tick is 0.01 ms
	T0TCR=2; //stop and reset time
	T0MCR=0x20;
	T0MR1=terase;  //make a 0.7ms delay, almost half of the erase time 2082 2084 2080 last time    2088  last 90 last time 92
	//  *********************************************

	uint8_t result;
	
	// Activate the chip and set up I/O
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;

	// Set up control signals
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE | P_ADDR_LATCH;
	FIO0SET = P_WRITE_PROTECT | P_READ_ENABLE;

	
	// Write the first word for erase
	write_cmd_word(CMD_BLOCK_ERASE_1);

	// Write address for it
	write_address(address, 1);

	// Write second command word
	write_cmd_word(CMD_BLOCK_ERASE_2);
	FIO0CLR = P_READ_ENABLE;

	// Change to input
	SET_IO_AS_INPUT; 
	
	// **********control the timing*************
	T0TCR=1; //start the timer
	while ((T0TCR&1)==1);  //time delay
	//  **************end of control the timing***************
	// ***********what I add*******
	SET_IO_AS_OUTPUT;

	// Set up control signals
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE | P_ADDR_LATCH;
	FIO0SET = P_WRITE_PROTECT | P_READ_ENABLE;

	
	// Write the first word for erase
	write_cmd_word(0xFF);  //reset to abort the erase
	FIO0CLR = P_READ_ENABLE;

	// Change to input
	SET_IO_AS_INPUT;  WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
	// *************end of what I add*********************
	
	// Wait for erase to finish
	WAIT_FOR_BUSY;
	reset_io();
	DEASSERT_CHIP_ENABLE; 

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
	SET_IO_AS_INPUT; 

	// Drive Read low to load the status register onto the I/O pins
	FIO0CLR = P_READ_ENABLE; WAIT;
	result = FIO0PIN2;

	// Clear chip enable and return results
	DEASSERT_CHIP_ENABLE; 
	reset_io();
	return result;
}

/**
 * Partial program write. Takes tprogram which is an integer. 
 * This time is the time (divided by 30 MHz) that will be used for programming.
 * The other program function takes a timer to report program time (otime1). 
 * For this, since we specify a tprogram, that is what the timer will report.
 */
uint8_t partial_write(uint32_t address, uint32_t count, const uint8_t *src, uint32_t tprogram) {
	uint32_t i;
	uint8_t result;

	//  *********control timing******
	T0PR=0;  //one tick is 0.01 ms
	T0TCR=2; //stop and reset time
	T0MCR=0x20;
	T0MR1=tprogram;  
	//  *********************************************

	// Activate the chip
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;
	FIO0SET = P_WRITE_ENABLE; 
	
	// Send the first half of the page program command
	write_cmd_word(CMD_PAGE_PROGRAM_1);

	// Send the address to start at (multiple bus cycles)
	write_address(address, 0);

	// With the address loaded, the data now needs to be loaded
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE | P_ADDR_LATCH;
	FIO0SET = P_READ_ENABLE | P_WRITE_PROTECT;

	// Actually send out the data
	for (i = 0; i < count; ++i) {
		FIO0CLR = P_WRITE_ENABLE; 
		WRITE_VALUE(*(src+i));
		FIO0SET = P_WRITE_ENABLE; 
	}

	// Now send out the final command to begin programming
	write_cmd_word(CMD_PAGE_PROGRAM_2);

	// Just to be safe go back to input mode
	SET_IO_AS_INPUT;
	
	// **********control the timing*************
	T0TCR=1; //start the timer
	while ((T0TCR&1)==1);  //time delay
	//  **************end of control the timing***************
	// ***********what I add*******
	SET_IO_AS_OUTPUT;
	
	// Reset 
	

	write_cmd_word(0xFF);  //reset to abort the program

	// Change to input
	SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
	// *************end of what I add*********************

	// Wait for programming to finish
	WAIT_FOR_BUSY;
	T0TCR=2; //stop and reset time
	
	DEASSERT_CHIP_ENABLE; 
	reset_io();

	// Read status and find out if the program was successful, returning status
	result = read_status();
	if ((result & SR_GENERIC_ERROR) == SR_GENERIC_ERROR)
		return 1;
	return 0;
}

/**
 * Sequentially programs some number of bytes starting at a given address.
 */
uint8_t complete_write(uint32_t address, uint32_t count, const uint8_t *src, uint8_t *otime1) {
	uint32_t i, otime;
	uint8_t result;
	
	//set up timer 0 to moniter the program time latency
	T0MCR=0x00;  //stop comparing
	T0PR=0;  //prescaler
	T0TCR=2; //stop and reset time

	// Activate the chip
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;
	FIO0SET = P_WRITE_ENABLE; 
	
	// Send the first half of the page program command
	write_cmd_word(CMD_PAGE_PROGRAM_1);

	// Send the address to start at (multiple bus cycles)
	write_address(address, 0);

	// With the address loaded, the data now needs to be loaded
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE | P_ADDR_LATCH;
	FIO0SET = P_READ_ENABLE | P_WRITE_PROTECT;

	// Actually send out the data
	for (i = 0; i < count; ++i) {
		FIO0CLR = P_WRITE_ENABLE; 
		WRITE_VALUE(*(src+i));
		FIO0SET = P_WRITE_ENABLE; 
	}

	// Now send out the final command to begin programming
	write_cmd_word(CMD_PAGE_PROGRAM_2);
	
	T0TCR=1; //start the timer

	// Just to be safe go back to input mode
	SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
	
	

	// Wait for programming to finish
	WAIT_FOR_BUSY;
	
	otime=T0TC;
	T0TCR=2; //stop and reset time
	otime1[0] = (uint8_t) (otime >> 24);  //time used
	otime1[1] = (uint8_t) (otime >> 16);
	otime1[2] = (uint8_t) (otime >> 8);
	otime1[3] = (uint8_t) (otime);
	
	DEASSERT_CHIP_ENABLE; 
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
/*	if (count > 2112)
		count = 2112;*/

	// Activate the chip
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;

	
	// Send first half of the read command
	write_cmd_word(CMD_READ_1);

	// Send the address (multiple bus cycles)
	write_address(address, 0);

	// With the address loaded, write the read confirm command
	write_cmd_word(CMD_READ_2);
	
	// Change to input type pins
	SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;

	// Now, wait for the busy signal to become de-asserted
	WAIT_FOR_BUSY;

	
		for (i = 0; i < count; ++i) {
			FIO0CLR = P_READ_ENABLE; WAIT;
			*(dest+i) = FIO0PIN2; WAIT;
			FIO0SET = P_READ_ENABLE; WAIT;
		}
	
	// Disable the chip once we are done
	DEASSERT_CHIP_ENABLE;
	reset_io();
}

/**
 * read the same page ntimes using cache read mode
 */
void cache_read(uint32_t address, uint32_t count, uint8_t *dest, uint32_t ntimes) {
	uint32_t i, j;

	// Activate the chip
	ASSERT_CHIP_ENABLE;
	SET_IO_AS_OUTPUT;

	// ***************************
	// The first part is normal read
	// ***************************
	// Send first half of the read command
	write_cmd_word(CMD_READ_1);

	// Send the address (multiple bus cycles)
	write_address(address, 0);

	// With the address loaded, write the read confirm command
	write_cmd_word(CMD_READ_2);
	
	// Change to input type pins, needed for check busy?
	SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;

	// Now, wait for the busy signal to become de-asserted
	WAIT_FOR_BUSY;
	
	for (j=0;j<(ntimes-1);j++)
	{
		SET_IO_AS_OUTPUT;
		// **************************
		// Now enter enhanced cache read mode
		// ****************************
		// Send first half of the read command
		write_cmd_word(CMD_CACHE_READ_1);

		// Send the address (multiple bus cycles)
		write_address(address, 0);	
		
		// With the address loaded, write the read confirm command
		write_cmd_word(CMD_CACHE_READ_2);

		// Change to input type pins
		SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
		
		// Now, wait for the busy signal to become de-asserted
		WAIT_FOR_BUSY;
		
		//pulse the R_ to output data
		for (i = 0; i < count; ++i) {
			FIO0CLR = P_READ_ENABLE; WAIT;
			*(dest+i) = FIO0PIN2; WAIT;
			FIO0SET = P_READ_ENABLE; WAIT;
		}
		
		//this means that when using the cache read function, count should be smaller than 64
		usb_write(dest, count);
	}
	
	// *****************************
	// got the last read data and exit cache read mode
	// *****************************
	SET_IO_AS_OUTPUT;
	write_cmd_word(CMD_EXIT_CACHE_READ);
	
	// Change to input type pins
	SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
		
	// Now, wait for the busy signal to become de-asserted
	WAIT_FOR_BUSY;
		
	//pulse the R_ to output data
	for (i = 0; i < count; ++i) {
		FIO0CLR = P_READ_ENABLE; WAIT;
		*(dest+i) = FIO0PIN2; WAIT;
		FIO0SET = P_READ_ENABLE; WAIT;
	}
		
	//this means that when using the cache read function, count should be smaller than 64
	usb_write(dest, count);
	
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

	// Send the suggested command
	WRITE_WITH_FLOP(cmd);

	// Clear the command latch so we don't have any weird timing problems
	FIO0CLR = P_CMD_LATCH; 
}

/**
 * Helper function that writes an address to the bus
 */
void write_address(uint32_t address, uint8_t doErase) {
	// Set up for address input, according to table 4 on page 16
	FIO0SET = P_ADDR_LATCH| P_READ_ENABLE;
	FIO0CLR = P_CMD_LATCH | P_WRITE_ENABLE;

	// The address is loaded in 8-bit segments in the sequence specified on page 9, section 1.6

	// These two blocks are only loaded if we need a full address
	if (!doErase) {
		// First, load the lowest 8 bytes
		WRITE_WITH_FLOP((uint8_t)(address & 0x000000ff));
	
		// Next load in 2 zeros along with the 6 next MSBs
		WRITE_WITH_FLOP((uint8_t)((address & 0x00003f00) >> 8));
	}
	
	// Next, load in the next eight bits (14-21)
	WRITE_WITH_FLOP((uint8_t)((address & 0x003fc000) >> 14));

	// Next eight bits (22-29)
	WRITE_WITH_FLOP((uint8_t)((address & 0x3fc00000) >> 22));

	// Last 2 bits (30-31)
	WRITE_WITH_FLOP((uint8_t)((address & 0xc0000000) >> 30));

	// De-assert the address latch now, just in case
	FIO0CLR = P_ADDR_LATCH;
}
