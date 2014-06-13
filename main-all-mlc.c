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
 *   This code runs the endurance tests 
 */


#define Nbyte 8640  //read 8640 bytes but only record every 4 bytes
#define Nbytebase 8192
#define Nbytespare 448
#define Nbit 69120  //Nbit=Nbyte*8
#define Nblocks 1024 // number of blocks in a Hynix 4gbit part
#define Npages 256 // number pages per block

/*******************************
** Endurance related #defines **
*******************************/
//#define blocktoprogram 568 // block 568. i messed up block 567 good on the test chip
//#define blocktoprogram 572 // now defined in endurance subroutine
#define pagetoprogram 32 // let's wear out page 32 I guess. 
//#define pptimetoprogram 6000
// 6000 is to represent complete program. in loop below, 6000 is detected then complete_write is used, not partial_write
#define intervaltocheck 10000  // 10k
#define pecyclestotry 10000000 // 10 million

/*******************************
** Retention related #defines **
** also measurement           **
*******************************/
#define blocklistsize 10
#define entireblocklistsize 2
#define pptimessize 1
// 13 partial program times
uint16_t pptimes[pptimessize] = {6000};
// 10blocks, 26 pages each - 160 pages total
uint16_t blocklist[blocklistsize] = {12, 23, 56, 68, 199, 345, 500, 670, 800, 988};

// uint16_t entireblocklist[10] = {500, 700, 950, 1620, 1950, 2750, 3120, 3300, 3700, 4020}; // 10 regions of 8 blocks. starting block.
// 2 regions of 13 blocks. starting block. 26 blocks total!! x 64 pages... a lot 1664 pages
//uint16_t entireblocklist[entireblocklistsize] = {300, 856};


/*******************************************************
** Combined partial erase / partial program endurance **
*******************************************************/
#define ppeetimessize 5
#define eetimessize 6
#define eeblocklistsize 2
//uint16_t ppeetimes[ppeetimessize] = {2500, 3000, 3500, 4000, 4500};
//uint16_t eetimes[eetimessize] = {500, 1000, 2000, 5000, 10000, 12500};
// need 5 * 6 contiguous blocks = 30
//uint16_t eeblocklist[eeblocklistsize] = {700, 3400}; // 700-729, 3400-3429

/**************************
** partial erase testing **
**************************/
// use same blocks as blocklist

/**********************************************************************
** Page list for partial program tests, current not used by combined **
**********************************************************************/
//uint8_t pagelist[pptimessize] = {2,3,6,9,12,13,15,18,21,24,27,30,31}; // add 32 to get other page
//uint8_t secondpagelist[pptimessize] = {34,35,38,41,44,45,47,50,53,56,59,62,63};



/**************************************
** Global read/write buffers         **
** Keep these global, not in main()  **
** unsure what the compiler is doing **
** but programs break when you have  **
** these big buffers in functions    **
**************************************/
uint8_t write_buffer[Nbyte];
uint8_t read_buffer[Nbyte];
uint8_t * readbufNbytebaseptr = (read_buffer + Nbytebase);

// address from which things are derived. I'm pretty sure this is all 0
// I'm not sure why we have this
uint32_t address0 = (((uint32_t) 0x00) << 26) | (((uint32_t) 0x00) << 22) | (((uint32_t) (0x00)) << 14); 

// timer
//clock_t timer;

uint8_t otimep[4]; 
uint8_t otimee[4]; 


/**
 * Application entry point
 */
int main(void) {

 	uint32_t i, j, k;

 	uint32_t address;
 	uint8_t cmd[20];

 	uint8_t result;

 	uint16_t block, pptime, eetime;
 	uint8_t page;

 	uint32_t pecycles = 0;
 	uint32_t interval = 0;


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

            // ************************************
            // partial erase + partial program test
            // - endurance
            // ************************************
            
/*            if (strncmp((char *) cmd, "e", 1) == 0) {

                //uint8_t erase = 0;

                // detect if erase or read
                //if (strncmp((char *) cmd, "ee", 2) == 0) {
                //    wearout = 1;
                //}
                
                // set write to 0x00
                memset(write_buffer, 0x00, Nbyte);

                page = pagetoprogram;


                // decide which block, pp time and pe time we are using
                if (strncmp((char *) cmd, "e11", 3) == 0) {

                    block = 700;
                    pptime = ppeetimes[0];
                    eetime = eetimes[0];

                }  
                else if (strncmp((char *) cmd, "e12", 3) == 0) {
                    
                    block = 701;
                    pptime = ppeetimes[0];
                    eetime = eetimes[1];
                
                }
                else if (strncmp((char *) cmd, "e13", 3) == 0) {

                    block = 702;
                    pptime = ppeetimes[0];
                    eetime = eetimes[2];

                }
                else if (strncmp((char *) cmd, "e14", 3) == 0) {

                    block = 703;
                    pptime = ppeetimes[0];
                    eetime = eetimes[3];

                }
                else if (strncmp((char *) cmd, "e15", 3) == 0) {

                    block = 704;
                    pptime = ppeetimes[0];
                    eetime = eetimes[4];

                }
                else if (strncmp((char *) cmd, "e16", 3) == 0) {

                    block = 705;
                    pptime = ppeetimes[0];
                    eetime = eetimes[5];

                }
                else if (strncmp((char *) cmd, "e21", 3) == 0) {

                    block = 706;
                    pptime = ppeetimes[1];
                    eetime = eetimes[0];

                }
                else if (strncmp((char *) cmd, "e22", 3) == 0) {

                    block = 707;
                    pptime = ppeetimes[1];
                    eetime = eetimes[1];

                }
                else if (strncmp((char *) cmd, "e23", 3) == 0) {

                    block = 708;
                    pptime = ppeetimes[1];
                    eetime = eetimes[2];

                }
                else if (strncmp((char *) cmd, "e24", 3) == 0) {

                    block = 709;
                    pptime = ppeetimes[1];
                    eetime = eetimes[3];

                }
                else if (strncmp((char *) cmd, "e25", 3) == 0) {

                    block = 710;
                    pptime = ppeetimes[1];
                    eetime = eetimes[4];

                }
                else if (strncmp((char *) cmd, "e26", 3) == 0) {

                    block = 711;
                    pptime = ppeetimes[1];
                    eetime = eetimes[5];

                }
                else if (strncmp((char *) cmd, "e31", 3) == 0) {

                    block = 712;
                    pptime = ppeetimes[2];
                    eetime = eetimes[0];

                }
                else if (strncmp((char *) cmd, "e32", 3) == 0) {

                    block = 713;
                    pptime = ppeetimes[2];
                    eetime = eetimes[1];

                }
                else if (strncmp((char *) cmd, "e33", 3) == 0) {

                    block = 714;
                    pptime = ppeetimes[2];
                    eetime = eetimes[2];

                }
                else if (strncmp((char *) cmd, "e34", 3) == 0) {

                    block = 715;
                    pptime = ppeetimes[2];
                    eetime = eetimes[3];

                }
                else if (strncmp((char *) cmd, "e35", 3) == 0) {

                    block = 716;
                    pptime = ppeetimes[2];
                    eetime = eetimes[4];

                }
                else if (strncmp((char *) cmd, "e36", 3) == 0) {

                    block = 717;
                    pptime = ppeetimes[2];
                    eetime = eetimes[5];

                }
                else if (strncmp((char *) cmd, "e41", 3) == 0) {

                    block = 718;
                    pptime = ppeetimes[3];
                    eetime = eetimes[0];

                }
                else if (strncmp((char *) cmd, "e42", 3) == 0) {

                    block = 719;
                    pptime = ppeetimes[3];
                    eetime = eetimes[1];

                }
                else if (strncmp((char *) cmd, "e43", 3) == 0) {

                    block = 720;
                    pptime = ppeetimes[3];
                    eetime = eetimes[2];

                }
                else if (strncmp((char *) cmd, "e44", 3) == 0) {

                    block = 721;
                    pptime = ppeetimes[3];
                    eetime = eetimes[3];

                }
                else if (strncmp((char *) cmd, "e45", 3) == 0) {

                    block = 722;
                    pptime = ppeetimes[3];
                    eetime = eetimes[4];

                }
                else if (strncmp((char *) cmd, "e46", 3) == 0) {

                    block = 723;
                    pptime = ppeetimes[3];
                    eetime = eetimes[5];

                }
                else if (strncmp((char *) cmd, "e51", 3) == 0) {

                    block = 724;
                    pptime = ppeetimes[4];
                    eetime = eetimes[0];

                }
                else if (strncmp((char *) cmd, "e52", 3) == 0) {

                    block = 725;
                    pptime = ppeetimes[4];
                    eetime = eetimes[1];

                }
                else if (strncmp((char *) cmd, "e53", 3) == 0) {

                    block = 726;
                    pptime = ppeetimes[4];
                    eetime = eetimes[2];

                }
                else if (strncmp((char *) cmd, "e54", 3) == 0) {

                    block = 727;
                    pptime = ppeetimes[4];
                    eetime = eetimes[3];

                }
                else if (strncmp((char *) cmd, "e55", 3) == 0) {

                    block = 728;
                    pptime = ppeetimes[4];
                    eetime = eetimes[4];

                }
                else if (strncmp((char *) cmd, "e56", 3) == 0) {

                    block = 729;
                    pptime = ppeetimes[4];
                    eetime = eetimes[5];

                }
                else {

                    usb_write((uint8_t *) "STOP",4);
                    break;

                }
                
                // output prep
                uint8_t outputprep[9];

                //uint8_t blockout[2];
                outputprep[0] = (uint8_t) (block>>8);
                outputprep[1] = (uint8_t) (block);

                // page
                outputprep[2] = page;

                //uint8_t ppeetimeout[2];
                outputprep[3] = (uint8_t) (pptime>>8);
                outputprep[4] = (uint8_t) (pptime);

                //uint8_t eetimeout[2];
                outputprep[5] = (uint8_t) (eetime>>8);
                outputprep[6] = (uint8_t) (eetime);

                outputprep[7] = (uint8_t) "E";
                outputprep[8] = (uint8_t) "E";
                
                       
                address = address0 | (((uint32_t) block) << 22 ) | (((uint32_t) page) << 14);

                // start from complete erase 
                result = complete_erase(address,otimee);

                // wearout
                while (pecycles < pecyclestotry) {

                    if (usb_read_ready()) {
                        usb_read(cmd,4);

                        // If you want to stop wear out, type "stop" in the USB interface program
                        // The code will then report the last cycle it did and everything
                        if (strncmp((char *)cmd, "stop", 4) == 0) {

                            break;// stop!

                        } 
                    }



                    // do the partial program
                    result = partial_write(address, Nbyte, write_buffer, pptime);

                    // check program
                    if (interval == intervaltocheck) {

                        usb_write((uint8_t *) "PECYCLES", 8);
                        uint8_t pecyclesout[4];
                        pecyclesout[0] = (uint8_t) (pecycles>>24);
                        pecyclesout[1] = (uint8_t) (pecycles>>16);
                        pecyclesout[2] = (uint8_t) (pecycles>>8);
                        pecyclesout[3] = (uint8_t) (pecycles);
                        usb_write(pecyclesout, 4);

                        // print out page
                        //address = address0 | (((uint32_t) block ) << 22 ) | (((uint32_t) page) << 14 );
                        read(address, Nbyte, read_buffer);
                        usb_write(read_buffer, Nbytebase);
                        usb_write(readbufNbytebaseptr, Nbytespare);                        

                    }
                
                    // partial erase!
                    result = partial_erase(address, eetime);

                    // check erase
                    if (interval == intervaltocheck) {

                        // print out page
                        //address = address0 | (((uint32_t) block ) << 22 ) | (((uint32_t) page) << 14 );
                        read(address, Nbyte, read_buffer);
                        usb_write(read_buffer, Nbytebase);
                        usb_write(readbufNbytebaseptr, Nbytespare);                     

                        interval = 0; // reset to 0   

                    }
                

                    pecycles++;
                    interval++;
                    
                }


                
                usb_write((uint8_t *) "PECYCLES", 8);
                //insertdelay
                uint8_t pecyclesoutfinal[4];
                pecyclesoutfinal[0] = (uint8_t) (pecycles>>24);
                pecyclesoutfinal[1] = (uint8_t) (pecycles>>16);
                pecyclesoutfinal[2] = (uint8_t) (pecycles>>8);
                pecyclesoutfinal[3] = (uint8_t) (pecycles);
                usb_write(pecyclesoutfinal, 4);

                usb_write(outputprep+3, 2); //pptime
                usb_write(outputprep+5, 4); //eetime

                read(address, Nbyte, read_buffer);
                                                
                //output
                //usb_write(blockout,2);

                
                //usb_write(ppeetimeout,2);

                //usb_write(eetimeout,2);

                usb_write(outputprep, 7);
                

                insert_delay(99);

                usb_write(read_buffer,Nbytebase);
                //insertdelay
                usb_write(readbufNbytebaseptr, Nbytespare);
                //insertdelay

                //usb_write((uint8_t *) "Done.", 5);

                usb_write((uint8_t *) "DONEDONEDONE", 12);

            } // end if e do partial erase + partial program test */
            

            // ***************************
            // partial program test
			// w for WRITE! r is for READ!
            // ***************************
/*    		if ( (strncmp((char *) cmd, "w", 1) == 0) || (strncmp((char *)cmd, "r", 1) == 0) ) { 

                usb_write(cmd, 4);
                //insertdelay
    			uint8_t write = 0;

    			if (strncmp((char *)cmd, "w", 1) == 0) {
    				write = 1;
    			}

				// initial base address
    			//address0 = (((uint32_t) 0x00) << 26) | (((uint32_t) 0x00) << 22) | (((uint32_t) (0x00)) << 14); 

				// set write to 0x00
    			memset(write_buffer, 0x00, Nbyte);

				// iterate through blocklist
    			for (i = 0; i < blocklistsize; i++) {

    				uint8_t blockout[2];
    				blockout[0] = (uint8_t) (blocklist[i]>>8);
    				blockout[1] = (uint8_t) (blocklist[i]);


    				for (j = 0; j < pptimessize; j++) {

						//pptimes and pagelist
    					address = address0 | (((uint32_t) blocklist[i]) << 22 ) | (((uint32_t) pagelist[j]) << 14);

    					if (write) {
							// erase 
    						result = complete_erase(address,otimee);

							// partial program
    						result = partial_write(address, Nbyte, write_buffer, pptimes[j]);
    					}

						// read
    					read(address, Nbyte, read_buffer);

						// output useful stuff
    					usb_write(blockout, 2);
                        //insertdelay
    					usb_write(&pagelist[j], 1);
                        //insertdelay

    					if (write) {
    						usb_write((uint8_t *) "WW", 2);
    					}

    					uint8_t pptimeout[2];
    					pptimeout[0] = (uint8_t) (pptimes[j]>>8);
    					pptimeout[1] = (uint8_t) (pptimes[j]);
    					usb_write(pptimeout,2);
                        //insertdelay  
    					usb_write(read_buffer,Nbytebase);
                        insert_delay(99);
                        usb_write(readbufNbytebaseptr, Nbytespare);
                        insert_delay(99);
    					usb_write((uint8_t *) "Done.", 5);
                        //insertdelay
    				}
    				for (j = 0; j < pptimessize; j++) {
						/////////////////
						// second page //

    					address = address0 | (((uint32_t) blocklist[i]) << 22 ) | (((uint32_t) secondpagelist[j]) << 14); 

    					if (write) {
							// partial program
    						result = partial_write(address, Nbyte, write_buffer, pptimes[j]);
    					}

						// read
    					read(address, Nbyte, read_buffer);

						// output useful stuff
    					usb_write(blockout, 2);
                        //insertdelay
    					usb_write(&secondpagelist[j], 1);
                        //insertdelay

    					if (write) {
    						usb_write((uint8_t *) "WW", 2);
                            //insertdelay
    					}

    					uint8_t pptimeout[2];
    					pptimeout[0] = (uint8_t) (pptimes[j]>>8);
    					pptimeout[1] = (uint8_t) (pptimes[j]);
    					usb_write(pptimeout,2);
                        //insertdelay

    					usb_write(read_buffer,Nbytebase);
                        insert_delay(99);
                        usb_write(readbufNbytebaseptr, Nbytespare);
                        insert_delay(99);
                        usb_write((uint8_t *) "Done.", 5);						
                        //insertdelay

    				}

    			}

				// now do entireblocklist

                insert_delay(1000); // clear buffer

                for (i = 0; i < entireblocklistsize; i++) {

					// pptime
                	for (k = 0; k < pptimessize; k++) {

						insert_delay(99); // clear buffer

                		block = entireblocklist[i] + k;

                		address = address0 | (((uint32_t) block) << 22 );

                		if (write) {
							// erase 
                			result = complete_erase(address,otimee);
                		}

                		uint8_t blockout[2];
                		blockout[0] = (uint8_t) (block>>8);
                		blockout[1] = (uint8_t) (block);


                		uint8_t pptimeout[2];
                		pptimeout[0] = (uint8_t) (pptimes[k]>>8);
                		pptimeout[1] = (uint8_t) (pptimes[k]);

						// for every page
                		for (j = 0; j < 64; j++) {

                			address = address0 | (((uint32_t) block) << 22 ) | (((uint32_t) j) << 14);

                			if (write) {
								// partial program
                				result = partial_write(address, Nbyte, write_buffer, pptimes[k]);
                			}

							// read
                			read(address, Nbyte, read_buffer);

							// output useful stuff
                			usb_write(blockout, 2);
                            //insertdelay

                			usb_write(&j, 1);
                            //insertdelay

                			if (write) {
                				usb_write((uint8_t *) "WW", 2);
                                //insertdelay
                			}

                			usb_write(pptimeout,2);
                            insert_delay(99);

                			usb_write(read_buffer,Nbytebase);
                            insert_delay(99);
                            
                            usb_write(readbufNbytebaseptr, Nbytespare);
                            insert_delay(99);
                			
                			usb_write((uint8_t *) "Done.", 5);
                            //insertdelay
                		}

                	}

                }

                usb_write((uint8_t *)"DONEDONEDONE",12);
                //insertdelay

            } // end if w/r do partial program 
*/

            // *******************************
            // partial erase test
            // pe read
            // pee erase
            // *******************************
/*            else if (strncmp((char *) cmd, "pe", 2) == 0) {

                usb_write(cmd, 4);

                uint8_t perase = 0;

                if (strncmp((char *)cmd, "pee", 3) == 0) {
                    perase = 1;
                }

                // complete write then partial erase
                memset(write_buffer, 0x00, Nbyte);

                for (i = 0; i < eetimessize; i++) {

                    
                    uint8_t blockout[2];
                    blockout[0] = (uint8_t) (blocklist[i]>>8);
                    blockout[1] = (uint8_t) (blocklist[i]);

                    address = address0 | (((uint32_t) blocklist[i]) << 22 );
                    

                    if (perase) {
                        
                        result = complete_erase(address,otimee);

                        for (j = 0; j < Npages; j++) {
                            address = address0 | (((uint32_t) blocklist[i]) << 22 ) | (((uint32_t) j) << 14);
                            result = complete_write(address, Nbyte, write_buffer, otimep);
                        }

                        result = partial_erase(address, eetimes[i]);

                    }

                    // read only the pages in the pptimessize pagelist
                    for (k = 0; k < pptimessize; k++) {

                        address = address0 | (((uint32_t) blocklist[i]) << 22 ) | (((uint32_t) pagelist[k]) << 14);

                        // read
                        read(address, Nbyte, read_buffer);

                        usb_write(blockout, 2);
                        usb_write(&pagelist[k], 1);

                        if (perase) {
                            usb_write((uint8_t *) "PEE", 3);
                        }

                        uint8_t eetimeout[2];
                        eetimeout[0] = (uint8_t) (eetimes[i]>>8);
                        eetimeout[1] = (uint8_t) (eetimes[i]);
                        usb_write(eetimeout,2);

                        usb_write(read_buffer,Nbytebase);
                        insert_delay(99);
                        usb_write(readbufNbytebaseptr, Nbytespare);
                        insert_delay(99);
                        usb_write((uint8_t *) "Done.", 5);
                    }
                    for (k = 0; k < pptimessize; k++) {

                        address = address0 | (((uint32_t) blocklist[i]) << 22 ) | (((uint32_t) secondpagelist[k]) << 14);

                        // read
                        read(address, Nbyte, read_buffer);

                        usb_write(blockout, 2);
                        usb_write(&secondpagelist[k], 1);

                        if (perase) {
                            usb_write((uint8_t *) "PEE", 3);
                        }

                        uint8_t eetimeout[2];
                        eetimeout[0] = (uint8_t) (eetimes[i]>>8);
                        eetimeout[1] = (uint8_t) (eetimes[i]);
                        usb_write(eetimeout,2);

                        usb_write(read_buffer,Nbytebase);
                        insert_delay(99);
                        usb_write(readbufNbytebaseptr, Nbytespare);
                        insert_delay(99);
                        usb_write((uint8_t *) "Done.", 5);
                    }



                }
                usb_write((uint8_t *)"DONEDONEDONE",12);

            }*/

            // ******** MEASUREMENT ***************************************************
			// m for Measure characteristic times (normal program, normal erase time)
            // measure each pptime on each block in blocklist
            // ************************************************************************
            
            if (strncmp((char *) cmd, "m", 1) == 0) { 

            	//usb_write((uint8_t *) "MM", 2);
                //insertdelay

				// program 0x00 to all bits
            	memset(write_buffer, 0x00, Nbyte);

                // for each partial program time
            	for (i = 0; i < pptimessize; i++) {

                    // for each block
            		for (j = 0; j < blocklistsize; j++) {

						// determine block and output block number
            			uint8_t blockout[2];
            			blockout[0] = (uint8_t) (blocklist[j]>>8);
            			blockout[1] = (uint8_t) (blocklist[j]);
            			usb_write(blockout,2);
                        //insertdelay

						// address of block
            			address = address0 | (((uint32_t) blocklist[j] ) << 22 ) | (((uint32_t) (0x00)) << 14 );

						// do an erase first
            			result = complete_erase(address, otimee);

						// output erase time 1
            			usb_write(otimee,4);
                        //insertdelay

						// write partial program time
            			uint8_t pptimeout[2];
            			pptimeout[0] = (uint8_t) (pptimes[i]>>8);
            			pptimeout[1] = (uint8_t) (pptimes[i]);
            			usb_write(pptimeout,2);
                        //insertdelay

						// do the partial program
            			if (pptimes[i] != 6000) {

							// partial program each page in the block
            				for (k = 0; k < Npages; k++) {

            					address = address0 | (((uint32_t) blocklist[j] ) << 22 ) | (((uint32_t) k) << 14 );
            					//timer = clock();
                                result = partial_write(address, Nbyte, write_buffer, pptimes[i]);
                                //timer = clock() - timer;
                                

            				}

            			}
            			else if (pptimes[i] == 6000) {

            				for (k = 0; k < Npages; k++) {
                                //usb_write((uint8_t *) "P", 1);

            					address = address0 | (((uint32_t) blocklist[j] ) << 22 ) | (((uint32_t) k) << 14 );
            					result = complete_write(address, Nbyte, write_buffer, otimep);
								// usb_write time req'd
            					usb_write(otimep, 4);
                                //insertdelay

            				}							

            			}


            			address = address0 | (((uint32_t) blocklist[j] ) << 22 ) | (((uint32_t) (0x00)) << 14 );
						// now erase and time that
                        //usb_write((uint8_t *) "erase", 5);
            			result = complete_erase(address, otimee);
            			usb_write(otimee, 4);
                        //insertdelay

            			usb_write((uint8_t *) "Done.", 5);
                        //insertdelay


            		}

            	}

            	usb_write((uint8_t *) "DONEDONEDONE" , 12);
                //insertdelay

            } // end if m do measurement
            

            // ************* WEAR OUT *******************
			// o for wear Out
            // ******************************************
/*            else if (strncmp((char *) cmd, "o", 1) == 0) { 

				// initial base address
            	//address0 = (((uint32_t) 0x00) << 26) | (((uint32_t) 0x00) << 22) | (((uint32_t) (0x00)) << 14); 

				// partial program 0
            	memset(write_buffer, 0x00, Nbyte);


                // write partial program time
                // if op1 use 3000
                // if op2 use 4000
                // if op3 use 5000
                if (strncmp((char *) cmd, "op1", 3) == 0) {
                    pptime = 3000;
                    block = 570;
                }
                else if (strncmp((char *) cmd, "op2", 3) == 0) {
                    pptime = 4000;
                    block = 568;
                }
                else if (strncmp((char *) cmd, "op3", 3) == 0) {
                    pptime = 5000;
                    block = 571;
                }
                else {
                    pptime = 6000; // complete program
                    block = 572;
                }


				// determine block and output block number
            	//block = blocktoprogram;

            	uint8_t blockout[2];
            	blockout[0] = (uint8_t) (block>>8);
            	blockout[1] = (uint8_t) (block);
            	usb_write(blockout,2);
				// insert delay just to be safe
				//insertdelay

            	page = pagetoprogram;
				//usb_write(&page,1);
			    ////insertdelay



            	uint8_t pptimeout[2];
            	pptimeout[0] = (uint8_t) (pptime>>8);
            	pptimeout[1] = (uint8_t) (pptime);
            	usb_write(pptimeout,2);
				//insertdelay

				// address to wear out
            	address = address0 | (((uint32_t) block ) << 22 ) | (((uint32_t) page) << 14 );

				// for the desired program time
				// program a page (32) in the block, then erase it.
            	while (pecycles < pecyclestotry) {

            		if (usb_read_ready()) {
            			usb_read(cmd,4);

						// If you want to stop wear out, type "stop" in the USB interface program
						// The code will then report the last cycle it did and everything
            			if (strncmp((char *)cmd, "stop", 4) == 0) {

							break;// stop!

						}

					}


                    // do an erase first
                    result = complete_erase(address, otimee);

					if (interval == intervaltocheck) {

						usb_write((uint8_t *) "PECYCLES", 8);
						//insertdelay
						uint8_t pecyclesout[4];
						pecyclesout[0] = (uint8_t) (pecycles>>24);
						pecyclesout[1] = (uint8_t) (pecycles>>16);
						pecyclesout[2] = (uint8_t) (pecycles>>8);
						pecyclesout[3] = (uint8_t) (pecycles);
						usb_write(pecyclesout, 4);
						//insertdelay
                        //usb_write(pptimeout,2); // program time
                        usb_write(otimee,4); // erase time

						// print out page
						address = address0 | (((uint32_t) block ) << 22 ) | (((uint32_t) page) << 14 );
						read(address, Nbyte, read_buffer);
						usb_write(read_buffer, Nbytebase);
                        usb_write(readbufNbytebaseptr, Nbytespare);
						//insertdelay
                    }
					

					

					// do the partial program
					// NOTE this is FOR COMPLETE_WRITE!! if partial programming need to change function call!!
					//for (j = 0; j < Npages; j++) {
					address = address0 | (((uint32_t) block ) << 22 ) | (((uint32_t) page ) << 14 );
					if (pptime == 6000) {
                        result = complete_write(address, Nbyte, write_buffer, otimep); 
                    }
                    else {
                        result = partial_write(address, Nbyte, write_buffer, pptime);
                    }
						//if (result) {
							//usb_write((uint8_t *) "ERROR", 5);
							//usb_write(&j,1);
							//break;
						//}

					//}

                    if (interval == intervaltocheck) {   
                        
                        // print out page
                        address = address0 | (((uint32_t) block ) << 22 ) | (((uint32_t) page) << 14 );
                        read(address, Nbyte, read_buffer);
                        usb_write(read_buffer, Nbytebase);
                        usb_write(readbufNbytebaseptr, Nbytespare);    

                        interval = 0; // reset to 0

                    }



					pecycles++;
					interval++;

				}


				usb_write((uint8_t *) "PECYCLES", 8);
				//insertdelay
				uint8_t pecyclesoutfinal[4];
				pecyclesoutfinal[0] = (uint8_t) (pecycles>>24);
				pecyclesoutfinal[1] = (uint8_t) (pecycles>>16);
				pecyclesoutfinal[2] = (uint8_t) (pecycles>>8);
				pecyclesoutfinal[3] = (uint8_t) (pecycles);
				usb_write(pecyclesoutfinal, 4);
				//insertdelay
                usb_write(otimep,4); // program time
                usb_write(otimee,4);

				// print out page
				read(address, Nbyte, read_buffer);
				usb_write(read_buffer, Nbytebase);
				//insertdelay
                usb_write(readbufNbytebaseptr, Nbytespare);

				usb_write((uint8_t *) "DONEDONEDONE", 12); // all done!
				//insertdelay
				//usb_user_init();

			} // end if o for wear out*/

            // ****************************
            // Simple read / write / erase 
            // ****************************
            
            else if ( (strncmp((char *) cmd, "sr", 2) == 0) || (strncmp((char *) cmd, "sw", 2) == 0) || (strncmp((char *) cmd, "se", 2) == 0)) {

                uint8_t blockyes = 0;
                uint8_t pageyes = 0;

                uint8_t op = 0;

                if (strncmp((char *) cmd, "sr", 2) == 0) { 
                    op = 1;
                }
                else if (strncmp((char *) cmd, "sw", 2) == 0) { 
                    op = 2;
                }
                else if (strncmp((char *) cmd, "se", 2) == 0) { 
                    op = 3;
                }

             
                while( 1 ) {


                    if (usb_read_ready()) {
                        usb_read(cmd, 4); // read in block

                        if (!blockyes) {
                            block = atoi(cmd);
                            uint8_t blockout[2];
                            blockout[0] = (uint8_t) (block>>8);
                            blockout[1] = (uint8_t) (block);
                            usb_write(blockout,2);

                            blockyes = 1;
                        }
                        else if (!pageyes) {
                            page = atoi(cmd);
                            usb_write(&page, 1);
                            pageyes = 1;
                        }

                    }

                    if (blockyes && pageyes) {
                        usb_write((uint8_t *) "BPRD", 4); // block and page are successfully read from input
                        break; // exit while loop
                    }

                }

                address = address0 | (((uint32_t) block ) << 22 ) | (((uint32_t) page) << 14 );

                if (op == 1) {
                    usb_write((uint8_t *) "READ", 4);
                }
                // write 0x00 ?
                else if (op == 2) {
                    usb_write((uint8_t *) "WRIT", 4);
                    memset(write_buffer, 0x00, Nbyte);
                    complete_write(address, Nbyte, write_buffer, otimep);
                    usb_write(otimep, 4);
                }
                // erase?
                else if (op == 3) {
                    usb_write((uint8_t *) "ERAS", 4);
                    complete_erase(address, otimee);
                    usb_write(otimee, 4);
                }

                // read the block/page given
                read(address, Nbyte, read_buffer);
                usb_write(read_buffer, Nbytebase);
                usb_write(readbufNbytebaseptr, Nbytespare);
                usb_write((uint8_t *) "DONEDONEDONE", 12);


            } // end if sr (special read) / write / erase
            
			
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
/*	if (count > Nbyte)
		count = Nbyte;*/

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
		WRITE_WITH_FLOP((uint8_t)((address & 0x00003f00) >> 8));
	}
	
	// Next, load in the next eight bits (12-19)
	WRITE_WITH_FLOP((uint8_t)((address & 0x003fc000) >> 14));

	// Next eight bits (20-27)
	WRITE_WITH_FLOP((uint8_t)((address & 0x3fc00000) >> 22));

	// Last few remaining bits, which do not extend to a full 32 bits (only 31 for 8 Gb, and only 30 for 4 Gb!)
	WRITE_WITH_FLOP((uint8_t)((address & 0xc0000000) >> 30));

	// De-assert the address latch now, just in case
	FIO0CLR = P_ADDR_LATCH;
}
