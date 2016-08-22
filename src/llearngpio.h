#ifndef _LLEARNGPIO_H
#define _LLEARNGPIO_H


#define MCP1_START 100
#define MCP2_START 108
#define ANALOG_SENSORS 10
#define SENSORCOUNT ANALOG_SENSORS+9 

#define TIMEPERIOD_COLLECT 1
#define TIMEPERIOD_PRINT 1
#define TIMEPERIOD_MS_DEBUNK 50

typedef struct {
    float *value;
    const char *fname;
    const char *mode;
} Device;


#endif
