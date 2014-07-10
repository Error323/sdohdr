#!/bin/bash

TIME=5

for port in 53268 53276 53284 53292 53300 53308
do 
  nc -u -l 10.99.100.1 $port | ./sdohdr -f $port.bin -t $TIME -i &
done

wait
