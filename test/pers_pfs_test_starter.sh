#!/bin/bash

# Use this script to start the persistence power fail save test after system restart

# HOW TO USE
# to start this script on startup, copy the systemd service file persistence-pfs-test-start.service to /lib/systemd/system/
# Create also a link from /lib/systemd/system/multi-user.target.wants 
# to the persistence-pfs-test-start.service in /lib/systemd/system/

echo "Starting persistence power fail save test"

check_process() {
  echo "$ts: checking $1"
  [ "$1" = "" ]  && return 0
  [ `pgrep -n $1` ] && return 1 || return 0
}

#!!!!!!!!!!!!dlt daemon has to run before starting test
# check if already running, otherwise error
# timestamp
ts=`date +%T`

echo "$ts: begin checking apps needed..."
check_process "dlt-daemon"
if [ $? == 0 ]
then
   echo "$ts: not running, starting ..."
   dlt-daemon &
else
   echo "$ts: already running..."
fi

export LD_LIBRARY_PATH='/usr/lib/'

if [ $# != 0 ]
then
   numLoops=$1
else
  numLoops=10000000
fi
  
/usr/bin/persistence_pfs_test "-l $numLoops" "-s/dev/ttyUSB0"
 

echo "End of persistence power fail save test"