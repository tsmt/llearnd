#ifndef _LLEARNGPIO_H
#define _LLEARNGPIO_H


#define MCP1_START 100
#define MCP2_START 108
#define ANALOG_SENSORS 10
#define SENSORCOUNT ANALOG_SENSORS+9

#define LED_THRESHOLD 0.100
#define LED_ISON(x) (x > LED_THRESHOLD)

#define LED_WASCHEN 0
#define LED_ENDE 1
#define LED_EXTRASPUELUNG 2
#define LED_STARTSTOP 3
#define LED_1600 4
#define LED_900 5
#define LED_500 6
#define LED_400 7
#define LED_KURZ 8
#define LED_FLECKEN 9

#define TIMEPERIOD_COLLECT 100
#define TIMEPERIOD_PRINT 500
#define TIMEPERIOD_PRINTLOG 5000
#define TIMEPERIOD_MS_DEBUNK 60

#define STM_STATE_WAIT 0
#define STM_STATE_RUNNING 1
#define STM_STATE_POSTPROCESS 2
#define STM_FREQUENCY_MS 50

#define M_STATE_OFF 0
#define M_STATE_ON 1
#define M_STATE_ON_RUNNING 2

int stmRun();
int stmWait();
int stmPreProcess();
int stmRunning();
int stmPostProcess();
int stPostDevInfo();

int error(int num, char msg[]);

int mkdir_p(const char *path);
int wrLog();

int collectLedData();
void collectSensorData(void);
void s0_impulse(void);

int mqttConnect();
int mqttPostDeviceStats();
int mqttPostMachineStats();

#endif
