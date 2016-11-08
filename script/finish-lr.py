# coding: utf-8

##
# Derive finish timestamp with Linear Regression
# Author: Tobias Schmitt [tobias.schmitt@mni.thm.de]
# 2016, Gießen
##

## imports and chdir

import glob, os, sys
import pandas as pd
import numpy as np
import warnings
import argparse
import configparser
import paho.mqtt.client as mqttc

from sklearn.cross_validation import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn import metrics

parser = argparse.ArgumentParser(description="LLearn Finishtimes Script")
parser.add_argument('-d', '--directory', help='Directory of training data', default="./testlogs")
parser.add_argument('-c', '--config', help='config file', default="./mqtt.ini")
parser.add_argument('-r', '--rotary', type=int, help='Rotary switch setting', default=0)
parser.add_argument('-s', '--short', type=int, help='Short mode [0/1]', default=0)
parser.add_argument('-t', '--timestamp', type=int, help='timestamp for start', default=0)
args = parser.parse_args()
#print(args)

config = configparser.ConfigParser()
config.read(args.config)

## connect mqtt and update


# connect to MQTT client
mclient = mqttc.Client()
mclient.username_pw_set(config['MQTT']['USER'], config['MQTT']['PASS'])
mclient.connect(config['MQTT']['SERVER'], int(config['MQTT']['PORT']), 60)
mclient.publish("llearnd/learn/text", "rechnen...", retain=True)
mclient.publish("llearnd/learn/rotary", str(args.rotary), retain=True)
mclient.publish("llearnd/learn/short", str(args.short), retain=True)
mclient.publish("llearnd/learn/timestamp", str(args.timestamp), retain=True)

# ignore filterwarning in scikit
warnings.filterwarnings(action="ignore", module="scipy", message="^internal gelsd")
# go to testdir
os.chdir(args.directory)

#
# ## fetch and sort data
#
# Read all .log files from directory and pass important values to array.
# This array consists of:
# * Starttime as UNIX Timestamp (int)
# * Setting of Rotary (0-23)
# * Setting of Spin (0-3)
# * Short-Laundry Setting (bool)
# * Maximum Shake Value (float)
# * Minimum Shake Value (float)
# * Length of Run (int)

llearn_data = []
for filename in glob.glob("*.log"):
    item = {}
    file = pd.read_csv(filename)
    # get settings
    item["starttime"] = int(filename.rstrip(".log"))
    item["rotary"] = file["rotary"][0]
    # spin
    item["spin"] = 0 if (file["a1"][0] == 1) else (1 if (file["a2"][0] == 1) else 2)
    item["short"] = file["a6"][0]
    item["maxShake"] = file["shake"].max()
    item["minShake"] = file["shake"].min()
    item["duration"] = int(file["time"].iloc[-1:])
    item["maxpower"] = int(file["s0"].iloc[-1:])
    llearn_data.append(item)
llearnpd = pd.DataFrame.from_records(llearn_data)
rot_nums = np.unique(llearnpd.rotary)

def deriveMeanValueRemap(data, array):
    dictmap = {}
    for i in array:
        item = {}
        duration = None
        for dfi in data:
            if dfi['rotary'] == i:
                if duration == None:
                    duration = dfi['duration']
                else:
                    duration = (duration + dfi['duration']) / 2
        item['meanDuration'] = duration
        dictmap[i] = item
    dictmap = sorted(dictmap.items(), key=lambda k: k[1]['meanDuration'])
    remap = {}
    for i in range(0, len(dictmap)):
        remap[dictmap[i][0]] = i
    return remap

def renameRotaryValues(source, remap):
    for i in source:
        i['rotary'] = remap[i['rotary']]
    return source

# derive mean value map
remap = deriveMeanValueRemap(llearn_data, rot_nums)

# rename rotary values with map
llearn_data = renameRotaryValues(llearn_data, remap)
prediction_data = renameRotaryValues([{'rotary': args.rotary, 'short': args.short}], remap)

# make a pandas dataframes
llearnpd = pd.DataFrame.from_records(llearn_data)
llearnpred = pd.DataFrame.from_records(prediction_data)
# print(llearnpred)

# Linear Regression:
# split set into X and y
# Prepare feature set of relevant settings
X = llearnpd[["rotary", "short"]]
# Prepare y
y = llearnpd["duration"]


# ### split training and test sets
X_train, X_test, y_train, y_test = train_test_split(X, y, random_state=1)


# ### linear regression with scikit
# import model

# instantiate
linreg = LinearRegression()
# fit the model to the training data (learn the coefficients)
linreg.fit(X_train, y_train)

# ### make a predition
# make predictions on the testing set
y_pred = linreg.predict(X_test)

# calculate RMAE and Absolue Error
sqrerr = np.sqrt(metrics.mean_squared_error(y_test, y_pred))
abserr = metrics.mean_absolute_error(y_test, y_pred)

# load testitem and do a prediction
pred = linreg.predict(llearnpred)

# print
endtime = int(pred[0]) + args.timestamp;

# connect to MQTT client
mclient.publish("llearnd/learn/text", str(endtime), retain=True)
mclient.disconnect()
