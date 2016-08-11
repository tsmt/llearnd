#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <mcp3004.h>
#include "mpu6050.h"

int error(int num, char msg[]) {
    printf("llearngpio fatal: %d - %s", num, msg);
    exit(-1);
}

int main(int argc, char* argv[]) {
    int i, mpu;
    wiringPiSetup();
    mcp3004Setup(100, 0);
    mcp3004Setup(110, 1);
    if((mpu = mpu6050Setup()) < 0) {
        error(mpu, "mpu6050 Setup");
    }
    while(1) {
        printf("mpu - %d %d %d\n", mpu6050GetAx(mpu),
                mpu6050GetAy(mpu), mpu6050GetAz(mpu));
        for(i = 0; i < 8; i++) {
            printf("%d[%d/%d]", i, analogRead(100+i), analogRead(110+i));
        }
        printf("\n-----------------------\n");
        delay(1000);
    }
    return 0;
}
