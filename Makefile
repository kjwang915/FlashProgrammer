# Custom makefile for building ARM programs. Hopefully stripped down a bit
# from the ones that come with WinARM, but without breaking things
# Also, targets Yagarto hopefully

# Source files
# TODO: Clean up the hack to get the core common library files
ARMELFPATH=C:/WinARM/arm-elf/src/

SRC=main.c usbcore.c usbdesc.c usbhw.c usbuser.c usbcomms.c
SRC+=$(ARMELFPATH)irq.c

ASRC=Startup.S $(ARMELFPATH)swi_handler.S

OBJS=main.o usbcore.o usbdesc.o usbhw.o usbuser.o usbcomms.o
OBJS+=irq.o Startup.o swi_handler.o

TARGET=usbtest

# Parameters for flash utility
COMPORT=com1
BAUDRATE=38400
FOSC=12000

# Model information
MCPU=arm7tdmi-s
SUBMDL=LPC2148
THUMB=-mthumb
THUMB_IW=-mthumb-interwork

# Output format for the compiled image
FORMAT=ihex

# Paths to toolchain-provided files
INCDIRS=-I. -I/c/WinARM/arm-elf/include
LIBDIRS=-L/c/WinARM/arm-elf/lib
# When we make the shared object, its path will go there and it'll be a compiler switch

# Definied values and stuff
CDEFS=-D__WinARM__
ADEFS=-D__WinARM__ -DROM_RUN

# Flags for C compiler
CFLAGS=$(CDEFS) $(INCDIRS) $(LIBDIRS) -Os -Wall -Wcast-align -Wimplicit
CFLAGS+=-Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type
CFLAGS+=-Wshadow -Wunused
CFLAGS+=-mcpu=$(MCPU) 

# Flags for assembler
AFLAGS=$(ADEFS) $(INCDIRS)
AFLAGS+=-mcpu=$(MCPU)

# Linker flags
LDFLAGS=-nostartfiles -Wl,-Map=$(TARGET).map,--cref
LDFLAGS+=-lc -lm -lgcc
LDFLAGS+=$(LIBDIRS)
# For now, only support the ROM-mode. (See example makefile for RAM script)
LDFLAGS+=-T$(SUBMDL)-ROM.ld

# Workhorse programs
MAKEPATH = C:/WinARM/bin/
WBIN = C:/Program Files (x86)/GnuWin32/usr/local/wbin/

SHELL=sh
CC=$(MAKEPATH)arm-elf-gcc
CPP=$(MAKEPATH)arm-elf-g++
OBJCOPY=$(MAKEPATH)arm-elf-objcopy
OBJDUMP=$(MAKEPATH)arm-elf-objdump
SIZE=$(MAKEPATH)arm-elf-size
NM=$(MAKEPATH)arm-elf-nm
RM=$(WBIN)rm -f
COPY=$(WBIN)cp
LPCISP=$(MAKEPATH)lpc21isp_old

# Now the real make targets
hex: image
image: elf
	$(OBJCOPY) -O $(FORMAT) $(TARGET).elf $(TARGET).hex

elf: objects
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJS) --output $(TARGET).elf

objects:
	$(CC) $(AFLAGS) -c $(ASRC)
	$(CC) $(CFLAGS) -c $(SRC)

clean:
	$(RM) *.o $(TARGET).elf $(TARGET).hex $(TARGET).map

flash:image
	$(LPCISP) -control $(TARGET).hex $(COMPORT) $(BAUDRATE) $(FOSC)

all: image
