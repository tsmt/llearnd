# llearnd
laundry learn daemon

## outline
This daemon is running on a laundry learn raspberry pi and acts as a whole
management daemon. It handles GPIO, detects the state of the laundry machine
and also posts live data to MQTT. It runs in as a finite statemachine to
map the laundry machine states and it will handle the different machine learning
scripts (python with scikit-learn) to determine a final approximation for the
laundry machine runtime.

## statemachine
### stmState
### stmWait
### stmRunning
### stmPreProcess
### stmPostProcess

## called scripts
### snu-log.sh
bash script to save and upload a logfile
### Machine Learning .py
A python script which processes logfiles and tries to determine a remaining
runtime for the laundry machine.
