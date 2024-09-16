PLATFORM=x86_64
PLATFORMDEF=PLATFORM_X86_64
ASFLAGS=-f elf64 -i./src/platform/$(PLATFORM)
CCFLAGS=-Wall -c -D$(PLATFORMDEF) -I./src/include -I./src/platform/$(PLATFORM)/include -mno-sse -ffreestanding -O3 -mcmodel=large -mno-red-zone
LDFLAGS=-T./src/platform/$(PLATFORM)/lux-$(PLATFORM).ld -nostdlib
AS=nasm
CC=x86_64-lux-gcc
LD=x86_64-lux-ld
SRCC:=$(shell find ./src -type f -name "*.c")
OBJC:=$(SRCC:.c=.o)
SRCA:=$(shell find ./src -type f -name "*.asm")
OBJA:=$(SRCA:.asm=.o)

all: lux

%.o: %.asm
	@echo "\x1B[0;1;32m as  \x1B[0m $<"
	@$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	@echo "\x1B[0;1;32m cc  \x1B[0m $<"
	@$(CC) $(CCFLAGS) -o $@ $<

lux: $(OBJA) $(OBJC) 
	@echo "\x1B[0;1;34m ld  \x1B[0m lux"
	@$(LD) $(LDFLAGS) $(OBJA) $(OBJC) -o lux

clean:
	@rm -f lux $(OBJC) $(OBJA)
