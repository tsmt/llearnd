# makefile from wiringPi example projects

#DEBUG	= -g -O0
DEBUG	= -O3
CC	= gcc
INCLUDE	= -I/usr/local/include
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS	= -L/usr/local/lib
LDLIBS    = -lwiringPi -lwiringPiDev -lpthread -lm

SRC= llearngpio.c
OBJ= $(SRC:.c=.o)
BINS= $(SRC:.c=)

all: $(BINS)

llearngpio: src/llearngpio.o
	@echo link $@
	@$(CC) -o $@ src/$@.o $(LDFLAGS) $(LDLIBS)

.c.o:
	@echo compile $<
	@$(CC) -c $(CFLAGS) $< -o $@

clean:
	@rm $(BINS)
	@cd src
	@rm -f $(OBJ)
