#!/usr/bin/python

##
# Derive finish timestamp with Linear Regression
# Author: Tobias Schmitt [tobias.schmitt@mni.thm.de]
# 2016, Gießen
##

## imports and chdir

import glob, os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
os.chdir("logs/")


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

remap = deriveMeanValueRemap(llearn_data, rot_nums)
llearn_data = renameRotaryValues(llearn_data, remap)
llearnpd = pd.DataFrame.from_records(llearn_data)

# Linear Regression:
# split set into X and y
# Prepare feature set of relevant settings
X = llearnpd[["rotary", "short"]]
# Prepare y
y = llearnpd["duration"]


# ### split training and test sets
from sklearn.cross_validation import train_test_split
X_train, X_test, y_train, y_test = train_test_split(X, y, random_state=1)


# ### linear regression with scikit
# import model
from sklearn.linear_model import LinearRegression

# instantiate
linreg = LinearRegression()
# fit the model to the training data (learn the coefficients)
linreg.fit(X_train, y_train)

# ### make a predition
# make predictions on the testing set
y_pred = linreg.predict(X_test)
from sklearn import metrics


# ### calculate errors
# In[11]:
print(np.sqrt(metrics.mean_squared_error(y_test, y_pred)))
# In[12]:
print(np.sqrt(metrics.mean_absolute_error(y_test, y_pred)))


# ### playground

# In[30]:

testitem = pd.DataFrame.from_records([{'rotary': 1, 'short': 0}]) # , 'spin': 0, 'short': 0
linreg.predict(testitem)
