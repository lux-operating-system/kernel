ASFLAGS=-f elf64
CCFLAGS=-c -I./src/include -ffreestanding -O2
LDFLAGS=-T./src/platform/x86_64/lux-x86_64.ld -nostdlib
AS=nasm
CC=x86_64-elf-gcc
LD=x86_64-elf-ld
SRCC:=$(shell find ./src -type f -name "*.c")
OBJC:=$(SRCC:.c=.o)
SRCA:=$(shell find ./src -type f -name "*.asm")
OBJA:=$(SRCA:.asm=.o)

all: lux

%.o: %.c
	@echo "\x1B[0;1;32m [  CC   ]\x1B[0m $<"
	@$(CC) $(CCFLAGS) -o $@ $<

%.o: %.asm
	@echo "\x1B[0;1;32m [  AS   ]\x1B[0m $<"
	@$(AS) $(ASFLAGS) -o $@ $<

lux: $(OBJC) $(OBJA) 
	@echo "\x1B[0;1;34m [  LD   ]\x1B[0m lux"
	@$(LD) $(LDFLAGS) $(OBJA) $(OBJC) -o lux

clean:
	@rm -f lux $(OBJC) $(OBJA)
