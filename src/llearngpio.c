#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <mcp3004.h>
#include "mpu6050.h"
#include "sht21.h"

int error(int num, char msg[]) {
    printf("llearngpio fatal: %d - %s", num, msg);
    exit(-1);
}

int main(int argc, char* argv[]) {
    int i, r;
    wiringPiSetup();
    mcp3004Setup(100, 0);
    mcp3004Setup(110, 1);
    if((r = mpu6050Setup()) < 0) {
        error(r, "mpu6050 Setup");
    }
    if((r = sht21Setup()) < 0) {
        error(r, "sht21 Setup");
    }
    while(1) {
        printf("mpu - %d %d %d %d %d %d %d\n", mpu6050GetAx(),
                mpu6050GetAy(), mpu6050GetAz(),
                mpu6050GetGx(), mpu6050GetGy(),
                mpu6050GetGz(), mpu6050GetTmp());
        printf("sht21 - %f %f\n", sht21GetTemp(), sht21GetHum());
        for(i = 0; i < 8; i++) {
            printf("%d[%d/%d]", i, analogRead(100+i), analogRead(110+i));
        }
        printf("\n-----------------------\n");
        delay(1000);
    }
    return 0;
}
