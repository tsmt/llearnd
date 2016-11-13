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
import time

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


##############
### Functions
##############

######
## Reorder Rotary Values
##
## These functions reorder the rotary values, so a linear regression
## algorithm has an easier way to calculate a good line.
## The difference in mean squared error is 1900 to 599
######


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

def renameRotaryValues(source, swmap):
    dst = source[:]
    for i in dst:
        if i['rotary'] in swmap:
            i['rotary'] = swmap[i['rotary']]
        else:
            # unknown setting. remap it to "middle"
            i['rotary'] = int(len(swmap)/2)
    return dst

######
## Mean Values for Rotary Settings
##
## This is a trivial algorithm to calulate
## the mean values for each wash setting
######

def deriveMeanValuesForRotary(X, y):
    means = {}
    # For every possible rotary state
    means['all'] = 0
    for i in np.unique(X.rotary):
        rotary = {}
        rotary['meanShort'] = 0
        rotary['mean'] = 0
        rotary['length'] = 0
        for k in X.index:
            #print(X.ix[k].rotary)
            if(X.ix[k].rotary == i):
                #print(ele['rotary'], ele['short'], ele['duration'])

                #derive whole mean for short
                if(X.ix[k].short == 1):
                    if(rotary['meanShort'] != 0):
                        rotary['meanShort'] = (rotary['meanShort'] + y.ix[k]) / 2
                    else:
                        rotary['meanShort'] = y.ix[k]

                # derive whole mean for non short
                else:
                    if(rotary['mean'] != 0):
                        rotary['mean'] = (rotary['mean'] + y.ix[k]) / 2
                    else:
                        rotary['mean'] = y.ix[k]
                rotary['length'] = rotary['length'] + 1
                #print(rotary)

            # derive whole mean
            if(means['all'] != 0):
                means['all'] = (means['all'] + y.ix[k]) / 2
            else:
                means['all'] = y.ix[k]

        means[i] = rotary
    return means

# predict for input X with meanValue
def predictWashTime(X, Mean):
    y = []
    for k in X.index:
        if X.ix[k].short == 1 and Mean[X.ix[k].rotary]['meanShort'] > 0:
            y.append(Mean[X.ix[k].rotary]['meanShort'])
        elif X.ix[k].short == 0 and Mean[X.ix[k].rotary]['mean'] > 0:
            y.append(Mean[X.ix[k].rotary]['mean'])
        else:
            y.append(Mean['all'])

    return y



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

# derive mean value map
remap = deriveMeanValueRemap(llearn_data, rot_nums)

# rename rotary values with map
llearn_data = renameRotaryValues(llearn_data, remap)
prediction_data = renameRotaryValues([{'rotary': args.rotary, 'short': args.short}], remap)

# make a pandas dataframes
llearnpd = pd.DataFrame.from_records(llearn_data)
llearnpred = pd.DataFrame.from_records(prediction_data)

# Prepare feature set of relevant settings
X = llearnpd[["rotary", "short"]]
# Prepare y
y = llearnpd["duration"]


#####
# Linear Regression
#####
# instantiate
linreg = LinearRegression()
# fit the model to the training data (learn the coefficients)
linreg.fit(X, y)

# ### make a predition
# make predictions on the testing set
lin_pred = linreg.predict(llearnpred)


#####
# Own Algorithm
#####
meanValues = deriveMeanValuesForRotary(X, y)
own_pred = predictWashTime(llearnpred, meanValues)


# load testitem and do a prediction

# print
lin_endtime = int(lin_pred[0]) + args.timestamp;
own_endtime = int(own_pred[0]) + args.timestamp;

# connect to MQTT client
mclient.publish("llearnd/learn/linreg", str(lin_endtime), retain=True)
mclient.publish("llearnd/learn/ownpred", str(own_endtime), retain=True)
mclient.publish("llearnd/learn/text", str(own_endtime), retain=True)
time.sleep(3)
mclient.disconnect()
