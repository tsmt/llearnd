#!/bin/bash

rsync -az /tmp/llearn/ /var/log/llearnd/
rsync  -az /var/log/llearnd/ ts@rhea.local:/home/ts/logs/
# TODO: upload to webserver
