#!/bin/sh

make clean
make
cp llearnd /usr/local/bin/llearnd
mkdir -p /var/log/llearnd
