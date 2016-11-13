#!/bin/bash

rsync -az /tmp/llearn/ /var/log/llearnd/
rsync  -az /var/log/llearnd/ ts@192.168.34.5:/home/ts/logs/
# TODO: upload to webserver
