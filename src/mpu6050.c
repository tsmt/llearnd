#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "mpu6050.h"

static int fd;
char high, low;

int mpu6050Setup() {
    fd = wiringPiI2CSetup(MPU6050_ADDRESS);
    if(fd < 0) {
        return fd; /* error on create */
    }
    wiringPiI2CWriteReg8(fd, MPU6050_PWRMNGR_1, 0);
    /* give him time to process */
    delay(50);
    return fd;
}

short mpu6050GetAx() {
    high = wiringPiI2CReadReg8(fd, MPU6050_AX);
    low = wiringPiI2CReadReg8(fd, MPU6050_AX+1);
    return (high << 8 | low);
}

short mpu6050GetAy() {
    high = wiringPiI2CReadReg8(fd, MPU6050_AY);
    low = wiringPiI2CReadReg8(fd, MPU6050_AY+1);
    return (high << 8 | low);
}

short mpu6050GetAz() {
    high = wiringPiI2CReadReg8(fd, MPU6050_AZ);
    low = wiringPiI2CReadReg8(fd, MPU6050_AZ+1);
    return (high << 8 | low);
}

short mpu6050GetGx() {
    high = wiringPiI2CReadReg8(fd, MPU6050_GX);
    low = wiringPiI2CReadReg8(fd, MPU6050_GX+1);
    return (high << 8 | low);
}

short mpu6050GetGy() {
    high = wiringPiI2CReadReg8(fd, MPU6050_GY);
    low = wiringPiI2CReadReg8(fd, MPU6050_GY+1);
    return (high << 8 | low);
}

short mpu6050GetGz() {
    high = wiringPiI2CReadReg8(fd, MPU6050_GZ);
    low = wiringPiI2CReadReg8(fd, MPU6050_GZ+1);
    return (high << 8 | low);
}

short mpu6050GetTmp() {
    high = wiringPiI2CReadReg8(fd, MPU6050_TMP);
    low = wiringPiI2CReadReg8(fd, MPU6050_TMP+1);
    return (high << 8 | low);
}
