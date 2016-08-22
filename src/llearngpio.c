#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wiringPi.h>
#include <mcp3004.h>
#include <errno.h>
#include "llearngpio.h"
#include "mpu6050.h"
#include "sht21.h"


float currentValues[SENSORCOUNT];
Device devices[SENSORCOUNT] = {
    { &currentValues[0], "/tmp/llearn/a0", "w+" },
    { &currentValues[1], "/tmp/llearn/a1", "w+" },
    { &currentValues[2], "/tmp/llearn/a2", "w+" },
    { &currentValues[3], "/tmp/llearn/a3", "w+" },
    { &currentValues[4], "/tmp/llearn/a4", "w+" },
    { &currentValues[5], "/tmp/llearn/a5", "w+" },
    { &currentValues[6], "/tmp/llearn/a6", "w+" },
    { &currentValues[7], "/tmp/llearn/a7", "w+" },
    { &currentValues[8], "/tmp/llearn/a8", "w+" },
    { &currentValues[9], "/tmp/llearn/a9", "w+" },
    { &currentValues[10], "/tmp/llearn/hum", "w+" },
    { &currentValues[11], "/tmp/llearn/tmp_sht", "w+" },
    { &currentValues[12], "/tmp/llearn/tmp_mpu", "w+" },
    { &currentValues[13], "/tmp/llearn/acc_x", "w+" },
    { &currentValues[14], "/tmp/llearn/acc_y", "w+" },
    { &currentValues[15], "/tmp/llearn/acc_z", "w+" },
    { &currentValues[16], "/tmp/llearn/gyr_x", "w+" },
    { &currentValues[17], "/tmp/llearn/gyr_y", "w+" },
    { &currentValues[18], "/tmp/llearn/gyr_z", "w+" }
};

const char devPath[] = "/tmp/llearn";

int mkdir_p(const char *path);
int error(int num, char msg[]);

int wrDevFile(Device *dev);
void collectSensorData(void);

void s0_impulse(void) {
    static unsigned int debunk;
    if(millis() > debunk + TIMEPERIOD_MS_DEBUNK) {
        printf("s0 int\n");
        debunk = millis();
    }
}

int main(int argc, char* argv[]) {
    int i, r;
    time_t current = 0;
    static time_t last_collection = 0, last_print = 0;

    mkdir_p(devPath);

    if((r = wiringPiSetup()) < 0 ) {
        error(r, "wiringPi Setup");
    }
    if((r = mcp3004Setup(MCP1_START, 0)) < 0 ) {
        error(r, "mcp3208-1 Setup");
    }
    if((r = mcp3004Setup(MCP2_START, 1)) < 0 ) {
        error(r, "mcp3208-2 Setup");
    }
    if((r = mpu6050Setup()) < 0) {
        error(r, "mpu6050 Setup");
    }
    if((r = sht21Setup()) < 0) {
        error(r, "sht21 Setup");
    }
    if(wiringPiISR(0, INT_EDGE_RISING, &s0_impulse) < 0) {
        error(r, "wiringPiISR");
    }

    while(1) {
        time(&current); /* error handling */
        if(current > (last_collection + TIMEPERIOD_COLLECT)) {
            collectSensorData();
            time(&last_collection); /* eh */
        }
        if(current > (last_print + TIMEPERIOD_PRINT)) {
            for(i = 0; i < SENSORCOUNT; i++)
                wrDevFile(&devices[i]);
            time(&last_print); /* eh */
        }
        usleep(500);
    }
    return 0;
}


int error(int num, char msg[]) {
    printf("[%d] llearngpio fatal: %d - %s", errno, num, msg);
    exit(num);
}

int wrDevFile(Device *dev) {
    FILE *fp;
    fp = fopen(dev->fname, dev->mode);
    if(fp == NULL)
        error((int)fp, "file open");
    fprintf(fp, "%f\n", *(dev->value));
    fclose(fp);
    return 0;
}

void collectSensorData() {
    int i;
    for(i = 0; i < ANALOG_SENSORS; i++) {
        currentValues[i] = (analogRead(MCP1_START + i) / 1023.0);
    }
    currentValues[ANALOG_SENSORS] = sht21GetHum();
    currentValues[ANALOG_SENSORS+1] = sht21GetTemp();
    currentValues[ANALOG_SENSORS+2] = mpu6050GetTmp();
    currentValues[ANALOG_SENSORS+3] = mpu6050GetAx();
    currentValues[ANALOG_SENSORS+4] = mpu6050GetAy();
    currentValues[ANALOG_SENSORS+5] = mpu6050GetAz();
    currentValues[ANALOG_SENSORS+6] = mpu6050GetGx();
    currentValues[ANALOG_SENSORS+7] = mpu6050GetGy();
    currentValues[ANALOG_SENSORS+8] = mpu6050GetGz();
}

/* 
recursive mkdir from 
https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950 
*/

int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p; 

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1; 
    }   
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return -1; 
            }

            *p = '/';
        }
    }   

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
            return -1; 
    }   

    return 0;
}

