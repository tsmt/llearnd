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
#include "llearnd.h"
#include "mpu6050.h"
#include "sht21.h"

/* Global values */
float currentValues[SENSORCOUNT];
const char devPath[] = "/tmp/llearn";
unsigned int s0;
unsigned int stmState;

void s0_impulse(void) {
    s0++;        //delay(50);
}

int main(int argc, char* argv[]) {
    int r;

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
    if(wiringPiISR(0, INT_EDGE_FALLING, &s0_impulse) < 0) {
        error(r, "wiringPiISR");
    }

    return stmRun();
}

int stmRun() {
    /* init statmachine values */
    int r = 0;
    /* check for machine state */
    r = collectLedData();
    switch(r) {
        case M_STATE_ON_RUNNING:
            /* TODO: boot while maching running. what to do??? */
            stmState = STM_STATE_RUNNING;
            printf("stm: running\n");
            break;
        case M_STATE_OFF:
            stmState = STM_STATE_WAIT;
            printf("stm: waiting\n");
            break;
        case M_STATE_ON:
            stmState = STM_STATE_WAIT;
            printf("stm: waiting\n");
            break;
        default:
            error(1, "default machine state forbidden!");
    }
    /* loop machine */
    r = 0;
    while (r == 0) {
        switch(stmState) {
            case STM_STATE_WAIT:
                r = stmWait();
                break;
            case STM_STATE_RUNNING:
                r = stmRunning();
                break;
            case STM_STATE_POSTPROCESS:
                r = stmPostProcess();
                break;
        }
        /* wait for machine frequency (throttle CPU usage)*/
        usleep(STM_FREQUENCY_MS);
    }
    error(r, "stmRun");
    return 0;
}

int stmWait() {
    int m = 0;
    /* get sensor info */
    m = collectLedData();
    collectSensorData();
    /* check for state change */
    if(m == M_STATE_ON_RUNNING) {
        stmState = STM_STATE_RUNNING;
        /* do preparation stuff */
        printf("stm: running\n");
        return 0;
    }
    /* update device info to MQTT every X seconds */
    return 0;
}

int stmRunning() {
    int m = 0;
    /* get sensor info */
    m = collectLedData();
    collectSensorData();
    /* check for state change */
    if(m != M_STATE_ON_RUNNING) {
        stmState = STM_STATE_POSTPROCESS;
        /* do preparation stuff */
        printf("stm: postprocess\n");
        return 0;
    }
    /* update device info to MQTT every X seconds */
    /* update machine info to MQTT evry X seconds */
    /* write LogFile every X seconds*/
    return 0;
}

int stmPostProcess() {
    printf("postprocessing...\n");
    stmState = STM_STATE_WAIT;
    printf("stm: waiting...\n");
    return 0;
}

int error(int num, char msg[]) {
    printf("[%d] llearngpio fatal: %d - %s\n", errno, num, msg);
    exit(num);
}

int wrLog() {
    int i;
    FILE *fp;
    fp = fopen("/tmp/llearn/log", "a+");
    if(fp == NULL)
        error((int)fp, "file open");
    fprintf(fp, "%lld,", (long long) time(NULL));
    for(i = 0; i < SENSORCOUNT-1; i++) {
        fprintf(fp, "%f,", currentValues[i]);
    }
    fprintf(fp,"%f\n", currentValues[SENSORCOUNT-1]);
    fclose(fp);
    return 0;
}

int collectLedData() {
    int i;
    /* read sensors */
    for(i = 0; i < ANALOG_SENSORS; i++) {
        currentValues[i] = (analogRead(MCP1_START + i) / 1023.0);
    }
    if(LED_ISON(currentValues[LED_WASCHEN])) {
        return M_STATE_ON_RUNNING;
    }
    return M_STATE_OFF;
}


void collectSensorData() {
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
