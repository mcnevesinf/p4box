#!/bin/bash

valid=true
count=1

while [ $valid ] 
do
	nPackets="$(sudo /opt/netronome/p4/bin/rtecli counters get -c nPackets_packets -i 0)"
	echo "${nPackets}"
	clearCounter="$(sudo /opt/netronome/p4/bin/rtecli counters clear -c nPackets_packets -i 0)"
	sleep 1s

	if [ $count -eq 30 ] 
	then
		break
	fi
	((count++))
done
