#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "sht21.h"

char high, low;
static int fd;

int sht21Setup() {
    fd = wiringPiI2CSetup(SHT21_ADDR);
    if(fd < 0) {
        return fd;
    }
    wiringPiI2CWrite(fd, SHT21_SOFTRESET);
    return fd;
}

int sht21GetTemp() {
    int val; 
    wiringPiI2CWrite(fd, SHT21_TEM);
    delay(SHT21_TEM_WAIT);
    val = wiringPiI2CRead(fd);
    return val;
}

int sht21GetHum() {
    int val;
    wiringPiI2CWrite(fd, SHT21_HUM);
    delay(SHT21_HUM_WAIT);
    val = wiringPiI2CRead(fd);
    return val;
}

