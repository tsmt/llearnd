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

float mpu6050GetAx() {
    short val;
    high = wiringPiI2CReadReg8(fd, MPU6050_AX);
    low = wiringPiI2CReadReg8(fd, MPU6050_AX+1);
    val = (high << 8 | low);
    return MPU6050_ACC_SCALE(val);
}

float mpu6050GetAy() {
    short val;
    high = wiringPiI2CReadReg8(fd, MPU6050_AY);
    low = wiringPiI2CReadReg8(fd, MPU6050_AY+1);
    val = (high << 8 | low);
    return MPU6050_ACC_SCALE(val);
}

float mpu6050GetAz() {
    short val;
    high = wiringPiI2CReadReg8(fd, MPU6050_AZ);
    low = wiringPiI2CReadReg8(fd, MPU6050_AZ+1);
    val = (high << 8 | low);
    return MPU6050_ACC_SCALE(val);
}

float mpu6050GetGx() {
    short val;
    high = wiringPiI2CReadReg8(fd, MPU6050_GX);
    low = wiringPiI2CReadReg8(fd, MPU6050_GX+1);
    val = (high << 8 | low);
    return MPU6050_GYRO_SCALE(val);
}

float mpu6050GetGy() {
    short val;
    high = wiringPiI2CReadReg8(fd, MPU6050_GY);
    low = wiringPiI2CReadReg8(fd, MPU6050_GY+1);
    val = (high << 8 | low);
    return MPU6050_GYRO_SCALE(val);
}

float mpu6050GetGz() {
    short val;
    high = wiringPiI2CReadReg8(fd, MPU6050_GZ);
    low = wiringPiI2CReadReg8(fd, MPU6050_GZ+1);
    val = (high << 8 | low);
    return MPU6050_GYRO_SCALE(val);
}

float mpu6050GetTmp() {
    short val;
    high = wiringPiI2CReadReg8(fd, MPU6050_TMP);
    low = wiringPiI2CReadReg8(fd, MPU6050_TMP+1);
    val = (high << 8 | low);
    return MPU6050_TEMP_SCALE(val);
}
