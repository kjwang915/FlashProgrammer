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

#define mylen 512  //write for hiding, use 512 bytes, which are 512 bits
#define Nbyte 512  //read 512 bytes
#define Nbit 4096 //Nbit=Nbyte*8
#define Ncount 128  //assign 128 bits to a group
#define Ngroups 32  //Number of groups Nbit bits are divided into
#define myMsk 0x1F  //for Ngroup 32, the last 5 bits of generated random location is useful
#define Ntimes 1
#define NP 16//number of used pages (64 pages forms 32 rows. In each row, only one page is used (perphaps we can use both of the pages)
#define Npages 64  //the first Npages pages are used in a block
#define Intv 4  //Npages=Intv*(NP-1)+1 or +2

uint8_t write_buffer[NP][mylen];
uint8_t write_buffer2[mylen];
uint8_t read_buffer[Nbyte];
uint16_t bitrank[Nbyte][8];  //record the bit program rank
uint8_t obuffer[64];  //output buffer
uint8_t myASG[Nbyte][8];  //group assignment for each bit
uint8_t myflags[Ngroups];
uint32_t tprogram, terase;
//now, one 512 bytes can hide 32 bits which are four letters
//then this message need ceil(44/4) 11 pages, just for testing
//for this message, there are 44 ascii letters
//const uint8_t hMessage[]="The quick brown fox jumps over the lazy dog.";
/**
 * Application entry point
 */
int main(void) {
	uint32_t otime, i;
	
	uint32_t address0, address;
	uint8_t cmd[20];
	uint8_t otime1[4]; 
	uint16_t block, byte, j, nn, count, zz;  
	uint8_t bit, page,  mynum, k;  
	
	uint8_t result;
	
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
			if (strncmp((char *)cmd, "m", 1) == 0) {
				
				//timer 1 setup
				T1MCR=0x00;  //stop comparing
				T1PR=99;  //prescaler
			
				usb_write((uint8_t *) "Done.", 5);
				
				address0 = (((uint32_t) 0x00) << 26) | (((uint32_t) 0x00) << 18) | (((uint32_t) (0x00)) << 12);  //lower page
				address=address0;
				
				srand(351);
				//put on random stress
				for (i=0;i<1000;i++) //10 times
				{
					for (block=1620;block<1625;block=block+1)   //these three must be consistent
					{
						address=address0 | (((uint32_t) block) << 18);

						result = complete_erase(address, otime1);  //complete erase
						
						for (page=0;page<64;page++)
						{
							address=address0 | (((uint32_t) block) << 18) | (((uint32_t) page) << 12);
							for (j=0;j<Nbyte;j++)
							{
								write_buffer2[j]= (uint8_t) rand();
							}
							
							result = write(address, mylen, write_buffer2, otime1); 
						}
						
					}
				}
				
				memset(write_buffer2, 0x00, mylen ); 
				//characterization part	
				for (i=0;i<Ntimes;i++)
				{
					for (block=1620;block<1625;block=block+1) 
					{
						//erase
						address=address0 | (((uint32_t) block) << 18);
						result = complete_erase(address, otime1);  //complete erase
						usb_write(otime1,4);  //output the erase time again, it may be different from the first erase time
						insert_delay(99);
						
						//complete write all of the block, also measure the program time
						for (page=0;page<64;page=page+1)  //64 pages
						{						
							address=address0 | (((uint32_t) block) << 18) | (((uint32_t) page) << 12);
							result = write(address, mylen, write_buffer2, otime1); 
							usb_write(otime1,4);  //output the program time for each page
							insert_delay(99);
						}  
						//erase
						address=address0 | (((uint32_t) block) << 18);
						result = complete_erase(address, otime1);  //complete erase, this erase is actually used
						usb_write(otime1,4);  //output the erase time again, it may be different from the first erase time
						insert_delay(99);
						//characterization, which is another program
						for (page=0;page<64;page=page+Intv)  //16 pages
						{						
							address=address0 | (((uint32_t) block) << 18) | (((uint32_t) page) << 12);
							memset(bitrank, 0x00, Nbit * 2);  //bitrank is uint16_t
							
							
							T1TCR=2; //stop and reset time				
							T1TCR=1; //start the timer
				
							tprogram=650;   //to be determined  810
							for (nn=0;nn<1200;nn++)
							{
								result = incomplete_write(address, mylen, write_buffer2);  //program the whole page
											
								read(address, Nbyte, read_buffer);  //read while output
																
								for (byte=0; byte<Nbyte; byte++)
								{
									for (bit=0; bit<8; bit++)
									{
										
										if ( (bitrank[byte][bit]==0x0000) && ((read_buffer[byte] & (0x01<<bit))==0x00))
										{
											bitrank[byte][bit]=nn+1;  //this should be nn+1
										}
									}
								}	
							}

							otime=T1TC;
							T1TCR=2; //stop and reset time
							
							count=0;
							for (byte=0; byte<Nbyte; byte++)
							{
								for (bit=0; bit<8; bit++)
								{
									obuffer[count]=(uint8_t) (bitrank[byte][bit]>>8);
									count++;
									obuffer[count]=(uint8_t) (bitrank[byte][bit]);
									count++;
								}
								if (count==64)
								{
									count=0;
									usb_write(obuffer,64);
								}
							}
							otime1[0] = (uint8_t) (otime >> 24);  //time used
							otime1[1] = (uint8_t) (otime >> 16);
							otime1[2] = (uint8_t) (otime >> 8);
							otime1[3] = (uint8_t) (otime);
							usb_write(otime1,4);

							usb_write((uint8_t *) "Done.", 5);	
							
							//insert some delay, because hynix chips need this
							insert_delay(99);				
						}  //end of pages
						//erase
						address=address0 | (((uint32_t) block) << 18);
						result = complete_erase(address, otime1);  //complete erase	
						usb_write(otime1,4);  //output the erase latenty again
						insert_delay(99);
						usb_write((uint8_t *) "Done.", 5);	
					}  //end of Nblocks
				} //end of Ntimes	
				usb_write((uint8_t *) "Fini.", 5);	
			}  //end of if strcomp
		}
	}

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
uint8_t erase(uint32_t address) {
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
 * Sequentially programs some number of bytes starting at a given address.
 */
uint8_t incomplete_write(uint32_t address, uint32_t count, const uint8_t *src) {
	uint32_t i;
	uint8_t result;

	//  *********control timing******
	T0PR=0;  //one tick is 0.01 ms
	T0TCR=2; //stop and reset time
	T0MCR=0x20;
	T0MR1=tprogram;  //make a 0.7ms delay, almost half of the erase time
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
	
	// Write the first word for erase
	write_cmd_word(0xFF);  //reset to abort the erase

	// Change to input
	SET_IO_AS_INPUT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT; WAIT;
	// *************end of what I add*********************

	// Wait for programming to finish
	WAIT_FOR_BUSY;
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
uint8_t write(uint32_t address, uint32_t count, const uint8_t *src, uint8_t *otime1) {
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
