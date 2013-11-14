

CC ?= gcc
INSTALL = /usr/bin/install
PREFIX ?= /usr/local

INCLUDES = -I. 
CFLAGS = -O0 -Wall -Wno-char-subscripts -g
LDFLAGS = -lftdi -lm

BINARY = usb-spi

all: $(BINARY)

.SUFFIXES: .c .o

OBJ=\
	main.o \
	buffer.o \
	mpsse.o \
	ftdi_io_libftdi.o 
    

$(BINARY): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $*.c


clean:
	@rm -f $(OBJ)
	@rm -f $(BINARY)
