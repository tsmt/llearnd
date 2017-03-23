#!/bin/sh

#echo "shutdown daemon"
#systemctl stop llearnd.service

echo "copy files"
cp llearnd /usr/local/bin/llearnd
cp snu-log.sh /usr/local/bin/snu-log.sh
cp script/finish-lr.py /usr/local/bin/finish-lr.py
cp script/mqtt.ini /usr/local/bin/mqtt.ini

echo "mk logfile folder"
mkdir -p /var/log/llearnd

echo "restart service"
systemctl restart llearnd.service
