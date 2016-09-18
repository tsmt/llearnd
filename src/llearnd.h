#ifndef _LLEARNGPIO_H
#define _LLEARNGPIO_H

#define VERSION "beta1"

#define MCP1_START 100
#define MCP2_START 108
#define ANALOG_SENSORS 10
#define SENSORCOUNT ANALOG_SENSORS+9

#define LED_THRESHOLD 0.9
#define LED_ISON(x) (x < LED_THRESHOLD)

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
#define ROTARY_SETTINGS_COUNT 21

#define TIMEP_DEV_MQTT 30
#define TIMEP_LOG_WRITE 1

#define STM_STATE_WAIT 0
#define STM_STATE_RUNNING 1
#define STM_STATE_POSTPROCESS 2
#define STM_STATE_PREPROCESS 3
#define STM_STATE_INIT 99
#define STM_FREQUENCY_MS 50

#define M_STATE_INIT 99
#define M_STATE_OFF 0
#define M_STATE_ON 1
#define M_STATE_ON_RUNNING 2

int stmRun();
int stmGetState();
int stmGetMachineState();
int stmGetRotaryState();
int stmWait();
int stmPreProcess();
int stmRunning();
int stmPostProcess();
int stPostDevInfo();

void error(int num, char msg[]);
void critical(int num, char msg[]);

int mkdir_p(const char *path);
int initLog();
int wrLog();

int collectLedData();
void collectShtData(void);
void collectMpuData(void);
void s0_impulse(void);

int mqttPostMessage(char* topic, char* message, char retained);
void mqttPostDeviceStats();

const float rotarySettings[ROTARY_SETTINGS_COUNT][3] = {
    {-0.44, -0.03, -0.97}, /* aus */
    {-0.44, -0.42, -0.87}, /* kochwaesche 90 */
    {-0.43, -0.63, -0.70}, /* kochwaesche 60 */
    {-0.40, -0.82, -0.49}, /* kochwaesche 40 */
    {-0.35, -0.91, -0.22}, /* kochwaesche 30 */
    {-0.30, -0.94, 0.04}, /* mit vor 60 */
    {-0.24, -0.88, 0.30}, /* mit vor 40 */
    {-0.18, -0.77, 0.54}, /* pflege 60 */
    {-0.12, -0.58, 0.72}, /* pflege 40 */
    {-0.08, -0.35, 0.86}, /* mix 20 */
    {-0.05, -0.11, 0.92}, /* leichtbuegeln 40 */
    {-0.03, 0.18, 0.90}, /* feinwaesche 40 */
    {-0.04, 0.46, 0.81}, /* feinwaesche 30 */
    {-0.05, 0.68, 0.65}, /* wolle+ 30 */
    {-0.08, 0.85, 0.43}, /* wolle+ 40 */
    {-0.12, 0.93, 0.21}, /* feinspuelen */
    {-0.19, 0.96, -0.12}, /* pumpen */
    {-0.22, 0.90, -0.38}, /* schleudern */
    {-0.30, 0.76, -0.62}, /* 30 min 30 kg*/
    {-0.35, 0.57, -0.80}, /* baumw. eco 40 */
    {-0.40, 0.33, -0.93} /* baumw. eco 60 */
};

#endif
