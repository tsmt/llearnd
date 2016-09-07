#!/bin/sh

make clean
make
echo "shutdown daemon"
systemctl stop llearnd.service

echo "copy files"
cp llearnd /usr/local/bin/llearnd
cp snu-log.sh /usr/local/bin/snu-log.sh

echo "mk logfile folder"
mkdir -p /var/log/llearnd

echo "restart service"
systemctl start llearnd.service
