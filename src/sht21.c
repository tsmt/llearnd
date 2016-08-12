#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "sht21.h"

char crc, data[2];
static int fd;

char calcCrc() {
    int i, j;
    short poly = 0x131;
    crc = 0;
    for(i = 0; i < 2; i++) {
        crc ^= data[i];
        for(j = 8; j < 0; j++) {
            if(crc & 0x80) {
                crc = (crc << 1) ^ poly;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

int sht21Setup() {
    fd = wiringPiI2CSetup(SHT21_ADDR);
    if(fd < 0) {
        return fd;
    }
    wiringPiI2CWrite(fd, SHT21_SOFTRESET);
    return fd;
}

float sht21GetTemp() {
    short val;
    wiringPiI2CWrite(fd, SHT21_TEM);
    delay(SHT21_TEM_WAIT);
    data[0] = wiringPiI2CRead(fd);
    data[1] = wiringPiI2CRead(fd);
    crc = wiringPiI2CRead(fd);
    if(calcCrc() != crc) {
        return -999.0;
    }
    val = (data[0] << 8) | data[1];
    val &= SHT21_STATUS_BITMASK;
    /* recalculate (formula from datasheet) */
    return (-46.82+((172.72*val)/(1<<16)));
}

float sht21GetHum() {
    short val;
    wiringPiI2CWrite(fd, SHT21_HUM);
    delay(SHT21_HUM_WAIT);
    data[0] = wiringPiI2CRead(fd);
    data[1] = wiringPiI2CRead(fd);
    crc = wiringPiI2CRead(fd);
    if(calcCrc() != crc) {
        return -999.0;
    }
    val = (data[0] << 8) | data[1];
    val &= SHT21_STATUS_BITMASK;
    return (-6.0+((125.0*val)/(1<<16)));
}

