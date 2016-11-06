# makefile from wiringPi example projects

#DEBUG	= -g -O0
DEBUG	= -O3
CC	= gcc
INCLUDE	= -I/usr/local/include -Iinclude
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS	= -L/usr/local/lib -Llib
LDLIBS    = -lwiringPi -lwiringPiDev -lpthread -lm -lpaho-mqtt3c

TARGET=llearnd
SRC= src/llearnd.c src/mpu6050.c src/sht21.c
OBJ= $(SRC:.c=.o)

llearnd: $(OBJ)
	@echo link $@
	@$(CC) -o $(TARGET) $(OBJ) $(LDFLAGS) $(LDLIBS)

.c.o:
	@echo compile $<
	@$(CC) -c $(CFLAGS) $< -o $@

clean:
	@rm $(TARGET)
	@rm -f $(OBJ)

install:
	./install.sh
