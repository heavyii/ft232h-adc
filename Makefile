

CC ?= gcc
INSTALL = /usr/bin/install
PREFIX ?= /usr/local

INCLUDES = -I. 
CFLAGS = -O2 -Wall -Wno-char-subscripts -g
LDFLAGS = -lftdi -lm

BINARY = ft232h-adc

all: $(BINARY)

.SUFFIXES: .c .o

OBJ=\
	main.o \
	mpsse.o \
	ftdi_io_libftdi.o \
	ltc1407a.o 
    

$(BINARY): $(OBJ)  
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDFLAGS)

.c.o:
	@$(CC) $(CFLAGS) -o $@ -c $*.c


clean:
	@rm -f *.o
	@rm -f $(BINARY)
	@rm -rf test-data
