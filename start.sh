#!/bin/bash

TIME=5

trap "killall -u ${USER} -9 sdohdr" SIGINT SIGTERM

for port in 53268 53276 53284 53292 53300 53308
do 
  cmd="sdohdr -f $port.bin -t $TIME -u $port"
  echo $cmd
  ./$cmd &
done

wait
