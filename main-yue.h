#ifndef _MAIN_H_
#define _MAIN_H_
/**
 * Definitions of things used to program flash memory
 */

// Standard headers
#include <LPC214x.h>
#include <inttypes.h>

// Definitions
// None

// Control pin constants, Port0, for use with FIOSET0
#define P_CHIP_ENABLE		(1 << 12)				// /E
#define P_ADDR_LATCH		(1 << 13)				// AL
#define P_CMD_LATCH			(1 << 8)				// CL
#define P_READ_ENABLE		(1 << 10)				// /R
#define P_WRITE_ENABLE		(1 << 4)				// /W
#define P_WRITE_PROTECT		(1 << 5)				// WP
#define P_BUSY				(1 << 6)				// R/B

// Define generic helper macros
#define WAIT asm("nop")
#define WAIT_FOR_BUSY while ((FIO0PIN & P_BUSY) != P_BUSY) { WAIT; }

// Command words
#define CMD_READ_1						0x00
#define CMD_READ_2						0x30
#define CMD_RANDOM_DATA_OUTPUT_1		0x05
#define CMD_RANDOM_DATA_OUTPUT_2		0xE0
#define CMD_EXIT_CACHE_READ				0x3F
#define CMD_CACHE_READ_1				0x00
#define CMD_CACHE_READ_2				0x31
#define CMD_PAGE_PROGRAM_1				0x80
#define CMD_PAGE_PROGRAM_2				0x10
#define CMD_RANDOM_DATA_INPUT			0x85
#define CMD_READ_STATUS					0x70
#define CMD_READ_SIGNATURE				0x90
#define CMD_BLOCK_ERASE_1				0x60
#define CMD_BLOCK_ERASE_2				0xD0

// Status register bits
#define SR_WRITE_PROTECT			(1 << 7)
#define SR_CACHE_READY				(1 << 6)
#define SR_PER_READY				(1 << 5)
#define SR4							(1 << 4)
#define SR3							(1 << 3)
#define SR2							(1 << 2)
#define SR_CACHE_PROGRAM_ERROR		(1 << 1)
#define SR_GENERIC_ERROR			(1 << 0)

// Address/data pins are Port0.16-Port0.123, so they should be used with
// FIOSET2, FIOCLR2, and FIOPIN2
#define WRITE_VALUE(x) (FIO0PIN2 = (x))
#define READ_DATA(x) ((x) = FIO0PIN2)

// Some macros for ease of manipulation
#define SET_IO_AS_OUTPUT (FIO0DIR2 = 0xff)
#define SET_IO_AS_INPUT (FIO0DIR2 = 0x00)

// Chip Enable macros
#define ASSERT_CHIP_ENABLE FIO0CLR = P_CHIP_ENABLE; WAIT; WAIT
#define DEASSERT_CHIP_ENABLE FIO0SET = P_CHIP_ENABLE

// Make it quicker to complete the process of writing a value to the device by clearing /W, setting the pins, and raising /W
#define WRITE_WITH_FLOP(x) FIO0CLR = P_WRITE_ENABLE;  WRITE_VALUE(x); FIO0SET = P_WRITE_ENABLE; //there are two waits at each position before

// Function prototypes
void ini_poweron();
void insert_delay(uint32_t Nprescaler);
void init(void);
void read(uint32_t address, uint32_t count, uint8_t *dest);
uint8_t read_status(void);
void read_electronic_signature(uint8_t *dest);
void readID(uint8_t *dest)
uint8_t partial_write(uint32_t address, uint32_t count, const uint8_t *src);
uint8_t complete_write(uint32_t address, uint32_t count, const uint8_t *src, uint8_t *otime1);
void write_cmd_word(uint8_t cmd);
void write_address(uint32_t address, uint8_t doErase);
void reset_io(void);
uint8_t partial_erase(uint32_t address);
uint8_t complete_erase(uint32_t address, uint8_t *otime1);
void cache_read(uint32_t address, uint32_t count, uint8_t *dest, uint32_t ntimes);
#endif
