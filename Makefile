# makefile from wiringPi example projects

#DEBUG	= -g -O0
DEBUG	= -O3
CC	= gcc
INCLUDE	= -I/usr/local/include
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS	= -L/usr/local/lib
LDLIBS    = -lwiringPi -lwiringPiDev -lpthread -lm

TARGET=llearngpio
SRC= src/llearngpio.c src/mpu6050.c
OBJ= $(SRC:.c=.o)

llearngpio: $(OBJ)
	@echo link $@
	@$(CC) -o $(TARGET) $(OBJ) $(LDFLAGS) $(LDLIBS)

.c.o:
	@echo compile $<
	@$(CC) -c $(CFLAGS) $< -o $@

clean:
	@rm $(TARGET)
	@rm -f $(OBJ)
