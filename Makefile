

CC ?= gcc
INSTALL = /usr/bin/install
PREFIX ?= /usr/local

INCLUDES = -I. 
CFLAGS = -O0 -Wall -Wno-char-subscripts 
LDFLAGS = -lftdi1 -lpthread -lm

BINARY = ft232h-adc

all: $(BINARY)

.SUFFIXES: .c .o

OBJ=\
	main.o \
	mpsse.o \
	ftdi_io_ftdi1.o \
	ltc1407a.o 
    

$(BINARY): $(OBJ)  
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $*.c


clean:
	@rm -f *.o
	@rm -f $(BINARY)
	@rm -rf test-data
