#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <wiringPi.h>
#include <mcp3004.h>
#include "llearnd.h"
#include "mpu6050.h"
#include "sht21.h"
#include "config.h"
#include "../include/MQTTClient.h"

#define FDELTA(x,y) (fabsf(x - y))

/* Global values */

/* device Values TODO: array docu */
float currentValues[SENSORCOUNT];
float shakeValue;
int rotaryState;

const char defLogPath[] = "/tmp/llearn";
const char *logPath = defLogPath;
char logfile[256];
unsigned int s0 = 0;
unsigned int stmState = STM_STATE_INIT;
unsigned int mState = M_STATE_INIT;

time_t currentTime;
time_t lastDevMqttUpdate;
time_t lastLog;
time_t lastStart;

/* CLI Options */
unsigned int cli_daemon = 0;

/* process & session id */
unsigned int pid, sid;


/* MQTT vars */
MQTTClient mqttc;
MQTTClient_connectOptions mqttc_conopt = MQTTClient_connectOptions_initializer;
MQTTClient_willOptions mqttc_willopt = MQTTClient_willOptions_initializer;


void sig_exit_handler(int signum) {
    syslog(LOG_WARNING, "Received signal %d - exiting", signum);
    if(signum == SIGINT || signum == SIGTERM) {
        closelog();
        fcloseall();
        /* TODO: * copy logfile to savepoint when in STM_STATE_RUNNING or
                        STM_STATE_POSTPROCESS
                 * maybe call a reboot script?
        */
        mqttPostMessage("llearnd/state", "offline(shutdown)", 1);
        MQTTClient_disconnect(mqttc, 500);
        exit(0);
    }
}


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
                exit(EXIT_SUCCESS);
                break;
            case 'h': case '?':
                printf("llearnd Version %s. Built %s %s\n", VERSION,
                                            __DATE__, __TIME__);
                printf("  -d\tdaemon\t\tDaemon mode on\n");
                printf("  -p\tpath=NAME\tPath of logfiles (DEFAULT /tmp/llearn)\n");
                printf("  -V\tversion\t\tPrint version\n");
                printf("  -h/?\thelp\t\tPrint helptext\n");
                exit(EXIT_SUCCESS);
                break;
        }
    }

    if(cli_daemon > 0) {
        /* initialize daemon mode (source:
            http://www.thegeekstuff.com/2012/02/c-daemon-process
        )*/
        pid = fork();
        if(pid < 0) {
            printf("fork failed\n");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            /* parent process */
            printf("[%d]\n", pid);
            exit(EXIT_SUCCESS);
        }
        /* child process */
        pid = getpid();
        /* umask files */
        umask(0);
        /* set session id */
        sid = setsid();
        if(sid < 0) {
            printf("set session id failed\n");
            exit(EXIT_FAILURE);
        }
        /* close stdin/out/err */
        close(STDERR_FILENO);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
    }
    /* set up devices */
    if((r = wiringPiSetup()) < 0 ) {
        critical(r, "wiringPi Setup");
    }
    if((r = mcp3004Setup(MCP1_START, 0)) < 0 ) {
        critical(r, "mcp3208-1 Setup");
    }
    if((r = mcp3004Setup(MCP2_START, 1)) < 0 ) {
        critical(r, "mcp3208-2 Setup");
    }
    if((r = mpu6050Setup()) < 0) {
        critical(r, "mpu6050 Setup");
    }
    if((r = sht21Setup()) < 0) {
        critical(r, "sht21 Setup");
    }
    if(wiringPiISR(0, INT_EDGE_FALLING, &s0_impulse) < 0) {
        critical(r, "wiringPiISR");
    }

    /* open syslogd for error reporting */
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog(argv[0], LOG_CONS | LOG_NDELAY, LOG_LOCAL1);

    /* set filesystem preferences  */
    mkdir_p(logPath);
    chmod(logPath, 0777);

    /* set signals  */
    signal(SIGINT, &sig_exit_handler);
    signal(SIGTERM, &sig_exit_handler);

    /* create client on smittens.de server. persistence is done by server */
    MQTTClient_create(&mqttc, MQTT_ADDRESS, MQTT_CLIENTID,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    /* mqttc_conopt.will = ... TODO */
    mqttc_willopt.message = "offline(disconnect)";
    mqttc_willopt.topicName = "llearnd/state";
    mqttc_willopt.retained = 1;
    mqttc_conopt.will = &mqttc_willopt;
    mqttc_conopt.username = MQTT_USERNAME;
    mqttc_conopt.password = MQTT_PASSWORD;

    if((r = MQTTClient_connect(mqttc, &mqttc_conopt)) != MQTTCLIENT_SUCCESS) {
        error(r, "no mqtt connection");
    }
    mqttPostMessage("llearnd/state", "online", 1);

    syslog(LOG_NOTICE, "successfully booted");

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
        rotaryState = stmGetRotaryState();
        /* collect new SHT data */
        collectShtData();
        collectMpuData();
        if(currentTime >= lastDevMqttUpdate + TIMEP_DEV_MQTT) {
            mqttPostDeviceStats();
            lastDevMqttUpdate = time(NULL);
        }
    }
    critical(r, "stmRun");
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
            stmState = STM_STATE_PREPROCESS;
            break;
        case M_STATE_OFF:
        case M_STATE_ON:
            stmState = STM_STATE_WAIT;
            break;
    }
    mState = m;
    mqttPostDeviceStats();
    return stmState;
}

int stmGetRotaryState() {
    int i,k;
    float diffs[ROTARY_SETTINGS_COUNT];
    for(i = 0; i < ROTARY_SETTINGS_COUNT; i++) {
        diffs[i] = fabsf(rotarySettings[i][0] - currentValues[13]);
        diffs[i] += fabsf(rotarySettings[i][1] - currentValues[14]);
        diffs[i] += fabsf(rotarySettings[i][2] - currentValues[15]);
    }
    /* get smallest difference -> assume this is the right setting */
    k = 0;
    for(i = 1; i < ROTARY_SETTINGS_COUNT; i++) {
        if(diffs[i] < diffs[k]) {
            k = i;
        }
    }
    return k;
}

int stmWait() {
    return 0;
}

int stmRunning() {
    /* TODO: write LogFile every X seconds*/
    if(currentTime >= lastLog + TIMEP_LOG_WRITE) {
        wrLog();
        lastLog = time(NULL);
    }
    return 0;
}

int stmPreProcess() {
    int r;
    FILE *fp;
    char payload[32];
    char rotary[2];
    char shrt[1];
    /* TODO: open logfile, init it */
    sprintf(logfile, "%s/%d.log", logPath, (unsigned int)time(&lastStart));
    fp = fopen(logfile, "w+");
    chmod(logfile, 0777);
    chown(logfile, 1000, 1000);
    fprintf(fp, "time,rotary,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,sht21hum,sht21temp,mputemp,ax,ay,az,gx,gy,gz,shake,s0\n");
    fclose(fp);

    sprintf(payload, "%ld", (long)time(NULL));
    mqttPostMessage("llearnd/machine/lastBegin", payload, 1);

    /* TODO: call machine learning python script to calc approximation */
    rotaryState = stmGetRotaryState();

    sprintf(rotary, "%d", rotaryState);
    sprintf(shrt, "%d", LED_ISON(currentValues[6]));

    r = fork();
    if(r == 0) {
        execlp("python3", "python3", "/usr/local/bin/finish-lr.py", 
                "-c", "/usr/local/bin/mqtt.ini", "-d", "/var/log/llearnd", 
                "-r", rotary ,"-s", shrt, "-t", payload, (char*)0);
        exit(-1);
    }
    stmState = STM_STATE_RUNNING;
    return 0;
}

int stmPostProcess() {
    /* TODO: copy and upload logfile */
    /* TODO: call machine learning script to use this log as training data */
    stmState = STM_STATE_WAIT;
    return 0;
}

int mqttPostMessage(char* topic, char* message, char retained) {
    int r;
    MQTTClient_message msg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    if(!MQTTClient_isConnected(mqttc)) {
        if((r = MQTTClient_connect(mqttc, &mqttc_conopt)) != MQTTCLIENT_SUCCESS) {
            return 0;
        }
        mqttPostMessage("llearnd/state", "online", 1);
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
    sprintf(payload, "%f", shakeValue);
    mqttPostMessage("llearnd/device/shakeValue", payload, 1);
    sprintf(payload, "%i", rotaryState);
    mqttPostMessage("llearnd/device/rotaryState", payload, 1);
    sprintf(payload, "%i", stmState);
    mqttPostMessage("llearnd/stm/state", payload, 1);
    sprintf(payload, "%i", mState);
    mqttPostMessage("llearnd/machine/state", payload, 1);
    sprintf(payload, "%u", (unsigned int)time(NULL));
    mqttPostMessage("llearnd/time", payload, 1);
}


/**
 * TODO: better error handling
        - syslog
        - mqtt post at errors
 */
void error(int num, char msg[]) {
    syslog(LOG_WARNING, "llearnd error: %d - %s", num, msg);
}
void critical(int num, char msg[]) {
    syslog(LOG_CRIT, "llearnd fatal: %d - %s. Shutting down!", num, msg);
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
    fprintf(fp, "%lld,%d,", (long long)(time(NULL) - lastStart), rotaryState);
    for(i = 0; i < ANALOG_SENSORS; i++) {
        fprintf(fp, "%d,", LED_ISON(currentValues[i]));

    }
    for(i = ANALOG_SENSORS; i < SENSORCOUNT; i++) {
        fprintf(fp, "%.2f,", currentValues[i]);
    }
    fprintf(fp, "%.2f,%d\n", shakeValue, s0);
    fclose(fp);
    shakeValue = 0.0;
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
    float Ax = 0.0, Ay = 0.0, Az = 0.0, Gx = 0.0,
            Gy = 0.0, Gz = 0.0, delta = 0.0;
    if(stmState == STM_STATE_RUNNING) {
        /* save old Values */
        Ax = currentValues[ANALOG_SENSORS+3];
        Ay = currentValues[ANALOG_SENSORS+4];
        Az = currentValues[ANALOG_SENSORS+5];
        Gx = currentValues[ANALOG_SENSORS+6];
        Gy = currentValues[ANALOG_SENSORS+7];
        Gz = currentValues[ANALOG_SENSORS+8];
    }
    /* get new values */
    currentValues[ANALOG_SENSORS+2] = mpu6050GetTmp();
    currentValues[ANALOG_SENSORS+3] = mpu6050GetAx();
    currentValues[ANALOG_SENSORS+4] = mpu6050GetAy();
    currentValues[ANALOG_SENSORS+5] = mpu6050GetAz();
    currentValues[ANALOG_SENSORS+6] = mpu6050GetGx();
    currentValues[ANALOG_SENSORS+7] = mpu6050GetGy();
    currentValues[ANALOG_SENSORS+8] = mpu6050GetGz();
    /* calc delta */
    if(stmState == STM_STATE_RUNNING) {
        delta = 0.0;
        delta += FDELTA(Ax,currentValues[ANALOG_SENSORS+3]);
        delta += FDELTA(Ay,currentValues[ANALOG_SENSORS+4]);
        delta += FDELTA(Az,currentValues[ANALOG_SENSORS+5]);
        delta += FDELTA(Gx,currentValues[ANALOG_SENSORS+6]);
        delta += FDELTA(Gy,currentValues[ANALOG_SENSORS+7]);
        delta += FDELTA(Gz,currentValues[ANALOG_SENSORS+8]);
        /* add ShakeValue */
        shakeValue += delta;
    }
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
