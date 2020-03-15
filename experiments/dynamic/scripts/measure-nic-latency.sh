#!/bin/bash

valid=true
count=1

while [ $valid ] 
do
	nPackets="$(sudo /opt/netronome/p4/bin/rtecli registers get -r latency -i 0)"
	#echo "${nPackets}"
	startValueStr=${nPackets:12:8}
	endValueStr=${nPackets:34:8}
	
	startValue=$(( 16#$startValueStr ))
	endValue=$(( 16#$endValueStr ))

	latency=$[endValue-startValue]
	latencyVector[$count]=$latency
	#echo "${latency}"
	
	sleep 0.1s

	if [ $count -eq 100 ] 
	then
		break
	fi
	((count++))
done

for i in "${latencyVector[@]}"
do
	#echo ${latencyVector[@]} > latency.dat
	echo $i >> latency.dat
done
