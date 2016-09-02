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
#include "config.h"
#include "../include/MQTTClient.h"

/* Global values */

/* device Values TODO: array docu */
float currentValues[SENSORCOUNT];

const char defLogPath[] = "/tmp/llearn";
char *logPath = defLogPath;
char logfile[256];
unsigned int s0 = 0;
unsigned int stmState = STM_STATE_INIT;
unsigned int mState;

time_t currentTime;
time_t lastDevMqttUpdate;
time_t lastLog;

/* CLI Options */
unsigned int cli_daemon = 0;


/* MQTT vars */
MQTTClient mqttc;
MQTTClient_connectOptions mqttc_conopt = MQTTClient_connectOptions_initializer;

/**
 * TODO:
    - daemon mode (fork)
    - accept CLI input
    - signal handling
*/
int main(int argc, char* argv[]) {
    int r;
    /* handle CLI arguments */
    while((r = getopt(argc, argv, "dp:Vh?")) != -1) {
        switch(r) {
            case 'd':
                cli_daemon = 1;
                break;
            case 'p':
                logPath = optarg;
                break;
            case 'V':
                printf("llearnd Version %s. Built %s %s\n", VERSION,
                                            __DATE__, __TIME__);
                exit(0);
                break;
            case 'h': case '?':
                printf("llearnd Version %s. Built %s %s\n", VERSION,
                                            __DATE__, __TIME__);
                printf("  -d\tdaemon\t\tDaemon mode on\n");
                printf("  -p\tpath=NAME\tPath of logfiles (DEFAULT /tmp/llearn)\n");
                printf("  -V\tversion\t\tPrint version\n");
                printf("  -h/?\thelp\t\tPrint helptext\n");
                exit(0);
                break;
        }
    }

    if(cli_daemon > 0) {
        printf("TODO: daemon mode on\n");
    }
    /* set filesystem preferences  */
    mkdir_p(logPath);
    chmod(logPath, 0777);

    if((r = wiringPiSetup()) < 0 ) {
        fatal(r, "wiringPi Setup");
    }
    if((r = mcp3004Setup(MCP1_START, 0)) < 0 ) {
        fatal(r, "mcp3208-1 Setup");
    }
    if((r = mcp3004Setup(MCP2_START, 1)) < 0 ) {
        fatal(r, "mcp3208-2 Setup");
    }
    if((r = mpu6050Setup()) < 0) {
        fatal(r, "mpu6050 Setup");
    }
    if((r = sht21Setup()) < 0) {
        fatal(r, "sht21 Setup");
    }
    if(wiringPiISR(0, INT_EDGE_FALLING, &s0_impulse) < 0) {
        fatal(r, "wiringPiISR");
    }

    /* create client on smittens.de server. persistence is done by server */
    MQTTClient_create(&mqttc, MQTT_ADDRESS, MQTT_CLIENTID,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    /* mqttc_conopt.will = ... TODO */
    mqttc_conopt.username = MQTT_USERNAME;
    mqttc_conopt.password = MQTT_PASSWORD;

    if((r = MQTTClient_connect(mqttc, &mqttc_conopt)) != MQTTCLIENT_SUCCESS) {
        error(r, "no mqtt connection");
    }

    return stmRun();
}

int stmRun() {
    int r;
    /* init statmachine values */
    stmGetState();
    /* loop machine */
    r = 0;
    while (r == 0) {
        currentTime = time(NULL);
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
            case STM_STATE_PREPROCESS:
                r = stmPreProcess();
                break;
        }
        /* check for state change */
        stmGetState();
        /* collect new SHT data */
        collectShtData();
        collectMpuData();
        if(currentTime > lastDevMqttUpdate + TIMEP_DEV_MQTT) {
            mqttPostDeviceStats();
            lastDevMqttUpdate = time(NULL);
        }
    }
    fatal(r, "stmRun");
    return 0;
}

int stmGetState() {
    /* init statmachine values */
    int m = 0;
    /* check for machine state */
    m = collectLedData();
    if(mState == m)
        return m;
    switch(m) {
        case M_STATE_ON_RUNNING:
            mqttPostMessage("llearnd/machine/status", "läuft", 1);
            /* if it comes from WAITING */
            if(stmState == STM_STATE_WAIT) {
                stmState = STM_STATE_PREPROCESS;
                mqttPostMessage("llearnd/stm/status", "preprocessing", 1);
            } else if(stmState == STM_STATE_INIT) {
                stmState = STM_STATE_PREPROCESS;
            } else {
                mqttPostMessage("llearnd/stm/error",
                    "inconsistent stm state [running]", 1);
                error(0, "inconsistent stm state");
            }
            /* TODO: boot while maching running. what to do??? */
            break;
        case M_STATE_OFF:
        case M_STATE_ON:
            if(m == M_STATE_ON) {
                mqttPostMessage("llearnd/machine/status", "ist an", 1);
            } else {
                mqttPostMessage("llearnd/machine/status", "ist aus", 1);
            }
            /* if it comes from state running */
            if(stmState == STM_STATE_RUNNING) {
                stmState = STM_STATE_POSTPROCESS;
                mqttPostMessage("llearnd/stm/status", "postprocessing", 1);
            } else if (stmState == STM_STATE_INIT) {
                stmState = STM_STATE_WAIT;
                mqttPostMessage("llearnd/stm/status", "waiting", 1);
            }
            break;
        default:
            mqttPostMessage("llearnd/machine/error",
                "default machine state error", 1);
            error (0, "inconsistent machine state");
            stmState = STM_STATE_WAIT;
    }
    mState = m;
    mqttPostDeviceStats();
    return stmState;
}

int stmWait() {
    return 0;
}

int stmRunning() {
    /* TODO: write LogFile every X seconds*/
    if(currentTime > lastLog + TIMEP_LOG_WRITE) {
        wrLog();
        lastLog = time(NULL);
    }
    return 0;
}

int stmPreProcess() {
    FILE *fp;
    /* TODO: open logfile, init it */
    sprintf(logfile, "%s/%d.log", logPath, (unsigned int)time(NULL));
    fp = fopen(logfile, "w+");
    chmod(logfile, 0777);
    chown(logfile, 1000, 1000);
    fprintf(fp, "time,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,sht21hum,sht21temp,mputemp,ax,ay,az,gx,gy,gz,s0\n");
    fclose(fp);

    /* TODO: call machine learning python script to calc approximation */
    stmState = STM_STATE_RUNNING;
    mqttPostMessage("llearnd/stm/status", "running", 1);
    return 0;
}

int stmPostProcess() {
    /* TODO: copy and upload logfile */
    /* TODO: call machine learning script to use this log as training data */
    stmState = STM_STATE_WAIT;
    mqttPostMessage("llearnd/stm/status", "waiting", 1);
    return 0;
}

int mqttPostMessage(char* topic, char* message, char retained) {
    int r;
    MQTTClient_message msg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    if(!MQTTClient_isConnected(mqttc)) {
        if((r = MQTTClient_connect(mqttc, &mqttc_conopt)) != MQTTCLIENT_SUCCESS) {
            error(r, "can't post, no mqtt connect");
            return 0;
        }
    }
    msg.payload = message;
    msg.payloadlen = strlen(message);
    msg.qos = 1;
    msg.retained = retained;
    MQTTClient_publishMessage(mqttc, topic, &msg, &token);
    //MQTTClient_waitForCompletion(mqttc, token, MQTT_TIMEOUT);
    return r;
}

void mqttPostDeviceStats() {
    int i = 0;
    char payload[128];
    sprintf(payload, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                LED_ISON(currentValues[0]), LED_ISON(currentValues[1]),
                LED_ISON(currentValues[2]), LED_ISON(currentValues[3]),
                LED_ISON(currentValues[4]), LED_ISON(currentValues[5]),
                LED_ISON(currentValues[6]), LED_ISON(currentValues[7]),
                LED_ISON(currentValues[8]), LED_ISON(currentValues[9])
            );
    mqttPostMessage("llearnd/device/leds", payload, 1);
    sprintf(payload, "%f,%f",
                currentValues[10], currentValues[11]
            );
    mqttPostMessage("llearnd/device/humtemp", payload, 1);
    sprintf(payload, "%f,%f,%f",
                currentValues[13], currentValues[14],
                currentValues[15]
            );
    mqttPostMessage("llearnd/device/accelerometer", payload, 1);
    sprintf(payload, "%f,%f,%f",
                currentValues[16], currentValues[17],
                currentValues[18]
            );
    mqttPostMessage("llearnd/device/gyro", payload, 1);
    sprintf(payload, "%u", (unsigned int)time(NULL));
    mqttPostMessage("llearnd/device/time", payload, 1);
    sprintf(payload, "%i", stmState);
    mqttPostMessage("llearnd/device/stmState", payload, 1);
    sprintf(payload, "%i", mState);
    mqttPostMessage("llearnd/device/mState", payload, 1);
}


/**
 * TODO: better error handling
        - syslog
        - mqtt post at errors
 */
void error(int num, char msg[]) {
    printf("llearnd error: %d - %s\n", num, msg);
}
void fatal(int num, char msg[]) {
    printf("llearnd fatal: %d - %s\n", num, msg);
    exit(num);
}

int wrLog() {
    int i;
    FILE *fp;
    fp = fopen(logfile, "a+");
    if(fp == NULL) {
        error((int)fp, "file open");
        return 1;
    }
    fprintf(fp, "%lld,", (long long) time(NULL));
    for(i = 0; i < ANALOG_SENSORS; i++) {
        fprintf(fp, "%d,", LED_ISON(currentValues[i]));

    }
    for(i = ANALOG_SENSORS; i < SENSORCOUNT; i++) {
        fprintf(fp, "%f,", currentValues[i]);
    }
    fprintf(fp, "%d\n", s0);
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
    for(i = 1; i < ANALOG_SENSORS; i++) {
        if(LED_ISON(currentValues[i])) {
            return M_STATE_ON;
        }
    }
    return M_STATE_OFF;
}


void collectShtData() {
    currentValues[ANALOG_SENSORS] = sht21GetHum();
    currentValues[ANALOG_SENSORS+1] = sht21GetTemp();
}

void collectMpuData() {
    currentValues[ANALOG_SENSORS+2] = mpu6050GetTmp();
    currentValues[ANALOG_SENSORS+3] = mpu6050GetAx();
    currentValues[ANALOG_SENSORS+4] = mpu6050GetAy();
    currentValues[ANALOG_SENSORS+5] = mpu6050GetAz();
    currentValues[ANALOG_SENSORS+6] = mpu6050GetGx();
    currentValues[ANALOG_SENSORS+7] = mpu6050GetGy();
    currentValues[ANALOG_SENSORS+8] = mpu6050GetGz();
}
void s0_impulse(void) {
    s0++;        //delay(50);
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
