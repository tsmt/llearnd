# llearnd
laundry learn daemon

## outline
This daemon is running on a laundry learn raspberry pi and acts as a whole
management daemon. It handles GPIO, detects the state of the laundry machine
and also posts live data to MQTT. It runs in as a finite statemachine to
map the laundry machine states and it will handle the different machine learning
scripts (python with scikit-learn) to determine a final approximation for the
laundry machine runtime.

## Install
#### Install wiringPi
#### Install Daemon

```
sudo cp llearnd.service /etc/systemd/system
sudo ./install.sh
systemctl enable llearnd.service
systemctl start llearnd.service
```

## source description

### llearnd.c / llearnd.h
This is the llearnd main program. It handles:
* system functionality (daemon mode, syslog, error handling, signals)
* machine state detection
* intern statemachine handling
* MQTT Broker connection

When started, the daemon handles typical system functionality like commandline
arguments and init of syslog and hardware modules. After that it changes into
statemachine mode. This statemachine has following states:

#### stmWait
This state is chosen, when the laundry machine is *not* running. This state has
not further logging. It reports the machine state to the MQTT Broker every 30 seconds.


#### stmPreProcess
This state is called, when the machine started to wash. It's purpose is to create
a logfile and start a machine learning script to approximate a machine runtime.
It then automatically switches to stmRun mode.

#### stmRun
When the laundry machine is running, the statemachine is in stmRun mode. This
state posts current machine stats to the MQTT broker and also logs all sensor
data into a logfile every second.

#### stmPostProcess
When a laundry machine run has ended, the PostProcess state will be called.
Postprocessing means, that the logfile will be closed, copied to a backup
location, analyzed and used as training data for the machine learning function.
After that, the state automatically switches to stmWait

### mpu6050.c / mpu6050.h
These files handle the MPU6050 3-axis-accelerometer and gyrometer. This module
is connected via I2C bus and used to determine a shakeValue (when machine is
in spin-mode) and the state of the rotary switch, which sets the running mode
of the laundry machine (Kochwäsche/Pflegestufe/Temperatur etc.).

### sht21.c / sht21.h
This is a temperature and humidity sensor which is also connected via SPI. It has
no special purpose besides detecting the room temperature and humidity.


## called scripts
### snu-log.sh
bash script to save and upload a logfile
### Machine Learning .py
A python script which processes logfiles and tries to determine a remaining
runtime for the laundry machine.
