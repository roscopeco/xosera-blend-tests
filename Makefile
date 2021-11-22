# Make rosco_m68k example programs
#
# Copyright (c) 2020 Xark
# MIT LICENSE

ifndef ROSCO_M68K_DIR
$(error Please set ROSCO_M68K_DIR to the top-level rosco_m68k directory to use for rosco_m68k building)
endif

CPU?=68010
EXTRA_CFLAGS?=-g -O2 -fomit-frame-pointer
SYSINCDIR?=$(ROSCO_M68K_DIR)/code/software/libs/build/include
SYSLIBDIR?=$(ROSCO_M68K_DIR)/code/software/libs/build/lib
DEFINES=-DROSCO_M68K
CFLAGS=-std=c11 -ffreestanding -ffunction-sections -fdata-sections \
				-Wall -Wextra -Werror -pedantic -Wno-unused-function -I$(SYSINCDIR) \
				-mcpu=$(CPU) -march=$(CPU) -mtune=$(CPU) $(DEFINES)
GCC_LIBS=$(shell $(CC) --print-search-dirs 															\
		| grep libraries:\ =																								\
    | sed 's/libraries: =/-L/g' 																				\
    | sed 's/:/m68000\/ -L/g')m68000/
LIBS=-lsdfat -lprintf -lcstdlib -lmachine -lstart_serial -lgcc
ASFLAGS=-mcpu=$(CPU) -march=$(CPU)
LDFLAGS=-T $(SYSLIBDIR)/ld/serial/rosco_m68k_program.ld -L $(SYSLIBDIR) \
				-Map=$(MAP) --gc-sections --oformat=elf32-m68k
VASMFLAGS=-Felf -m$(CPU) -quiet -Lnf $(DEFINES)
CC=m68k-elf-gcc
AS=m68k-elf-as
LD=m68k-elf-ld
NM=m68k-elf-nm
LD=m68k-elf-ld
OBJDUMP=m68k-elf-objdump
OBJCOPY=m68k-elf-objcopy
SIZE=m68k-elf-size
VASM=vasmm68k_mot
RM=rm -f
KERMIT=kermit
SERIAL?=/dev/modem
BAUD?=9600

# Output config (assume name of directory)
PROGRAM_BASENAME=$(shell basename $(CURDIR))

# Set other output files using output basname
ELF=$(PROGRAM_BASENAME).elf
BINARY=$(PROGRAM_BASENAME).bin
DISASM=$(PROGRAM_BASENAME).dis
MAP=$(PROGRAM_BASENAME).map
SYM=$(PROGRAM_BASENAME).sym

# Assume source files in Makefile directory are source files for project
CSOURCES=$(wildcard *.c)
SSOURCES=$(wildcard *.S)
ASMSOURCES=$(wildcard *.asm)
SOURCES=$(CSOURCES) $(SSOURCES) $(ASMSOURCES)

# Assume each source files makes an object file
OBJECTS=$(addsuffix .o,$(basename $(SOURCES)))

all: $(BINARY) $(DISASM)

$(ELF) : $(OBJECTS)
	$(LD) $(LDFLAGS) $(GCC_LIBS) $^ -o $@ $(LIBS)
	$(NM) --numeric-sort $@ >$(SYM)
	$(SIZE) $@
	-chmod a-x $@

$(BINARY) : $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(BINARY)

$(DISASM) : $(ELF)
	$(OBJDUMP) --disassemble -S $(ELF) >$(DISASM)

$(OBJECTS): Makefile

%.o : %.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $<

%.o : %.asm
	$(VASM) $(VASMFLAGS) $(EXTRA_VASMFLAGS) -L $(basename $@).lst -o $@ $<

# remove targets that can be generated by this Makefile
clean:
	$(RM) $(OBJECTS) $(ELF) $(BINARY) $(MAP) $(SYM) $(DISASM) $(addsuffix .lst,$(basename $(SSOURCES) $(ASMSOURCES)))

disasm: $(DISASM)

# hexdump of program binary
dump: $(BINARY)
	hexdump -C $(BINARY)

# upload binary to rosco (if ready and kermit present)
load: $(BINARY)
	kermit -i -l $(SERIAL) -b $(BAUD) -s $(BINARY)

# This is handy to test on Ubuntu
test: $(BINARY) $(DISASM)
	-killall screen && sleep 1
	kermit -i -l $(SERIAL) -b $(BAUD) -s $(BINARY)
	gnome-terminal --geometry=80x25 --title="rosco_m68k $(SERIAL)" -- screen $(SERIAL) $(BAUD)

# This is handy to test on MacOS
mactest: $(BINARY) $(DISASM)
	-killall screen && sleep 1
	kermit -i -l $(SERIAL) -b $(BAUD) -s $(BINARY)
	echo "/usr/bin/screen $(SERIAL) $(BAUD)" > $(TMPDIR)/rosco_screen.sh
	-chmod +x $(TMPDIR)/rosco_screen.sh
	open -b com.apple.terminal $(TMPDIR)/rosco_screen.sh

# Makefile magic (for "phony" targets that are not real files)
.PHONY: all clean dump disasm load test mactest