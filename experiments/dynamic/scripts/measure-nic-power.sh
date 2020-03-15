#!/bin/bash

valid=true
count=1

while [ $valid ] 
do
	readPower="$(sudo /opt/netronome/bin/nic-power | grep current)"
	
	powerValue=${readPower:9:6}

	powerVector[$count]=$powerValue
	
	sleep 0.1s

	if [ $count -eq 300 ] 
	then
		break
	fi
	((count++))
done

for i in "${powerVector[@]}"
do
	echo $i >> power.dat
done
